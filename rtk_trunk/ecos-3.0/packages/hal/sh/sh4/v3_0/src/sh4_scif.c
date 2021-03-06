//=============================================================================
//
//      sh4_scif.c
//
//      Simple driver for the SH4 Serial Communication Interface with FIFO
//
//=============================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//=============================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):   jskov
// Contributors:Haruki Kashiwaya
// Date:        2000-08-09
// Description: Simple driver for the SH Serial Communication Interface
//              The driver can be used for the SCIF modules.
//              Clients of this file can configure the behavior with:
//              CYGNUM_SCIF_PORTS: number of SCI ports
//
//####DESCRIPTIONEND####
//
//=============================================================================

#include <pkgconf/hal.h>

#ifdef CYGNUM_HAL_SH_SH4_SCIF_PORTS

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/drv_api.h>            // CYG_ISR_HANDLED
#include <cyg/hal/hal_misc.h>           // Helper functions
#include <cyg/hal/hal_intr.h>           // HAL_ENABLE/MASK/UNMASK_INTERRUPTS
#include <cyg/hal/hal_arch.h>           // SAVE/RESTORE GP
#include <cyg/hal/hal_if.h>             // Calling-if API
#include <cyg/hal/sh_regs.h>            // serial register definitions
#include <cyg/hal/sh_stub.h>            // target_register_t

#define CYGPRI_HAL_SH_SH4_SCIF_PRIVATE
#include <cyg/hal/sh4_scif.h>           // our header

//--------------------------------------------------------------------------

static void
cyg_hal_plf_scif_set_baud(cyg_uint8 *base, cyg_uint32 baud)
{
    cyg_uint16 tmp;
    
    // Set desired baudrate
    HAL_READ_UINT16(base+_REG_SCSMR, tmp);
    tmp &= ~CYGARC_REG_SCIF_SCSMR_CKSx_MASK;
    tmp |= CYGARC_SCBRR_CKSx(baud);
    HAL_WRITE_UINT16(base+_REG_SCSMR, tmp);
    HAL_WRITE_UINT8(base+_REG_SCBRR, 
                    CYGARC_SCBRR_N(baud));

    // Let things settle: Here we should wait the equivalent of
    // one bit interval, i.e. 1/<baudrate> second, but we'll wait twice
    // that to be sure.
    CYGACC_CALL_IF_DELAY_US(2000000/baud);
}

void
cyg_hal_plf_scif_init_channel(channel_data_t* chan)
{
    cyg_uint8* base = chan->base;
    cyg_uint16 sr;

    // Disable everything.
    HAL_WRITE_UINT16(base+_REG_SCSCR, 0);

    // Reset FIFO.
    HAL_WRITE_UINT16(base+_REG_SCFCR, 
                    CYGARC_REG_SCIF_SCFCR_TFRST|CYGARC_REG_SCIF_SCFCR_RFRST);

    // 8-1-no parity.
    HAL_WRITE_UINT16(base+_REG_SCSMR, 0);

    chan->baud_rate = CYGNUM_HAL_SH_SH4_SCIF_BAUD_RATE;
    cyg_hal_plf_scif_set_baud(base, CYGNUM_HAL_SH_SH4_SCIF_BAUD_RATE);

    // Clear status register (read back first).
    HAL_READ_UINT16(base+_REG_SCFSR, sr);
    HAL_WRITE_UINT16(base+_REG_SCFSR, 0);

    // Bring FIFO out of reset and set to trigger on every char in
    // FIFO (or C-c input would not be processed).
    HAL_WRITE_UINT16(base+_REG_SCFCR, 
                    CYGARC_REG_SCIF_SCFCR_RTRG_1|CYGARC_REG_SCIF_SCFCR_TTRG_1);

    // Leave Tx/Rx interrupts disabled, but enable Tx/Rx
    HAL_WRITE_UINT16(base+_REG_SCSCR, 
                    CYGARC_REG_SCIF_SCSCR_TE|CYGARC_REG_SCIF_SCSCR_RE);
}

static cyg_bool
cyg_hal_plf_scif_getc_nonblock(void* __ch_data, cyg_uint8* ch)
{
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    cyg_uint16 fdr, sr;

    HAL_READ_UINT16(base+_REG_SCFDR, fdr);
    if ((fdr & CYGARC_REG_SCIF_SCFDR_RCOUNT_MASK) == 0)
        return false;

    HAL_READ_UINT8(base+_REG_SCFRDR, *ch);

    // Clear FIFO full flag (read before clearing)
    HAL_READ_UINT16(base+_REG_SCFSR, sr);
    HAL_WRITE_UINT16(base+_REG_SCFSR,
                    CYGARC_REG_SCIF_SCSSR_CLEARMASK & ~CYGARC_REG_SCIF_SCSSR_RDF);

    return true;
}

cyg_uint8
cyg_hal_plf_scif_getc(void* __ch_data)
{
    cyg_uint8 ch;
    CYGARC_HAL_SAVE_GP();

    while(!cyg_hal_plf_scif_getc_nonblock(__ch_data, &ch));

    CYGARC_HAL_RESTORE_GP();
    return ch;
}

void
cyg_hal_plf_scif_putc(void* __ch_data, cyg_uint8 c)
{
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    cyg_uint16 fdr, sr;
    CYGARC_HAL_SAVE_GP();

    do {
        HAL_READ_UINT16(base+_REG_SCFDR, fdr);
    } while (((fdr & CYGARC_REG_SCIF_SCFDR_TCOUNT_MASK) >> CYGARC_REG_SCIF_SCFDR_TCOUNT_shift) == 16);

    HAL_WRITE_UINT8(base+_REG_SCFTDR, c);

    // Clear FIFO-empty/transmit end flags (read back SR first)
    HAL_READ_UINT16(base+_REG_SCFSR, sr);
    HAL_WRITE_UINT16(base+_REG_SCFSR, CYGARC_REG_SCIF_SCSSR_CLEARMASK   
                     & ~(CYGARC_REG_SCIF_SCSSR_TDFE | CYGARC_REG_SCIF_SCSSR_TEND ));

    // Hang around until the character has been safely sent.
    do {
        HAL_READ_UINT16(base+_REG_SCFDR, fdr);
    } while ((fdr & CYGARC_REG_SCIF_SCFDR_TCOUNT_MASK) != 0);

    CYGARC_HAL_RESTORE_GP();
}


static channel_data_t channels[CYGNUM_HAL_SH_SH4_SCIF_PORTS];

static void
cyg_hal_plf_scif_write(void* __ch_data, const cyg_uint8* __buf, 
                         cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        cyg_hal_plf_scif_putc(__ch_data, *__buf++);

    CYGARC_HAL_RESTORE_GP();
}

static void
cyg_hal_plf_scif_read(void* __ch_data, cyg_uint8* __buf, cyg_uint32 __len)
{
    CYGARC_HAL_SAVE_GP();

    while(__len-- > 0)
        *__buf++ = cyg_hal_plf_scif_getc(__ch_data);

    CYGARC_HAL_RESTORE_GP();
}

cyg_bool
cyg_hal_plf_scif_getc_timeout(void* __ch_data, cyg_uint8* ch)
{
    channel_data_t* chan = (channel_data_t*)__ch_data;
    int delay_count;
    cyg_bool res;
    CYGARC_HAL_SAVE_GP();

    delay_count = chan->msec_timeout * 10; // delay in .1 ms steps

    for(;;) {
        res = cyg_hal_plf_scif_getc_nonblock(__ch_data, ch);
        if (res || 0 == delay_count--)
            break;
        
        CYGACC_CALL_IF_DELAY_US(100);
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}

static int
cyg_hal_plf_scif_control(void *__ch_data, __comm_control_cmd_t __func, ...)
{
    static int irq_state = 0;
    channel_data_t* chan = (channel_data_t*)__ch_data;
    cyg_uint8 scr;
    int ret = 0;
    CYGARC_HAL_SAVE_GP();

    switch (__func) {
    case __COMMCTL_IRQ_ENABLE:
        irq_state = 1;
        HAL_INTERRUPT_UNMASK(chan->isr_vector);
        HAL_READ_UINT16(chan->base+_REG_SCSCR, scr);
        scr |= CYGARC_REG_SCIF_SCSCR_RIE;
        HAL_WRITE_UINT16(chan->base+_REG_SCSCR, scr);
        break;
    case __COMMCTL_IRQ_DISABLE:
        ret = irq_state;
        irq_state = 0;
        HAL_INTERRUPT_UNMASK(chan->isr_vector);
        HAL_READ_UINT16(chan->base+_REG_SCSCR, scr);
        scr &= ~CYGARC_REG_SCIF_SCSCR_RIE;
        HAL_WRITE_UINT16(chan->base+_REG_SCSCR, scr);
        break;
    case __COMMCTL_DBG_ISR_VECTOR:
        ret = chan->isr_vector;
        break;
    case __COMMCTL_SET_TIMEOUT:
    {
        va_list ap;

        va_start(ap, __func);

        ret = chan->msec_timeout;
        chan->msec_timeout = va_arg(ap, cyg_uint32);

        va_end(ap);
    }
    break;
    case __COMMCTL_SETBAUD:
    {
        cyg_uint8* base = chan->base;
        va_list ap;

        va_start(ap, __func);
        chan->baud_rate = va_arg(ap, cyg_uint32);
        va_end(ap);

        // Set desired baudrate
        cyg_hal_plf_scif_set_baud(base, chan->baud_rate);
    }
    break;
    case __COMMCTL_GETBAUD:
        ret = chan->baud_rate;
        break;
    default:
        break;
    }
    CYGARC_HAL_RESTORE_GP();
    return ret;
}

static int
cyg_hal_plf_scif_isr(void *__ch_data, int* __ctrlc, 
                     CYG_ADDRWORD __vector, CYG_ADDRWORD __data)
{
    cyg_uint8 c;
    cyg_uint16 fdr, sr;
    cyg_uint8* base = ((channel_data_t*)__ch_data)->base;
    int res = 0;
    CYGARC_HAL_SAVE_GP();

    *__ctrlc = 0;
    HAL_READ_UINT16(base+_REG_SCFDR, fdr);
    if ((fdr & CYGARC_REG_SCIF_SCFDR_RCOUNT_MASK) != 0) {
        HAL_READ_UINT8(base+_REG_SCFRDR, c);

        // Clear buffer full flag (read back first).
        HAL_READ_UINT16(base+_REG_SCFSR, sr);
        HAL_WRITE_UINT16(base+_REG_SCFSR, 
                         CYGARC_REG_SCIF_SCSSR_CLEARMASK & ~CYGARC_REG_SCIF_SCSSR_RDF);

        if( cyg_hal_is_break( &c , 1 ) )
            *__ctrlc = 1;

        res = CYG_ISR_HANDLED;
    }

    CYGARC_HAL_RESTORE_GP();
    return res;
}

void
cyg_hal_plf_scif_init(int scif_index, int comm_index, 
                      int rcv_vect, cyg_uint8* base)
{
    channel_data_t* chan = &channels[scif_index];
    hal_virtual_comm_table_t* comm;
    int cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);

    // Initialize channel table
    chan->base = base;
    chan->isr_vector = rcv_vect;
    chan->msec_timeout = 1000;

    // Disable interrupts.
    HAL_INTERRUPT_MASK(chan->isr_vector);

    // Init channel
    cyg_hal_plf_scif_init_channel(chan);

    // Setup procs in the vector table

    // Initialize channel procs
    CYGACC_CALL_IF_SET_CONSOLE_COMM(comm_index);
    comm = CYGACC_CALL_IF_CONSOLE_PROCS();
    CYGACC_COMM_IF_CH_DATA_SET(*comm, chan);
    CYGACC_COMM_IF_WRITE_SET(*comm, cyg_hal_plf_scif_write);
    CYGACC_COMM_IF_READ_SET(*comm, cyg_hal_plf_scif_read);
    CYGACC_COMM_IF_PUTC_SET(*comm, cyg_hal_plf_scif_putc);
    CYGACC_COMM_IF_GETC_SET(*comm, cyg_hal_plf_scif_getc);
    CYGACC_COMM_IF_CONTROL_SET(*comm, cyg_hal_plf_scif_control);
    CYGACC_COMM_IF_DBG_ISR_SET(*comm, cyg_hal_plf_scif_isr);
    CYGACC_COMM_IF_GETC_TIMEOUT_SET(*comm, cyg_hal_plf_scif_getc_timeout);

    // Restore original console
    CYGACC_CALL_IF_SET_CONSOLE_COMM(cur);
}

#endif // CYGNUM_HAL_SH_SH4_SCIF_PORTS

//-----------------------------------------------------------------------------
// end of sh4_scif.c
