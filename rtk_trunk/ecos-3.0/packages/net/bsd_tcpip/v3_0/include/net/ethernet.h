//==========================================================================
//
//      include/net/ethernet.h
//
//==========================================================================
// ####BSDCOPYRIGHTBEGIN####                                    
// -------------------------------------------                  
// This file is part of eCos, the Embedded Configurable Operating System.
//
// Portions of this software may have been derived from FreeBSD 
// or other sources, and if so are covered by the appropriate copyright
// and license included herein.                                 
//
// Portions created by the Free Software Foundation are         
// Copyright (C) 2002 Free Software Foundation, Inc.            
// -------------------------------------------                  
// ####BSDCOPYRIGHTEND####                                      
//==========================================================================

/*
 * Fundamental constants relating to ethernet.
 *
 * $FreeBSD: src/sys/net/ethernet.h,v 1.12.2.5 2000/07/19 21:43:52 archie Exp $
 *
 */

#ifndef _NET_ETHERNET_H_
#define _NET_ETHERNET_H_

/*
 * The number of bytes in an ethernet (MAC) address.
 */
#define	ETHER_ADDR_LEN		6

/*
 * The number of bytes in the type field.
 */
#define	ETHER_TYPE_LEN		2

/*
 * The number of bytes in the trailing CRC field.
 */
#define	ETHER_CRC_LEN		4

/*
 * The length of the combined header.
 */
#define	ETHER_HDR_LEN		(ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)

/*
 * The minimum packet length.
 */
#define	ETHER_MIN_LEN		64

/*
 * The maximum packet length.
 */
#define	ETHER_MAX_LEN		1518

/*
 * A macro to validate a length with
 */
#define	ETHER_IS_VALID_LEN(foo)	\
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	u_char	ether_dhost[ETHER_ADDR_LEN];
	u_char	ether_shost[ETHER_ADDR_LEN];
	u_short	ether_type;
} __attribute__ ((aligned(1), packed));

/*
 * Structure of a 48-bit Ethernet address.
 */
struct	ether_addr {
	u_char octet[ETHER_ADDR_LEN];
} __attribute__ ((aligned(1), packed));

#define	ETHERTYPE_PUP		0x0200	/* PUP protocol */
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
#define	ETHERTYPE_ARP		0x0806	/* Addr. resolution protocol */
#define	ETHERTYPE_REVARP	0x8035	/* reverse Addr. resolution protocol */
#define	ETHERTYPE_VLAN		0x8100	/* IEEE 802.1Q VLAN tagging */
#define	ETHERTYPE_IPV6		0x86dd	/* IPv6 */
#define	ETHERTYPE_LOOPBACK	0x9000	/* used to test interfaces */

#define ETHERTYPE_PPPOE_DISC	0x8863	/* pppoe discovery packets     */
#define ETHERTYPE_PPPOE_SESS	0x8864	/* pppoe session packets       */
/*modify by lq ,Add by cfm for batch upgrade*/
#define	ETHERTYPE_BATCH_UPG			0x5151	/* Batch upgrade ctrl  */
#define	ETHERTYPE_BATCH_UPG_DATA	0x5252	/* Batch upgrade data */
/*lq end cfm*/
/* XXX - add more useful types here */
/*lq add*/
#define	ETHERTYPE_PAE		0x888e	/* EAPOL PAE/802.1x */
#define	ETHERTYPE_PRE_AUTH  0x88c7	/* 802.1x preauthentication */
#define	ETHERTYPE_8021Q		ETHERTYPE_VLAN		/* 802.1Q */
#define	ETHERTYPE_BRCM		0x886c		/* Broadcom Corp. */
#define	ETHERTYPE_TOPO		0x88d9		/* Vista LLTD protocol. */
#define	ETHERTYPE_MAX		0xFFFF		/* Maximum valid ethernet type, reserved */
/*lq add end*/
/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#define	ETHERMTU	(ETHER_MAX_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#define	ETHERMIN	(ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)

#ifdef _KERNEL

/*
 * For device drivers to specify whether they support BPF or not
 */
#define ETHER_BPF_UNSUPPORTED	0
#define ETHER_BPF_SUPPORTED	1

struct ifnet;
struct mbuf;

extern	void (*ng_ether_input_p)(struct ifnet *ifp,
		struct mbuf **mp, struct ether_header *eh);
extern	void (*ng_ether_input_orphan_p)(struct ifnet *ifp,
		struct mbuf *m, struct ether_header *eh);
extern	int  (*ng_ether_output_p)(struct ifnet *ifp, struct mbuf **mp);
extern	void (*ng_ether_attach_p)(struct ifnet *ifp);
extern	void (*ng_ether_detach_p)(struct ifnet *ifp);

#else /* _KERNEL */

#include <sys/param.h>

/*
 * Ethernet address conversion/parsing routines.
 */
__BEGIN_DECLS
struct	ether_addr *ether_aton __P(( char *));
int	ether_hostton __P((const char *, struct ether_addr *));
int	ether_line __P((const char *, struct ether_addr *, char *));
char 	*ether_ntoa __P((const struct ether_addr *));
int	ether_ntohost __P((char *, const struct ether_addr *));
__END_DECLS

#endif /* !_KERNEL */

#endif /* !_NET_ETHERNET_H_ */
