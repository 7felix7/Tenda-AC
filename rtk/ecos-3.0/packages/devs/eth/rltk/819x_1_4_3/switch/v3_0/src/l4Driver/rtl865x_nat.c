/*
* Copyright c                  Realtek Semiconductor Corporation, 2008  
* All rights reserved.
* 
* Program : napt table driver
* Abstract : 
* Author : hyking (hyking_liu@realsil.com.cn)  
*/

/*      @doc RTL_LAYEREDDRV_API

        @module rtl865x_nat.c - RTL865x Home gateway controller Layered driver API documentation       |
        This document explains the API interface of the table driver module. Functions with rtl865x prefix
        are external functions.
        @normal Hyking Liu (Hyking_liu@realsil.com.cn) <date>

        Copyright <cp>2008 Realtek<tm> Semiconductor Cooperation, All Rights Reserved.

        @head3 List of Symbols |
        Here is a list of all functions and variables in this module.
        
        @index | RTL_LAYEREDDRV_API
*/

#include <cyg/io/eth/rltk/819x/wrapper/sys_support.h>
#include <cyg/io/eth/rltk/819x/wrapper/timer.h>
#include "../rtl_types.h"
#include "../rtl865xC_tblAsicDrv.h"
#include <switch/rtl865x_netif.h>
#include "common/rtl865x_netif_local.h"
#include "common/rtl865x_eventMgr.h"
#include <switch/rtl_glue.h>
#include <rtl/rtl865x_eventMgr.h>
//#include "rtl865x_ppp_local.h"
//#include "rtl865x_route.h"
#include "l2Driver/rtl865x_fdb.h"
#include <switch/rtl865x_fdb_api.h>
#include "l3Driver/rtl865x_ip.h"
#include <switch/rtl865x_nat.h>
#include "rtl865x_nat_local.h"
#include <netinet/in.h>
//#include <netinet/fastpath/rtl_alias.h>
#include <rtl/rtl_alias.h>

#include "rtl865x_asicL4.h"

int gHwNatEnabled = 1;
extern int32 rtl_fpAddConnCheck(struct alias_link *link, struct ip *iph);
extern unsigned long rtl_getTcpExpireByState(int state_in, int state_out);

#if defined(CONFIG_RTL_AVOID_ADDING_WLAN_PKT_TO_HW_NAT)
extern int32 rtl865x_isEthArp(ipaddr_t ip);
#endif


static struct nat_table nat_tbl;
static int32 rtl865x_enableNaptFourWay=FALSE;

#define	RTL_NAT_AVG_INTERVAL	3
#define	RTL_NAT_MIN_INTERVAL	1

int32 rtl_nat_expire_interval_update(int proto, int32 interval)
{
	int32	asic_interval;

	asic_interval = interval>RTL_NAT_AVG_INTERVAL?interval-RTL_NAT_AVG_INTERVAL:interval-RTL_NAT_MIN_INTERVAL;
	asic_interval = asic_interval<RTL_NAT_MIN_INTERVAL?RTL_NAT_MIN_INTERVAL:asic_interval;
	
	if (proto==RTL865X_PROTOCOL_UDP) {
		nat_tbl.tcp_timeout = asic_interval;
	}else if (proto==RTL865X_PROTOCOL_UDP) {
		nat_tbl.udp_timeout = asic_interval;
	} else {
		return FAILED;
	}

	/* Set ASIC timeout value */
	rtl8651_setAsicNaptTcpLongTimeout(nat_tbl.tcp_timeout);
	rtl8651_setAsicNaptTcpMediumTimeout(nat_tbl.tcp_timeout);
	rtl8651_setAsicNaptTcpFastTimeout(nat_tbl.tcp_timeout);
	rtl8651_setAsicNaptUdpTimeout(nat_tbl.udp_timeout);

	return SUCCESS;
}

static int32 _rtl865x_nat_init(void)
{
	rtl865x_tblAsicDrv_naptTcpUdpParam_t naptTcpUdp;
	uint32 flowTblIdx;
	
	memset(nat_tbl.nat_bucket, 0, 
		sizeof(struct nat_entry)*RTL8651_TCPUDPTBL_SIZE);

	nat_tbl.connNum = 0;
	nat_tbl.tcp_timeout = TCP_TIMEOUT; //24*60*60;
	nat_tbl.udp_timeout = UDP_TIMEOUT; //60*15;
	nat_tbl.freeEntryNum=RTL8651_TCPUDPTBL_SIZE;

	/* Set ASIC timeout value */
	rtl8651_setAsicNaptTcpLongTimeout(TCP_TIMEOUT);
	rtl8651_setAsicNaptTcpMediumTimeout(TCP_TIMEOUT);
	rtl8651_setAsicNaptTcpFastTimeout(TCP_TIMEOUT);
	rtl8651_setAsicNaptUdpTimeout(UDP_TIMEOUT);

	/*enable 865xC enhanced hash1*/
	_rtl8651_enableEnhancedHash1();
	
	/* Initial ASIC NAT Table */
	memset( &naptTcpUdp, 0, sizeof(naptTcpUdp) );
	naptTcpUdp.isCollision = 1;
	naptTcpUdp.isCollision2 = 1;
	for(flowTblIdx=0; flowTblIdx<RTL8651_TCPUDPTBL_SIZE; flowTblIdx++)
		rtl8651_setAsicNaptTcpUdpTable(TRUE, flowTblIdx, &naptTcpUdp );

	//rtl865x_nat_register_event();
		
	return SUCCESS;
}



static struct nat_entry * _rtl865x_nat_outbound_lookup(struct nat_tuple *nat_tuple)
{
	struct nat_entry *nat_out;
	uint32 i,hash;

	#if defined(CONFIG_RTL_REDUCE_MEMORY)
	hash = rtl8651_naptTcpUdpTableIndex((uint8)nat_tuple->proto, nat_tuple->in_host_ip, nat_tuple->in_host_port, 
										nat_tuple->rem_host_ip, nat_tuple->rem_host_port);
	#else
	hash = rtl8651_naptTcpUdpTableIndex((uint8)nat_tuple->proto, nat_tuple->int_host.ip, nat_tuple->int_host.port, 
										nat_tuple->rem_host.ip, nat_tuple->rem_host.port);
	#endif
	if(rtl865x_enableNaptFourWay==TRUE)
	{
		for(i=0; i<4; i++)
		{
			nat_out = &nat_tbl.nat_bucket[hash];
			if (!memcmp(nat_out, nat_tuple, sizeof(*nat_tuple)) &&
				(nat_out->flags&NAT_OUTBOUND))
			{
				return nat_out;
			}
			hash=(hash&0xFFFFFFFC)+(hash+1)%4;
			ASSERT(hash<=RTL8651_TCPUDPTBL_SIZE);
		}
	}
	else
	{
		nat_out = &nat_tbl.nat_bucket[hash];
		if (!memcmp(nat_out, nat_tuple, sizeof(*nat_tuple)) &&
			(nat_out->flags&NAT_OUTBOUND))
			return nat_out;
	}
	return (struct nat_entry *)0;
}

static struct nat_entry * _rtl865x_nat_inbound_lookup(struct nat_tuple *nat_tuple)
{
	struct nat_entry *nat_in;
	uint32 hash;

	#if defined(CONFIG_RTL_REDUCE_MEMORY)
	hash = rtl8651_naptTcpUdpTableIndex((uint8)nat_tuple->proto, nat_tuple->rem_host_ip, nat_tuple->rem_host_port, 
											nat_tuple->ext_host_ip, nat_tuple->ext_host_port);
	#else
	hash = rtl8651_naptTcpUdpTableIndex((uint8)nat_tuple->proto, nat_tuple->rem_host.ip, nat_tuple->rem_host.port, 
											nat_tuple->ext_host.ip, nat_tuple->ext_host.port);
	#endif
	
	nat_in = &nat_tbl.nat_bucket[hash];
	if (!memcmp(nat_in, nat_tuple, sizeof(*nat_tuple)) && 
		(nat_in->flags&NAT_INBOUND))
	{
			return nat_in;
	}
	
	return (struct nat_entry *)0;
}

#if defined (CONFIG_RTL_INBOUND_COLLISION_AVOIDANCE)
static int _rtl865x_isEntryPreReserved(uint32 index)
{
	unsigned long now = 0;
	struct nat_entry *natEntry;
	if(index>=RTL8651_TCPUDPTBL_SIZE)
	{
		return FALSE;
	}
	
	natEntry= &nat_tbl.nat_bucket[index];

	now = (unsigned long)jiffies;
	if((natEntry->flags & NAT_PRE_RESERVED))
	{
		if(now>=natEntry->reserveTime)
		{
			if(now>(natEntry->reserveTime+RESERVE_EXPIRE_TIME*HZ))
			{
				/*pre-reserve become invalid now*/
				natEntry->flags &= (~(NAT_PRE_RESERVED));
				natEntry->reserveTime=0;
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
		else
		{
			/*timer overflow*/
			if(((0xFFFFFFFF-natEntry->reserveTime)+now+1)>(RESERVE_EXPIRE_TIME*HZ))
			{
				natEntry->flags &= (~(NAT_PRE_RESERVED));
				natEntry->reserveTime=0;
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
		return TRUE;
	}

	return FALSE;
}

static int _rtl865x_PreReserveEntry(uint32 index)
{
	struct nat_entry *natEntry;
	if(index>=RTL8651_TCPUDPTBL_SIZE)
	{
		return FAILED;
	}
	natEntry= &nat_tbl.nat_bucket[index];
	
	if(NAT_INUSE(natEntry))
	{
		/*already used by other napt connection, can not reserve it*/
		natEntry->flags &= (~(NAT_PRE_RESERVED));
		natEntry->reserveTime=0;
	}
	else
	{
		natEntry->flags|=NAT_PRE_RESERVED;
		natEntry->reserveTime=(unsigned long)jiffies;
	}
	return SUCCESS;
}

static int _rtl865x_getNaptHashInfo( rtl865x_napt_entry *naptEntry, 
                        rtl865x_naptHashInfo_t *naptHashInfo)
{
	
	uint32 in, out;
	uint32  i,index;
	struct nat_entry *nat_in, *nat_out;
	struct nat_entry *natEntry;
	
	if(naptHashInfo==NULL)
	{
		return FAILED;
	}
	
	memset(naptHashInfo, 0, sizeof(rtl865x_naptHashInfo_t));
	
	in = rtl8651_naptTcpUdpTableIndex((uint8)(naptEntry->protocol), naptEntry->remIp, naptEntry->remPort, naptEntry->extIp, naptEntry->extPort);
	out = rtl8651_naptTcpUdpTableIndex((uint8)(naptEntry->protocol), naptEntry->intIp, naptEntry->intPort, naptEntry->remIp, naptEntry->remPort);

	if(rtl865x_enableNaptFourWay==TRUE)
	{
		uint32 outAvailIdx=0xFFFFFFFF;
		index=out;
		for(i=0;i<4;i++)
		{
			natEntry = &nat_tbl.nat_bucket[index];
			if (NAT_INUSE(natEntry) || _rtl865x_isEntryPreReserved(index))
			{
			
			}
			else
			{
				if(index==in)
				{
					/*collide with inbound*/
				}
				else
				{
					out=index;
					break;
				}
			}
			index=(index&0xFFFFFFFC)+(index+1)%4;
			ASSERT(index<=RTL8651_TCPUDPTBL_SIZE);
				
		}

		if(i>=4)
		{
			/*only one empty entry, but collide with its own inbound*/
			if(outAvailIdx!=0xFFFFFFFF)
			{
				out=outAvailIdx;
			}
		}
		else
		{
			/*proper empty entry has been found*/
		}
	}
	
	naptHashInfo->outIndex=out;
	naptHashInfo->inIndex=in;

	if((in&0xFFFFFFFC)==(out&0xFFFFFFFC))
	{
		naptHashInfo->sameFourWay=1;
	}	

	if(in==out)
	{
		naptHashInfo->sameLocation=1;
		
		nat_out = &nat_tbl.nat_bucket[out];
		if(NAT_INUSE(nat_out)|| _rtl865x_isEntryPreReserved(out))
		{
			naptHashInfo->outCollision=1;
		}

		naptHashInfo->inCollision=1;
	}
	else
	{
		nat_out = &nat_tbl.nat_bucket[out];
		nat_in = &nat_tbl.nat_bucket[in];
		
		if(NAT_INUSE(nat_out) || _rtl865x_isEntryPreReserved(out))
		{
			naptHashInfo->outCollision=1;
		}
		
		if (NAT_INUSE(nat_in) ||  _rtl865x_isEntryPreReserved(in))
		{
			naptHashInfo->inCollision=1;
		}
	}
	

	index=in;
	naptHashInfo->inFreeCnt=0;
	for(i=0;i<4;i++)
	{
		natEntry = &nat_tbl.nat_bucket[index];
		if (NAT_INUSE(natEntry) || _rtl865x_isEntryPreReserved(index))
		{
		
		}
		else
		{
			naptHashInfo->inFreeCnt++;
		}
		index=(index&0xFFFFFFFC)+(index+1)%4;
		ASSERT(index<=RTL8651_TCPUDPTBL_SIZE);
	}
	#if 0
	printk("%s:%d:%s (%u.%u.%u.%u:%u -> %u.%u.%u.%u:%u -> %u.%u.%u.%u:%u) ,out is %d,in is %d\n",
			__FUNCTION__,__LINE__,protocol?"tcp":"udp", 
			NIPQUAD(intIp), intPort, NIPQUAD(extIp), extPort, NIPQUAD(remIp), remPort, out, in);	
	#endif
	return SUCCESS;
}

int rtl865x_optimizeExtPort(unsigned short origDelta, unsigned int rangeSize, unsigned short *newDelta)
{
	int i;
	int msb;
	unsigned int bitShift;

	msb=0;
	for(i=0;i<16;i++)
	{
		if((1<<i) & rangeSize)
		{
			msb=i;
		}
	}

	if(((1<<msb)+1)>rangeSize)
	{
		if(msb>1)
		{
			msb--;
		}
	}
		
 	*newDelta=0;
	if(msb<10)
	{
		bitShift=0x01;
		for(i=0;i<=msb;i++)
		{
			if(i==0)/*bit0 keep the same*/
			{
				if(origDelta&bitShift)
				{
					 *newDelta|=bitShift;
				}
			}
			else /*original bit1~ bit_maxPower mapped to bit_maxPower~bit1*/
			{
				if(origDelta&bitShift) 
				{
					 *newDelta |=(0x1<<(msb+1-i));
				}
			}

			bitShift=bitShift<<1;
		}
	}
	else
	{
		bitShift=0x01;
		
		for(i=0;i<=msb;i++)
		{
			if(i==0)	/*bit0 keep the same*/
			{
				if(origDelta&bitShift) 
				{
					*newDelta |=bitShift;
				}
			}
			else if (i<10) /*bit1~ bit9 mapped to bit 9~bit1*/
			{
				if(origDelta&bitShift) 
				{
					*newDelta  |=(0x1<<(10-i));
				}
			}
			else/*other bits keep the same*/
			{
				if(origDelta&bitShift) 
				{
					*newDelta  |=bitShift;
				}
			}

			bitShift=bitShift<<1;
		}


	}
	return SUCCESS;
}

int rtl865x_getAsicNaptHashScore(rtl865x_napt_entry *naptEntry, 
					                        uint32 *naptHashScore)
{
	 rtl865x_naptHashInfo_t naptHashInfo;
	_rtl865x_getNaptHashInfo(naptEntry, &naptHashInfo);

	/*initialize napt hash score*/
	*naptHashScore=100;

	/*note:we can not change outbound index*/
	
	if(naptHashInfo.inCollision==FALSE)
	{
		if(naptHashInfo.inFreeCnt==4)
		{
			if(!naptHashInfo.sameFourWay)
			{
				*naptHashScore=100;
			}
			else
			{
				if(!naptHashInfo.sameLocation)
				{
					*naptHashScore=80;
				}
				else
				{
					*naptHashScore=0;
				}
			}
		}
		else if (naptHashInfo.inFreeCnt==3)
		{
			if(!naptHashInfo.sameFourWay)
			{
				*naptHashScore=80;
			}
			else
			{
				if(!naptHashInfo.sameLocation)
				{
					*naptHashScore=70;
				}
				else
				{
					*naptHashScore=0;
				}
			}
		}
		else if (naptHashInfo.inFreeCnt==2)
		{
			if(!naptHashInfo.sameFourWay)
			{
				*naptHashScore=70;
			}
			else
			{
				if(!naptHashInfo.sameLocation==FALSE)
				{
					*naptHashScore=60;
				}
				else
				{
					*naptHashScore=0;
				}
			}
		}
		else if (naptHashInfo.inFreeCnt==1)
		{
			if(naptHashInfo.sameFourWay==FALSE)
			{
				*naptHashScore=60;
			}
			else
			{
				*naptHashScore=0;
					
			}
		}
		else
		{
			*naptHashScore=0;
		}
		

	}
	else
	{
		/*worst case:inbound is collision*/
		*naptHashScore=0;
	}
	
	return SUCCESS;

}

int32 rtl865x_preReserveConn(rtl865x_napt_entry *naptEntry)
{

	 rtl865x_naptHashInfo_t naptHashInfo;
	_rtl865x_getNaptHashInfo(naptEntry, &naptHashInfo);

	if(naptHashInfo.outCollision==FALSE)
	{
		_rtl865x_PreReserveEntry(naptHashInfo.outIndex);
	}

	if(naptHashInfo.inCollision==FALSE)
	{
		_rtl865x_PreReserveEntry(naptHashInfo.inIndex);
	}
	
	return SUCCESS;
}	

#endif

static int32 _rtl865x_addNaptConnection(rtl865x_napt_entry *naptEntry, rtl865x_priority *prio)
{
	int32 retval;
	rtl865x_tblAsicDrv_naptTcpUdpParam_t asic_nat;
	struct nat_entry *nat_in, *nat_out;
	struct nat_tuple nat_tuple;
	uint32 in, out, offset, ipidx, i;
	uint16 very,selEidx_out;
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT) 
	#if defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)
	rtl865x_route_t		rt;
	rtl865x_arpMapping_entry_t	arpInfo;
	ipaddr_t		sip, dip;
	uint16		sport, dport;
	rtl865x_AclRule_t		rule4[2], rule2[2];
	int32		defPriority[2];
	int32 		upDown[2], defUpDown[2];//0: uplink; 1: downlink
	#endif
	int32		priority[2];
#endif
	//int32		elasped, del_conn_flags;
	
	uint32 outCollision=FALSE;
	uint32 inCollision=FALSE;

	/* Make sure natip */
	retval = rtl865x_getIpIdxByExtIp(naptEntry->extIp, &ipidx);
	if(retval != SUCCESS)
		return TBLDRV_EINVALIDINPUT;
	
	memset(&nat_tuple, 0, sizeof(struct nat_tuple));
	#if defined(CONFIG_RTL_REDUCE_MEMORY)
	nat_tuple.in_host_ip			= naptEntry->intIp;
	nat_tuple.in_host_port			= naptEntry->intPort;
	nat_tuple.ext_host_ip			= naptEntry->extIp;
	nat_tuple.ext_host_port			= naptEntry->extPort;
	nat_tuple.rem_host_ip			= naptEntry->remIp;
	nat_tuple.rem_host_port		= naptEntry->remPort;
	nat_tuple.proto				= naptEntry->protocol;
	#else
	nat_tuple.int_host.ip			= naptEntry->intIp;
	nat_tuple.int_host.port			= naptEntry->intPort;
	nat_tuple.ext_host.ip			= naptEntry->extIp;
	nat_tuple.ext_host.port			= naptEntry->extPort;
	nat_tuple.rem_host.ip			= naptEntry->remIp;
	nat_tuple.rem_host.port		= naptEntry->remPort;
	nat_tuple.proto				= naptEntry->protocol;
	#endif
	
	nat_out = _rtl865x_nat_outbound_lookup(&nat_tuple);
	if (nat_out)
	{
		return TBLDRV_EENTRYALREADYEXIST;
	}
	
	nat_in = _rtl865x_nat_inbound_lookup(&nat_tuple);
	if(nat_in)
	{
		return TBLDRV_EENTRYALREADYEXIST;
	}

	offset = (naptEntry->extPort&0x0000ffff)>>10;
	very = rtl8651_naptTcpUdpTableIndex(((uint8)naptEntry->protocol) |HASH_FOR_VERI , naptEntry->remIp, naptEntry->remPort, 0, 0);
	
	selEidx_out = naptEntry->extPort&0x3ff;
	in = rtl8651_naptTcpUdpTableIndex((uint8)naptEntry->protocol, naptEntry->remIp, naptEntry->remPort, naptEntry->extIp, naptEntry->extPort);
	out = rtl8651_naptTcpUdpTableIndex((uint8)naptEntry->protocol, naptEntry->intIp, naptEntry->intPort, naptEntry->remIp, naptEntry->remPort);
	/*support outbound 4-way*/
	if(rtl865x_enableNaptFourWay==TRUE)
	{

		uint32 hash=out;
		uint32 outAvailIdx=0xFFFFFFFF;

		for(i=0;i<4;i++)
		{
			nat_out = &nat_tbl.nat_bucket[hash];
			if (NAT_INUSE(nat_out))
			{
				/* collision with outbound */
			}
			else
			{
				outAvailIdx=hash;
				if(hash==in)
				{
					/*collision with inbound*/
				}
				else
				{
					out=hash;
					break;
				}
			}
			hash=(hash&0xFFFFFFFC)+(hash+1)%4;
			ASSERT(hash<=RTL8651_TCPUDPTBL_SIZE);
				
		}


		if(i>=4)
		{
			/*only one empty entry,but collide with its own inbound*/
			if(outAvailIdx!=0xFFFFFFFF)
			{
				out=outAvailIdx;
			}
		}
	}

	
	nat_out = &nat_tbl.nat_bucket[out];
	nat_in = &nat_tbl.nat_bucket[in];
	#if 0
	del_conn_flags = 0;
	if (NAT_INUSE(nat_out))
	{
		elasped = _rtl865x_naptIdleCheck(out);
		if ( ((nat_out->tuple_info.proto==RTL865X_PROTOCOL_UDP)&&(elasped>UDP_OVERRIDE_ELASPED_THRESHOLD)) 
			|| ((nat_out->tuple_info.proto==RTL865X_PROTOCOL_TCP)&&(elasped>TCP_OVERRIDE_ELASPED_THRESHOLD)))
		{
			NAT_INVALID(nat_out);		/*	invalide	*/
			nat_tbl.freeEntryNum++;
			del_conn_flags++;
		}
	}

	if (NAT_INUSE(nat_in))
	{
		elasped = _rtl865x_naptIdleCheck(in);
		if ( ((nat_out->tuple_info.proto==RTL865X_PROTOCOL_UDP)&&(elasped>UDP_OVERRIDE_ELASPED_THRESHOLD)) 
			|| ((nat_out->tuple_info.proto==RTL865X_PROTOCOL_TCP)&&(elasped>TCP_OVERRIDE_ELASPED_THRESHOLD)))
		{
			NAT_INVALID(nat_in);		/*	invalide	*/
			nat_tbl.freeEntryNum++;
			del_conn_flags++;
		}
	}

	if (del_conn_flags==2) {
		nat_tbl.connNum--;
	}
	#endif
	if ( NAT_INUSE(nat_out) && NAT_INUSE(nat_in))
	{
		/*both outbound and inbound has been occupied*/
		return TBLDRV_EINVALIDINPUT;
	}
	
	if(out==in)
	{
		outCollision=FALSE;
		inCollision=TRUE;
		/*we don't support this case at present, otherwise, when delete napt connection must be very very careful*/
		//return RTL_EENTRYALREADYEXIST;
	}
	else
	{
		if (NAT_INUSE(nat_out))
		{
			outCollision=TRUE;
		}

		if(NAT_INUSE(nat_in))
		{
			inCollision=TRUE;
		}
	}
#ifdef CONFIG_HARDWARE_NAT_DEBUG
	rtlglue_printf("LR(%s):  %s (%u.%u.%u.%u:%u -> %u.%u.%u.%u:%u) g:(%u.%u.%u.%u:%u)\n",
			("add_nat"), ((naptEntry->protocol)? "tcp": "udp"), 
			NIPQUAD(naptEntry->intIp), naptEntry->intPort, NIPQUAD(naptEntry->remIp), naptEntry->remPort, NIPQUAD(naptEntry->extIp), naptEntry->extPort);
#endif

#if defined (CONFIG_RTL_HALF_NAPT_ACCELERATION)

#else
	if((outCollision==TRUE) || (inCollision==TRUE))
	{
		/*we must make sure both direction can be written into asic*/
		return TBLDRV_EINVALIDINPUT;
	}
#endif

	if ( outCollision==FALSE)
	{
		memset(nat_out, 0, sizeof(struct nat_entry));
		*((struct nat_tuple *)nat_out)=nat_tuple;
		nat_out->out=out;
		if( inCollision==FALSE)
		{
			nat_out->in=in;
		}
		else
		{
			nat_out->in=0xFFFFFFFF;
		}
		#if defined(CONFIG_RTL_REDUCE_MEMORY)
		nat_out->natip_idx	=(uint8)ipidx;	
		#else
		nat_out->natip_idx	=ipidx;	
		#endif
		SET_NAT_FLAGS(nat_out, NAT_OUTBOUND);
	}
	else
	{
		nat_out=NULL;
	}

	if( inCollision==FALSE)
	{
		memset(nat_in, 0, sizeof(struct nat_entry));
		*((struct nat_tuple *)nat_in) = nat_tuple;
		nat_in->in = in;
		if ( outCollision==FALSE)
		{
			 nat_in->out = out;
		}
		else
		{
			 nat_in->out=0xFFFFFFFF;
		}
		#if defined(CONFIG_RTL_REDUCE_MEMORY)
		nat_in->natip_idx	=(uint8)ipidx;	
		#else
		nat_in->natip_idx = ipidx;
		#endif
		SET_NAT_FLAGS(nat_in, NAT_INBOUND);
	}
	else
	{
		nat_in=NULL;
	}

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
#if	defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)
	sip = naptEntry->intIp;
	dip = naptEntry->remIp;
	sport = naptEntry->intPort;
	dport = naptEntry->remPort;
	
	//Initial
	for(i=0;i<2;i++)
	{
		priority[i] = 0;
		defPriority[i]=-1;
		upDown[i]=-1;
		defUpDown[i]=-1;
	}
	
	for(i=0;i<2;i++)
	{
		if (rtl865x_getRouteEntry(sip, &rt)==SUCCESS)
		{
			/*	check ip base rule firstly	*/

			memset(&rule4[i], 0, sizeof(rtl865x_AclRule_t));
			rule4[i].ruleType_ = (naptEntry->protocol==RTL865X_PROTOCOL_TCP?RTL865X_ACL_TCP:RTL865X_ACL_UDP);
			rule4[i].srcIpAddr_ = sip;
			rule4[i].dstIpAddr_ = dip;
			rule4[i].tcpSrcPortLB_ = sport;
			rule4[i].tcpDstPortLB_ = dport;
			rule4[i].netifIdx_ = _rtl865x_getNetifIdxByNameExt(rt.dstNetif->name);
			
			if (rtl865x_qosCheckNaptPriority(&rule4[i])==SUCCESS)
			{
				priority[i] = rule4[i].priority_;		/* matched priority	*/
				upDown[i]=rule4[i].upDown_;
				//break;

				if (i==0)
				{
					sip = naptEntry->remIp;
					dip = naptEntry->intIp;
					sport = naptEntry->remPort;
					dport = naptEntry->intPort;
					continue;
				}
				 
			}
			else if (i==0)
			{
				sip = naptEntry->remIp;
				dip = naptEntry->intIp;
				sport = naptEntry->remPort;
				dport = naptEntry->intPort;
				continue;
			}
			else
			{
				defPriority[i] = rule4[i].priority_;
				defUpDown[i]=rule4[i].upDown_;
			}
		}
		else
		{
			sip = naptEntry->remIp;
			dip = naptEntry->intIp;
			sport = naptEntry->remPort;
			dport = naptEntry->intPort;
		}
	}

	{
		sip = naptEntry->intIp;
		for(i=0;i<2;i++)
		{
			if (rtl865x_getArpMapping(sip, &arpInfo)==SUCCESS && rtl865x_getRouteEntry(sip, &rt)==SUCCESS)
			{
				/*	check mac base rule secondly	*/
				memset(&rule2[i], 0, sizeof(rtl865x_AclRule_t));
				rule2[i].ruleType_ = RTL865X_ACL_MAC;
				memcpy(rule2[i].srcMac_.octet, arpInfo.mac.octet, ETHER_ADDR_LEN);
				memset(rule2[i].srcMacMask_.octet, 0xff, ETHER_ADDR_LEN);
				rule2[i].netifIdx_ = _rtl865x_getNetifIdxByNameExt(rt.dstNetif->name);

				if (rtl865x_qosCheckNaptPriority(&rule2[i])==SUCCESS)
				{
					priority[i] = rule2[i].priority_;		/* matched priority	*/
					upDown[i]=rule2[i].upDown_;
					//break;

					 if(i==0)
					{
						sip = naptEntry->remIp;
						continue;
					}
					 
				}
				else if(i==0)
				{
					sip = naptEntry->remIp;
					continue;
				}
				else
				{
					dip = naptEntry->remIp;
					for(i=0;i<2;i++)
					{
						if (rtl865x_getArpMapping(dip, &arpInfo)==SUCCESS)
						{
							/*	check mac base rule secondly	*/
							memset(&rule2[i], 0, sizeof(rtl865x_AclRule_t));
							rule2[i].ruleType_ = RTL865X_ACL_MAC;
							memcpy(rule2[i].dstMac_.octet, arpInfo.mac.octet, ETHER_ADDR_LEN);
							memset(rule2[i].dstMacMask_.octet, 0xff, ETHER_ADDR_LEN);
							rule2[i].netifIdx_ = _rtl865x_getNetifIdxByNameExt(rt.dstNetif->name);

							if (rtl865x_qosCheckNaptPriority(&rule2[i])==SUCCESS)
							{
								priority[i] = rule2[i].priority_;		/* matched priority	*/
								upDown[i]=rule2[i].upDown_;
								//break;
								
								if(i==0)
								{
									dip = naptEntry->intIp;
									continue;
								}
								
							}
							else if(i==0)
							{
								dip = naptEntry->intIp;
								continue;
							}
							else
							{
								defPriority[i] = rule2[i].priority_;
								defUpDown[i]=rule2[i].upDown_;
							}
						}
						else
						{
							dip = naptEntry->intIp;
						}
					}
				}
			}
			else
			{
				sip = naptEntry->remIp;
			}
		}
	}

	for(i=0;i<2;i++)
	{
		if (rule4[i].aclIdx&&rule2[i].aclIdx)
		{
			priority[i] = (rule4[i].aclIdx<rule2[i].aclIdx)?rule4[i].priority_:rule2[i].priority_;
			upDown[i]=(rule4[i].aclIdx<rule2[i].aclIdx)?rule4[i].upDown_:rule2[i].upDown_;
		}
		else if (rule4[i].aclIdx)
		{
			priority[i] = rule4[i].priority_;
			upDown[i]=rule4[i].upDown_;
		}
		else if (rule2[i].aclIdx)
		{
			priority[i] = rule2[i].priority_;
			upDown[i]=rule2[i].upDown_;
		}
		else if (defPriority[i]>-1)
		{
			priority[i] = defPriority[i];
			upDown[i]=defUpDown[i];
		}
	}
#else	/*	defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)	*/
	priority[0]=prio->uplinkPrio;
	priority[1]=prio->downlinkPrio;
#endif	/*	defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)	*/
#endif	/*	defined(CONFIG_RTL_HW_QOS_SUPPORT)		*/

	for(i=0; i<2; i++) {
		
		if(i==0)
		{
			/*check writing outbound connection into asic*/
			if((outCollision==TRUE) || (nat_out==NULL))
			{
				/*shouldn't be written into asic*/
				continue;
			}
		}
		else if(i==1)
		{
			/*check writing inbound connection into asic*/
			if((inCollision==TRUE)||(nat_in==NULL))
			{
				/*shouldn't be written into asic*/
				continue;
			}
		}
		else
		{
			break;
		}
		//If qos enabled, not add inbound napt
		//That is all downlink pkt will be trapped to CPU for software QoS
		#if 0
		/* drop pkt in the queue will resolve the wan->lan issue */
		if((flags==FLAG_QOS_ENABLE)&&(i==1))
			break;
		#endif
		
		memset(&asic_nat, 0, sizeof(asic_nat));
		asic_nat.insideLocalIpAddr	= naptEntry->intIp;
		asic_nat.insideLocalPort		= naptEntry->intPort;
		asic_nat.isCollision			= 0;
		asic_nat.isCollision2		= 0;
		asic_nat.isDedicated		= 0;
		asic_nat.isStatic			= 1;
		asic_nat.isTcp			= (naptEntry->protocol==RTL865X_PROTOCOL_TCP)? 1: 0;
		asic_nat.isValid			= 1;
		asic_nat.offset			= ((i==0)?offset : (naptEntry->extPort & 0x3f));
		asic_nat.selEIdx			= ((i==0)?selEidx_out: very &0x3ff);
		asic_nat.selExtIPIdx		= ((i==0)?ipidx:((naptEntry->extPort & 0x3ff) >> 6));
		//asic_nat.tcpFlag			= (((in!=out)? 0x2:0x0) | ((i==0)? 1: 0));
		/*enhanced hash1 doesn't support outbound/inbound share one connection*/
		asic_nat.tcpFlag			= (0x2 | ((i==0)? 1: 0));
		asic_nat.ageSec			= (naptEntry->protocol==RTL865X_PROTOCOL_TCP)? nat_tbl.tcp_timeout:nat_tbl.udp_timeout;
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT)
		asic_nat.priValid			=	FALSE;
		#if defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)
		if(i==0&&upDown[i]==0)//Because ((i==0)?out: in)
		{
			//out: uplink
			asic_nat.priority			=	priority[i];
		}
		if (i==1&&upDown[i]==1)
		{
			//in: downlink
			asic_nat.priority			=	priority[i];
		}
		#else	/*	defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)	*/
		asic_nat.priority			=	priority[i];
		#endif	/*	defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)	*/
		asic_nat.priValid			=	TRUE;
#else
		asic_nat.priValid			=	FALSE;
#endif
		
		rtl8651_setAsicNaptTcpUdpTable(1, ((i==0)?out: in), &asic_nat);
	}


	nat_tbl.connNum++;
	
	if((outCollision==FALSE) && (nat_out!=NULL))
	{
		if(nat_tbl.freeEntryNum>0)
		{
			nat_tbl.freeEntryNum--;
		}
	}
		
	if((inCollision==FALSE) && (nat_in!=NULL))
	{
		if(nat_tbl.freeEntryNum>0)
		{
			nat_tbl.freeEntryNum--;
		}
	}
	
	return SUCCESS;
}


static int32 _rtl865x_delNaptConnection(rtl865x_napt_entry *naptEntry)
{
	struct nat_entry *nat_out, *nat_in;
	struct nat_tuple nat_tuple;

	memset(&nat_tuple, 0, sizeof(struct nat_tuple));
	#if defined(CONFIG_RTL_REDUCE_MEMORY)
	nat_tuple.in_host_ip			= naptEntry->intIp;
	nat_tuple.in_host_port			= naptEntry->intPort;
	nat_tuple.ext_host_ip			= naptEntry->extIp;
	nat_tuple.ext_host_port			= naptEntry->extPort;
	nat_tuple.rem_host_ip			= naptEntry->remIp;
	nat_tuple.rem_host_port		= naptEntry->remPort;
	nat_tuple.proto				= naptEntry->protocol;
	#else
	nat_tuple.int_host.ip			= naptEntry->intIp;
	nat_tuple.int_host.port			= naptEntry->intPort;
	nat_tuple.ext_host.ip			= naptEntry->extIp;
	nat_tuple.ext_host.port			= naptEntry->extPort;
	nat_tuple.rem_host.ip			= naptEntry->remIp;
	nat_tuple.rem_host.port		= naptEntry->remPort;
	nat_tuple.proto				= naptEntry->protocol;
	#endif

	nat_out = _rtl865x_nat_outbound_lookup(&nat_tuple);
	nat_in =  _rtl865x_nat_inbound_lookup(&nat_tuple);

	if ((nat_out==NULL) && (nat_in==NULL))
	{
		return TBLDRV_EENTRYNOTFOUND;
	}

	if(nat_out==nat_in)	
	{
		rtl8651_delAsicNaptTcpUdpTable(nat_out->out, nat_out->out);
		memset(nat_out, 0, sizeof(*nat_out));
		nat_tbl.freeEntryNum++;
		
	}
	else
	{
		if(nat_out)
		{
			rtl8651_delAsicNaptTcpUdpTable(nat_out->out, nat_out->out);
			memset(nat_out, 0, sizeof(*nat_out));
			nat_tbl.freeEntryNum++;
		}	

		if(nat_in)
		{
			rtl8651_delAsicNaptTcpUdpTable(nat_in->in, nat_in->in);
			memset(nat_in, 0, sizeof(*nat_in));
			nat_tbl.freeEntryNum++;
		}
	}
	#ifdef CONFIG_HARDWARE_NAT_DEBUG
	/*2007-12-19*/
	rtlglue_printf("LR(%s):  %s (%u.%u.%u.%u:%u -> %u.%u.%u.%u:%u) g:(%u.%u.%u.%u:%u)\n",
			("del_nat"), ((naptEntry->protocol)? "tcp": "udp"), 
			NIPQUAD(naptEntry->intIp), naptEntry->intPort, NIPQUAD(naptEntry->remIp), naptEntry->remPort, NIPQUAD(naptEntry->extIp), naptEntry->extPort);
	#endif

	if(nat_tbl.connNum>0)
	{
		nat_tbl.connNum--;
	}
	
	return SUCCESS;
}
#if 0
static int32 _rtl865x_naptIdleCheck(int32 index)
{
	rtl865x_tblAsicDrv_naptTcpUdpParam_t	entry;
	int32 	hw_now;
	
	rtl8651_getAsicNaptTcpUdpTable(index, &entry);

	hw_now = entry.ageSec;
	
	if (entry.isTcp) {
		return (nat_tbl.tcp_timeout>hw_now?nat_tbl.tcp_timeout-hw_now:0);
	} else {
		return (nat_tbl.udp_timeout>hw_now?nat_tbl.udp_timeout-hw_now:0);
	}
}
#endif
static int32 _rtl865x_naptSync(rtl865x_napt_entry *naptEntry, uint32 refresh)
{
	rtl865x_tblAsicDrv_naptTcpUdpParam_t asic_nat_out;
	rtl865x_tblAsicDrv_naptTcpUdpParam_t asic_nat_in;
	struct nat_entry *nat_out,*nat_in;
	struct nat_tuple nat_tuple;
	int32 rc;

	memset(&nat_tuple, 0, sizeof(struct nat_tuple));
	#if defined(CONFIG_RTL_REDUCE_MEMORY)
	nat_tuple.in_host_ip			= naptEntry->intIp;
	nat_tuple.in_host_port			= naptEntry->intPort;
	nat_tuple.ext_host_ip			= naptEntry->extIp;
	nat_tuple.ext_host_port			= naptEntry->extPort;
	nat_tuple.rem_host_ip			= naptEntry->remIp;
	nat_tuple.rem_host_port		= naptEntry->remPort;
	nat_tuple.proto				= naptEntry->protocol;
	#else
	nat_tuple.int_host.ip			= naptEntry->intIp;
	nat_tuple.int_host.port			= naptEntry->intPort;
	nat_tuple.ext_host.ip			= naptEntry->extIp;
	nat_tuple.ext_host.port			= naptEntry->extPort;
	nat_tuple.rem_host.ip			= naptEntry->remIp;
	nat_tuple.rem_host.port		= naptEntry->remPort;
	nat_tuple.proto				= naptEntry->protocol;
	#endif
	
	nat_out = _rtl865x_nat_outbound_lookup(&nat_tuple);
	nat_in = _rtl865x_nat_inbound_lookup(&nat_tuple);

	if( (!nat_out) && (!nat_in))
	{
		return 0;

	}

	memset(&asic_nat_out ,0 ,sizeof(rtl865x_tblAsicDrv_naptTcpUdpParam_t));
	memset(&asic_nat_in,0 ,sizeof(rtl865x_tblAsicDrv_naptTcpUdpParam_t));
		
	if((nat_out!=NULL) && (nat_in!=NULL))
	{
		rc = rtl8651_getAsicNaptTcpUdpTable(nat_out->out, &asic_nat_out);
		ASSERT(rc==SUCCESS);
		
		rc = rtl8651_getAsicNaptTcpUdpTable(nat_in->in, &asic_nat_in);
		ASSERT(rc==SUCCESS);
		return (asic_nat_out.ageSec>asic_nat_in.ageSec)? asic_nat_out.ageSec: asic_nat_in.ageSec;
	}
	else if((nat_out!=NULL) && (nat_in==NULL))
	{
		rc = rtl8651_getAsicNaptTcpUdpTable(nat_out->out, &asic_nat_out);
		ASSERT(rc==SUCCESS);
		return asic_nat_out.ageSec;
	}
	else if((nat_out==NULL) && (nat_in!=NULL))
	{
		rc = rtl8651_getAsicNaptTcpUdpTable(nat_in->in, &asic_nat_in);
		ASSERT(rc==SUCCESS);
		return asic_nat_in.ageSec;
		
	}
	else
	{
		return 0;
	}

	return 0;
	
	
}


/*
@func int32 | rtl865x_addNaptConnection |add a napt entry.
@parm uint32 | protocol | protocol.
@parm ipaddr_t | intIp | internal ip address.
@parm uint32 | intPort | internal port.
@parm ipaddr_t | extIp | external ip address.
@parm uint32 | extPort | external port.
@parm ipaddr_t | remIp | remote ip address.
@parm uint32 | remPort | remote port.
@parm int32 | flags | flags.
@rvalue SUCCESS | success.
@rvalue FAILED | failed.
@rvalue RTL_EINVALIDINPUT | invalid input.
@rvalue RTL_EENTRYALREADYEXIST | route entry is already exist.
@rvalue RTL_ENOFREEBUFFER | not enough memory in system.
@comm
	value of protocol should be RTL865X_PROTOCOL_TCP/RTL865X_PROTOCOL_UDP
*/
int32 rtl865x_addNaptConnection(rtl865x_napt_entry *naptEntry, rtl865x_priority *prio)
{
	int s;
	int32 retval = FAILED;

	s = splimp();
	retval = _rtl865x_addNaptConnection(naptEntry, prio);
	splx(s);
	return retval;
}

/*
@func int32 | rtl865x_delNaptConnection |delete a napt entry.
@parm uint32 | protocol | protocol.
@parm ipaddr_t | intIp | internal ip address.
@parm uint32 | intPort | internal port.
@parm ipaddr_t | extIp | external ip address.
@parm uint32 | extPort | external port.
@parm ipaddr_t | remIp | remote ip address.
@parm uint32 | remPort | remote port.

@rvalue SUCCESS | success.
@rvalue FAILED | failed.
@rvalue RTL_EENTRYNOTFOUND | not found this entry in system.
@comm
*/
int32 rtl865x_delNaptConnection(rtl865x_napt_entry *naptEntry)
{
	int s;
	int32 retval = FAILED;

	s = splimp();
	retval = _rtl865x_delNaptConnection(naptEntry);
	splx(s);
	
	return retval;
}

int32 rtl865x_naptSync(rtl865x_napt_entry *naptEntry, uint32 refresh)
{
	return _rtl865x_naptSync(naptEntry,refresh);
}

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT) && defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)
inline static int32 rtl865x_naptSetAsicWithPriority(struct nat_entry *entry, int32 priority)
{
	rtl865x_tblAsicDrv_naptTcpUdpParam_t asic_nat;
	int32		idx;

	//If qos enabled, not add inbound napt
	//That is all downlink pkt will be trapped to CPU for software QoS
	if(entry->flags&NAT_INBOUND)
		return SUCCESS;
	
	//printk("--%s(%d),entry(%p)\n",__FUNCTION__,__LINE__,entry);
	idx = (entry->flags&NAT_INBOUND)?entry->in:entry->out;
	
	rtl8651_getAsicNaptTcpUdpTable(idx, &asic_nat);
	asic_nat.priority = priority;
	asic_nat.priValid = TRUE;
	rtl8651_setAsicNaptTcpUdpTable(1, idx, &asic_nat);

	return SUCCESS;
}

static int32 rtl865x_naptCallbackFn_for_qosChange(void *param)
{
	int num, i;
	struct nat_entry *nat_this, *nat_that;
	ipaddr_t		sip, dip;
	uint16		sport, dport;
	rtl865x_route_t		rt;
	rtl865x_arpMapping_entry_t	arpInfo;
	rtl865x_AclRule_t		rule4, rule2;
	int32		priority=-1, defPriority=-1;

	num = i = 0;

	while(num < nat_tbl.connNum && i < RTL8651_TCPUDPTBL_SIZE)
	{
		if(NAT_INUSE(&nat_tbl.nat_bucket[i]))
		{
			nat_this = &nat_tbl.nat_bucket[i];
			if (nat_this->flags&NAT_INBOUND)
			{
				if(nat_this->out!=0xFFFFFFFF)
				{
					nat_that = &nat_tbl.nat_bucket[nat_this->out];
				}
				else
				{
					/*no outbound*/
					nat_that = NULL;
				}
				
				sip = nat_this->rem_ip_;
				dip = nat_this->ext_ip_;
				sport = nat_this->rem_port_;
				dport = nat_this->ext_port_;
			}
			else
			{
				if(nat_this->in!=0xFFFFFFFF)
				{
					nat_that = &nat_tbl.nat_bucket[nat_this->in];
				}
				else
				{
					/*no inbound*/
					nat_that=NULL;
				}
				
				sip = nat_this->int_ip_;
				dip = nat_this->rem_ip_;
				sport = nat_this->int_port_;
				dport = nat_this->rem_port_;
			}
			
			if (nat_this->flags&NAT_PRI_PROCESSED)
			{
				CLR_NAT_FLAGS(nat_this, NAT_PRI_PROCESSED);
				num++;
			}
			else 
			{
				if (rtl865x_getRouteEntry(sip, &rt)==SUCCESS)
				{
					memset(&rule4, 0, sizeof(rtl865x_AclRule_t));
					rule4.ruleType_ = (nat_this->proto_==RTL865X_PROTOCOL_TCP?RTL865X_ACL_TCP:RTL865X_ACL_UDP);
					rule4.srcIpAddr_ = sip;
					rule4.dstIpAddr_ = dip;
					rule4.tcpSrcPortLB_ = sport;
					rule4.tcpDstPortLB_ = dport;
					rule4.netifIdx_ = _rtl865x_getNetifIdxByNameExt(rt.dstNetif->name);
					if(rule4.netifIdx_ < 0 || rule4.netifIdx_ >= NETIF_NUMBER)
					{
						printk("===%s %s(%d) Can't get netif(%s)\n",__FILE__,__FUNCTION__,__LINE__,rt.dstNetif->name);						
					}

					if(rule4.netifIdx_ >=0 && rule4.netifIdx_ < NETIF_NUMBER)
						if (rtl865x_qosCheckNaptPriority(&rule4)!=SUCCESS)
						{
							if (nat_this->flags&NAT_INBOUND)
							{
								rule4.dstIpAddr_ = nat_this->int_ip_;
								rule4.tcpDstPortLB_ = nat_this->int_port_;
								if (rtl865x_qosCheckNaptPriority(&rule4)!=SUCCESS)
									defPriority = rule4.priority_;
							}
							else
								defPriority = rule4.priority_;
						}
						
					if (rtl865x_getArpMapping(sip, &arpInfo)==SUCCESS)
					{
						memset(&rule2, 0, sizeof(rtl865x_AclRule_t));
						rule2.ruleType_ = RTL865X_ACL_MAC;
						memcpy(rule2.srcMac_.octet, arpInfo.mac.octet, ETHER_ADDR_LEN);
						memset(rule2.srcMacMask_.octet, 0xff, ETHER_ADDR_LEN);
						rule2.netifIdx_ = _rtl865x_getNetifIdxByNameExt(rt.dstNetif->name);
						if(rule4.netifIdx_ < 0 || rule4.netifIdx_ >= NETIF_NUMBER)
						{
							printk("===%s %s(%d) Can't get netif(%s)\n",__FILE__,__FUNCTION__,__LINE__,rt.dstNetif->name);						
						}
						
						if(rule4.netifIdx_ >=0 && rule4.netifIdx_ < NETIF_NUMBER)
							if (rtl865x_qosCheckNaptPriority(&rule2)!=SUCCESS)
							{
								if (nat_this->flags&NAT_INBOUND)
								{
									if(rtl865x_getArpMapping(nat_this->int_ip_, &arpInfo)==SUCCESS)
									{
										memset(&rule2, 0, sizeof(rtl865x_AclRule_t));
										rule2.ruleType_ = RTL865X_ACL_MAC;
										memcpy(rule2.dstMac_.octet, arpInfo.mac.octet, ETHER_ADDR_LEN);
										memset(rule2.dstMacMask_.octet, 0xff, ETHER_ADDR_LEN);
										rule2.netifIdx_ = _rtl865x_getNetifIdxByNameExt(rt.dstNetif->name);
										if (rtl865x_qosCheckNaptPriority(&rule2)!=SUCCESS)
											defPriority = rule2.priority_;
									}
								}
								else
									defPriority = rule2.priority_;
							}
					}

					if (rule4.aclIdx&& rule2.aclIdx)
					{
						priority = (rule4.aclIdx<rule2.aclIdx)?rule4.priority_:rule2.priority_;
					}
					else if (rule4.aclIdx)
					{
						priority = rule4.priority_;
					}
					else if (rule2.aclIdx)
					{
						priority = rule2.priority_;
					}

					if (priority>-1)
					{
						
						rtl865x_naptSetAsicWithPriority(nat_this, priority);
						
						if(nat_that!=NULL)
						{
							rtl865x_naptSetAsicWithPriority(nat_that, priority);
						}
						
						if (nat_this->flags&NAT_PRI_HALF_PROCESSED)
						{
							CLR_NAT_FLAGS(nat_this, NAT_PRI_HALF_PROCESSED);
							num++;
						}
						else
						{
							if(nat_that!=NULL)
							{
								SET_NAT_FLAGS(nat_that, NAT_PRI_PROCESSED);
							}
							else
							{
								/*only half accelerated*/
								num++;
							}
						}
					}
					else
					{
						if (nat_this->flags&NAT_PRI_HALF_PROCESSED)
						{
							rtl865x_naptSetAsicWithPriority(nat_this, defPriority>-1?defPriority:0);
							ASSERT(nat_that!=NULL);
							if(nat_that!=NULL)
							{
								rtl865x_naptSetAsicWithPriority(nat_that, defPriority>-1?defPriority:0);
							}
							CLR_NAT_FLAGS(nat_this, NAT_PRI_HALF_PROCESSED);
							num++;
						}
						else
						{
							if(nat_that!=NULL)
							{
								SET_NAT_FLAGS(nat_that, NAT_PRI_HALF_PROCESSED);
							}
							else
							{
								/*only half accelerated*/
								num++;
							}
						}
					}
				}
				else
				{
					if (nat_this->flags&NAT_PRI_HALF_PROCESSED)
					{
						CLR_NAT_FLAGS(nat_this, NAT_PRI_HALF_PROCESSED);
						num++;
					}
					else
					{
						if(nat_that!=NULL)
						{
							SET_NAT_FLAGS(nat_that, NAT_PRI_HALF_PROCESSED);
						}
						else
						{
							/*only half accelerated*/
							num++;
						}
					}
				}
			}
		}

		i++;
	}
	return EVENT_CONTINUE_EXECUTE;
}


static int32 rtl865x_napt_register_qosEvent(void)
{
	rtl865x_event_Param_t eventParam;
	
	eventParam.eventLayerId=DEFAULT_LAYER2_EVENT_LIST_ID;
	eventParam.eventId=EVENT_CHANGE_QOSRULE;
	eventParam.eventPriority=0;
	eventParam.event_action_fn=rtl865x_naptCallbackFn_for_qosChange;
	rtl865x_registerEvent(&eventParam);

	eventParam.eventId=EVENT_FLUSH_QOSRULE;
	rtl865x_registerEvent(&eventParam);

	eventParam.eventLayerId=DEFAULT_LAYER3_EVENT_LIST_ID;
	eventParam.eventId=EVENT_ADD_ARP;
	rtl865x_registerEvent(&eventParam);

	return SUCCESS;
}

static int32 rtl865x_napt_unRegister_qosEvent(void)
{
	rtl865x_event_Param_t eventParam;
	
	eventParam.eventLayerId=DEFAULT_LAYER2_EVENT_LIST_ID;
	eventParam.eventId=EVENT_CHANGE_QOSRULE;
	eventParam.eventPriority=0;
	eventParam.event_action_fn=rtl865x_naptCallbackFn_for_qosChange;
	rtl865x_unRegisterEvent(&eventParam);

	eventParam.eventId=EVENT_FLUSH_QOSRULE;
	rtl865x_unRegisterEvent(&eventParam);

	eventParam.eventLayerId=DEFAULT_LAYER3_EVENT_LIST_ID;
	eventParam.eventId=EVENT_ADD_ARP;
	rtl865x_unRegisterEvent(&eventParam);

	return SUCCESS;
}
#endif

/*
@func int32 | rtl865x_setNatFourWay |enable 4way hash algorithm.
@parm int32 | enable | enable or disable.
@rvalue SUCCESS | success.
@comm
	default is enable in system.
*/
int32 rtl865x_setNatFourWay(int32 enable)
{
	 _set4WayHash(enable);
	rtl865x_enableNaptFourWay=enable;
	return SUCCESS;
}

/*
@func int32 | rtl865x_nat_init |initialize napt table.
@rvalue SUCCESS | success.
@comm	
*/
int32 rtl865x_nat_init(void)
{
	int32 retval = FAILED;
#if	defined(CONFIG_RTL_HW_QOS_SUPPORT) && defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)
	rtl865x_napt_unRegister_qosEvent();
#endif

	retval = _rtl865x_nat_init();
	rtl865x_setNatFourWay(TRUE);

#if	defined(CONFIG_RTL_HW_QOS_SUPPORT) && defined(CONFIG_RTL_IPTABLES_RULE_2_ACL)
	rtl865x_napt_register_qosEvent();
#endif
	return retval;
}

int32 rtl865x_nat_reinit(void)
{
	return rtl865x_nat_init();

}

#ifdef CONFIG_RTL_PROC_DEBUG
int32 rtl865x_flushAllNaptConnection(void)
{
	uint32 i,outIndex,inIndex;
	struct nat_entry *nat_out=NULL, *nat_in=NULL, *tmp=NULL;
	for(i=0;i<RTL8651_TCPUDPTBL_SIZE;i++)
	{
		tmp = &nat_tbl.nat_bucket[i];

		if(NAT_INUSE(tmp))
		{
			outIndex=tmp->out;
			inIndex=tmp->in;
			if(outIndex!=0xFFFFFFFF)
			{
				nat_out=&nat_tbl.nat_bucket[outIndex];
			}
			else
			{
				nat_out=NULL;
			}

			if(inIndex!=0xFFFFFFFF)
			{
				nat_in=&nat_tbl.nat_bucket[inIndex];	
			}
			else
			{
				nat_in=NULL;	
			}

			if((nat_out==NULL) &&(nat_in==NULL))
			{
				rtl8651_delAsicNaptTcpUdpTable(i, i);
				continue;
			}
			
			if(nat_out==nat_in)
			{
				if(nat_out->flags&NAT_OUTBOUND)
				{
					rtl8651_delAsicNaptTcpUdpTable(outIndex, outIndex);
					memset(nat_out, 0, sizeof(*nat_out));
					nat_tbl.freeEntryNum++;
				}
				else if(nat_in->flags&NAT_INBOUND)
				{
					rtl8651_delAsicNaptTcpUdpTable(inIndex, inIndex);
					memset(nat_in, 0, sizeof(*nat_in));
					nat_tbl.freeEntryNum++;
				}
				else
				{
					/*fatal error*/
					return RTL_EENTRYNOTFOUND;
				}
			}
			else
			{
				if((nat_out!=NULL) && (nat_out->flags&NAT_OUTBOUND))
				{
					rtl8651_delAsicNaptTcpUdpTable(outIndex, outIndex);
					memset(nat_out, 0, sizeof(*nat_out));
					nat_tbl.freeEntryNum++;
				}
				
				if((nat_in!=NULL) && (nat_in->flags&NAT_INBOUND))		
				{
					rtl8651_delAsicNaptTcpUdpTable(inIndex,inIndex);
					memset(nat_in, 0, sizeof(*nat_in));
					nat_tbl.freeEntryNum++;
				}
			}
			
			if(nat_tbl.connNum>0)
			{
				nat_tbl.connNum--;
			}
		
		}
	}
	
	return SUCCESS;
}

int32 rtl865x_sw_napt_seq_read(struct seq_file *s, void *v)
{

	int i;
	struct nat_entry *natEntryPtr;
	int len=0;
	
	len = seq_printf(s, "%s\n", "sw napt table:");
	
	for(i=0; i<RTL8651_TCPUDPTBL_SIZE; i++)
	{
		natEntryPtr= &nat_tbl.nat_bucket[i];
		if(NAT_INUSE(natEntryPtr))
		{
			if(natEntryPtr->flags&NAT_OUTBOUND)
			{
				len += seq_printf(s, "[%4d]%s:%d.%d.%d.%d:%d---->%d.%d.%d.%d:%d---->%d.%d.%d.%d:%d\n      flags:0x%x,outbound:(%d),inbound:(%d)\n",
				i,natEntryPtr->proto_==1?"tcp":"udp" ,NIPQUAD(natEntryPtr->int_ip_),natEntryPtr->int_port_,
				NIPQUAD(natEntryPtr->ext_ip_),natEntryPtr->ext_port_,NIPQUAD(natEntryPtr->rem_ip_),natEntryPtr->rem_port_,natEntryPtr->flags,natEntryPtr->out, natEntryPtr->in);
			}

			if(natEntryPtr->flags&NAT_INBOUND)
			{
				len += seq_printf(s, "[%4d]%s:%d.%d.%d.%d:%d<----%d.%d.%d.%d:%d<----%d.%d.%d.%d:%d\n      flags:0x%x, outbound:(%d), inbound:(%d)\n",
				i,natEntryPtr->proto_==1?"tcp":"udp" ,NIPQUAD(natEntryPtr->int_ip_),natEntryPtr->int_port_,
				NIPQUAD(natEntryPtr->ext_ip_),natEntryPtr->ext_port_,NIPQUAD(natEntryPtr->rem_ip_),natEntryPtr->rem_port_,natEntryPtr->flags,natEntryPtr->out,natEntryPtr->in);
			}
		}
	
	}
	
	len += seq_printf(s, "total napt connection number is %d\n",nat_tbl.connNum);
	len += seq_printf(s, "total free entry number is %d\n",nat_tbl.freeEntryNum);
	return 0;
}

int32  rtl865x_sw_napt_seq_write( struct file *filp, const char *buff,unsigned long len, loff_t *off )
{
	char 	tmpbuf[64];
	uint32	delIndex,inIndex=0,outIndex=0;
	char		*strptr, *cmd_addr;
	char		*tokptr;
	struct nat_entry *nat_out=NULL, *nat_in=NULL, *tmp=NULL;
	
	if (buff && !copy_from_user(tmpbuf, buff, len)) {
		tmpbuf[len] = '\0';
		strptr=tmpbuf;
		cmd_addr = strsep(&strptr," ");
		if (cmd_addr==NULL)
		{
			goto errout;
		}

		if (!memcmp(cmd_addr, "flush", 5))
		{
			rtl865x_flushAllNaptConnection();
		}
		else if (!memcmp(cmd_addr, "del", 3))
		{
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
			{
				goto errout;
			}

			delIndex=simple_strtol(tokptr, NULL, 0);
			if(delIndex>RTL8651_TCPUDPTBL_SIZE)
			{
				printk("error input!\n");
				return len;
			}
			tmp = &nat_tbl.nat_bucket[delIndex];

			if(NAT_INUSE(tmp))
			{
				outIndex=tmp->out;
				inIndex=tmp->in;
				if(outIndex!=0xFFFFFFFF)
				{
					nat_out=&nat_tbl.nat_bucket[outIndex];
				}
				else
				{
					nat_out=NULL;
				}

				if(inIndex!=0xFFFFFFFF)
				{
					nat_in=&nat_tbl.nat_bucket[inIndex];	
				}
				else
				{
					nat_in=NULL;	
				}
			
				if ((nat_out==NULL) && (nat_in==NULL))
				{
					rtl8651_delAsicNaptTcpUdpTable(delIndex, delIndex);
					goto errout;
				}

				if(nat_out==nat_in)	
				{
					rtl8651_delAsicNaptTcpUdpTable(outIndex, outIndex);
					memset(nat_out, 0, sizeof(*nat_out));
					nat_tbl.freeEntryNum++;
				}
				else
				{
					if((nat_out!=NULL) && (nat_out->flags&NAT_OUTBOUND))
					{
						rtl8651_delAsicNaptTcpUdpTable(outIndex, outIndex);
						memset(nat_out, 0, sizeof(*nat_out));
						nat_tbl.freeEntryNum++;
					}
					
					if((nat_in!=NULL) && ( nat_in->flags&NAT_INBOUND))		
					{
						rtl8651_delAsicNaptTcpUdpTable(inIndex,inIndex);
						memset(nat_in, 0, sizeof(*nat_in));
						nat_tbl.freeEntryNum++;
					}
				}
				
				if(nat_tbl.connNum>0)
				{
					nat_tbl.connNum--;
				}
				printk("del napt flow,outbound:%d,inbound:%d\n", outIndex, inIndex);
			
			}
			
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		return len;
	}

	return len;
}

#endif

#if defined(CONFIG_RTL_AVOID_ADDING_WLAN_PKT_TO_HW_NAT)
static int rtl_checkLanIp(struct alias_link *link)
{

	unsigned long lanIp=link->src_addr.s_addr;

	if(rtl865x_isEthArp(lanIp)==TRUE)
	{
		return SUCCESS;
	}

	return FAILED;
}
#endif


int rtl_addHwNatEntry(struct alias_link *link, struct ip *iph)
{
	int ret = FAILED;
	int assured = 0;
	int create_conn = FALSE;
	int protocol;
	rtl865x_napt_entry rtl865xNaptEntry;
	rtl865x_priority rtl865xPrio;

	if (gHwNatEnabled!=1)
		return ret;

	if (link->link_type == IPPROTO_TCP) {
		protocol = RTL865X_PROTOCOL_TCP;
	} else if (link->link_type == IPPROTO_UDP) {
		protocol = RTL865X_PROTOCOL_UDP;
	} else {
		return ret;
	}
	
	assured = link->assured;
	
	#if defined(CONFIG_RTL_AVOID_ADDING_WLAN_PKT_TO_HW_NAT)
	if(rtl_checkLanIp(link)==FAILED)
		return ret;
	#endif

	#if defined(CONFIG_RTL_FREEBSD_FAST_PATH)
	if(!assured)
		create_conn = rtl_fpAddConnCheck(link, iph);
	#endif

	//diag_printf("assured is %d, link_type is %x, dir is %d, sIp is 0x%x, aIp is 0x%x, rIp is 0x%x\n", link->assured, link->link_type, link->dir,
		//	link->src_addr.s_addr, link->alias_addr.s_addr, link->dst_addr.s_addr);
#if defined(CONFIG_RTL_BYPASS_PKT)
/*lq add hw_nat      don't enter fastpath before 15 packets*/
	if (1 == FastPath_Enable2() && link->bypass_cnt < RTL_BYPASS_PKT_NUM) {
	    assured = 0;
	    create_conn = 0;
	}
#endif

	if (assured || (TRUE==create_conn))
	{
		rtl865xPrio.downlinkPrio=0;
		rtl865xPrio.uplinkPrio=0;

		rtl865xNaptEntry.protocol=protocol;
		rtl865xNaptEntry.intIp=link->src_addr.s_addr;
		rtl865xNaptEntry.intPort=link->src_port;
		rtl865xNaptEntry.extIp=link->alias_addr.s_addr;
		rtl865xNaptEntry.extPort=link->alias_port;
		rtl865xNaptEntry.remIp=link->dst_addr.s_addr;
		rtl865xNaptEntry.remPort=link->dst_port;

		ret = rtl865x_addNaptConnection(&rtl865xNaptEntry, &rtl865xPrio);
       }

	return ret;
}

int rtl_hwNatTimer_update(struct alias_link *link)
{
	/*2007-12-19*/
	unsigned long expires, now;
	int		drop;
	int protocol;
	rtl865x_napt_entry rtl865xNaptEntry;
	unsigned long lastUsed;
	int timeval = 0;

	if (gHwNatEnabled!=1)
		return FAILED;
	
	now = rtl_getTimeStamp();
	//read_lock_bh(&nf_conntrack_lock);
	if (link->link_type  == IPPROTO_UDP){
		protocol = RTL865X_PROTOCOL_UDP;
		expires = UDP_EXPIRE_TIME;
	}
	else if (link->link_type == IPPROTO_TCP){
		protocol = RTL865X_PROTOCOL_TCP;
		#if defined(CONFIG_RTL_FREEBSD_FAST_PATH)
		expires = rtl_getTcpExpireByState(link->data.tcp->state.in, link->data.tcp->state.out);
		#else
		expires = TCP_EXPIRE_INITIAL;
		#endif
	}
	else {
		//read_unlock_bh(&nf_conntrack_lock);
		return FAILED;
	}

	drop = TRUE;

	/* query for idle */

	rtl865xNaptEntry.protocol=protocol;
	rtl865xNaptEntry.intIp=link->src_addr.s_addr;
	rtl865xNaptEntry.intPort=link->src_port;
	rtl865xNaptEntry.extIp=link->alias_addr.s_addr;
	rtl865xNaptEntry.extPort=link->alias_port;
	rtl865xNaptEntry.remIp=link->dst_addr.s_addr;
	rtl865xNaptEntry.remPort=link->dst_port;

	timeval = rtl865x_naptSync(&rtl865xNaptEntry, 0);
	if (timeval > 0){
		drop = FALSE;
		link->expire_time = expires;
		link->timestamp   = now;
	}

	if (drop == FALSE) {
		return SUCCESS;
	} else{
		return FAILED;
	}

}

int rtl_delHwNatEntry(struct alias_link *link)
{
	int protocol;
	int ret;
	rtl865x_napt_entry rtl865xNaptEntry;

	if (gHwNatEnabled!=1)
		return FAILED;
	
	if (link->link_type == IPPROTO_TCP) {
		protocol = RTL865X_PROTOCOL_TCP;
	} else if (link->link_type == IPPROTO_UDP) {
		protocol = RTL865X_PROTOCOL_UDP;
	} else {
		return FAILED;
	}

	rtl865xNaptEntry.protocol=protocol;
	rtl865xNaptEntry.intIp=link->src_addr.s_addr;
	rtl865xNaptEntry.intPort=link->src_port;
	rtl865xNaptEntry.extIp=link->alias_addr.s_addr;
	rtl865xNaptEntry.extPort=link->alias_port;
	rtl865xNaptEntry.remIp=link->dst_addr.s_addr;
	rtl865xNaptEntry.remPort=link->dst_port;

	ret = rtl865x_delNaptConnection(&rtl865xNaptEntry);

	return ret;
}


void rtl865x_asicNaptShow(void)
{
	rtl865x_tblAsicDrv_naptTcpUdpParam_t asic_tcpudp;
	uint32 idx, entry=0;

	diag_printf("HW NAT %s!\n", (gHwNatEnabled==1)?"ENABLED":"DISABLED");
	diag_printf("%s\n", "ASIC NAPT TCP/UDP Table:\n");

	for(idx=0; idx<RTL8651_TCPUDPTBL_SIZE; idx++) {
		if (rtl8651_getAsicNaptTcpUdpTable(idx, &asic_tcpudp) == FAILED)
			continue;

		if (asic_tcpudp.isValid == 1 || asic_tcpudp.isDedicated == 1 ) {
			diag_printf("[%4d] %d.%d.%d.%d:%d {V,D}={%d,%d} col1(%d) col2(%d) static(%d) tcp(%d)\n",
			       idx,
			       asic_tcpudp.insideLocalIpAddr>>24, (asic_tcpudp.insideLocalIpAddr&0x00ff0000) >> 16,
			       (asic_tcpudp.insideLocalIpAddr&0x0000ff00)>>8, asic_tcpudp.insideLocalIpAddr&0x000000ff,
			       asic_tcpudp.insideLocalPort, 
			       asic_tcpudp.isValid, asic_tcpudp.isDedicated,
			       asic_tcpudp.isCollision, asic_tcpudp.isCollision2, asic_tcpudp.isStatic, asic_tcpudp.isTcp );

			diag_printf("   age(%d) offset(%d) tcpflag(%d) SelEIdx(%d) SelIPIdx(%d) priValid:%d pri(%d)\n",
			        asic_tcpudp.ageSec, asic_tcpudp.offset<<10, asic_tcpudp.tcpFlag, 
			        asic_tcpudp.selEIdx, asic_tcpudp.selExtIPIdx,asic_tcpudp.priValid,asic_tcpudp.priority );
			entry++;
		}
	}
	diag_printf("Total entry: %d\n", entry);	
	
	return 0;
}

void rtl_hwNatOnOff(int value)
{
	rtl865x_tblAsicDrv_naptTcpUdpParam_t naptTcpUdp;
	uint32 flowTblIdx;
	
	gHwNatEnabled = value;
	if(value == 0){	//hw_nat off
		/* Initial ASIC NAT Table */
	memset( &naptTcpUdp, 0, sizeof(naptTcpUdp) );
	naptTcpUdp.isCollision = 1;
	naptTcpUdp.isCollision2 = 1;
	for(flowTblIdx=0; flowTblIdx<RTL8651_TCPUDPTBL_SIZE; flowTblIdx++)
		rtl8651_setAsicNaptTcpUdpTable(TRUE, flowTblIdx, &naptTcpUdp );
	}
}


