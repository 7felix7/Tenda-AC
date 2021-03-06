/*
 * Common OS-independent driver header for rate management.
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlc_rate.h,v 1.83.2.4 2010-12-05 00:53:16 Exp $
 */

#ifndef _WLC_RATE_H_
#define _WLC_RATE_H_

extern const uint8 rate_info[];
extern const struct wlc_rateset cck_ofdm_mimo_rates;
extern const struct wlc_rateset ofdm_mimo_rates;
extern const struct wlc_rateset cck_ofdm_rates;
extern const struct wlc_rateset ofdm_rates;
extern const struct wlc_rateset cck_rates;
extern const struct wlc_rateset gphy_legacy_rates;
extern const struct wlc_rateset wlc_lrs_rates;
extern const struct wlc_rateset rate_limit_1_2;

typedef struct mcs_info {
	uint32 phy_rate_20;	/* phy rate in kbps [20Mhz] */
	uint32 phy_rate_40;	/* phy rate in kbps [40Mhz] */
	uint32 phy_rate_20_sgi;	/* phy rate in kbps [20Mhz] with SGI */
	uint32 phy_rate_40_sgi;	/* phy rate in kbps [40Mhz] with SGI */
	uint8  tx_phy_ctl3;	/* phy ctl byte 3, code rate, modulation type, # of streams */
	uint8  leg_ofdm;	/* matching legacy ofdm rate in 500bkps */
} mcs_info_t;

#define WLC_MAXMCS	32	/* max valid mcs index */
#define MCS_TABLE_SIZE	33	/* Number of mcs entries in the table */
extern const mcs_info_t mcs_table[];

#define MCS_INVALID	0xFF
#define MCS_CR_MASK	0x07	/* Code Rate bit mask */
#define MCS_MOD_MASK	0x38	/* Modulation bit shift */
#define MCS_MOD_SHIFT	3	/* MOdulation bit shift */
#define MCS_TXS_MASK   	0xc0	/* num tx streams - 2 bit mask */
#define MCS_TXS_SHIFT	6	/* num tx streams - 2 bit shift */
#define MCS_CR(_mcs)	(mcs_table[_mcs].tx_phy_ctl3 & MCS_CR_MASK)
#define MCS_MOD(_mcs)	((mcs_table[_mcs].tx_phy_ctl3 & MCS_MOD_MASK) >> MCS_MOD_SHIFT)
#define MCS_TXS(_mcs)	((mcs_table[_mcs].tx_phy_ctl3 & MCS_TXS_MASK) >> MCS_TXS_SHIFT)
#define MCS_RATE(_mcs, _is40, _sgi)	(_sgi ? \
	(_is40 ? mcs_table[_mcs].phy_rate_40_sgi : mcs_table[_mcs].phy_rate_20_sgi) : \
	(_is40 ? mcs_table[_mcs].phy_rate_40 : mcs_table[_mcs].phy_rate_20))
#define VALID_MCS(_mcs)	((_mcs < MCS_TABLE_SIZE))

#define	WLC_RATE_FLAG	0x80	/* Rate flag: basic or ofdm */

/* Macros to use the rate_info table */
#define	RATE_MASK	0x7f		/* Rate value mask w/o basic rate flag */
#define	RATE_MASK_FULL	0xff		/* Rate value mask with basic rate flag */


#define WLC_RATE_500K_TO_BPS(rate)	((rate) * 500000)	/* convert 500kbps to bps */

/* rate spec : holds rate and mode specific information required to generate a tx frame. */
/* Legacy CCK and OFDM information is held in the same manner as was done in the past    */
/* (in the lower byte) the upper 3 bytes primarily hold MIMO specific information        */
typedef uint32	ratespec_t;

/* rate spec bit fields */
#define RSPEC_RATE_MASK		0x0000007F     	/* Either 500Kbps units or MIMO MCS idx */
#define RSPEC_MIMORATE		0x08000000	/* mimo MCS is stored in RSPEC_RATE_MASK */
#define RSPEC_BW_MASK		0x00000700     	/* mimo bw mask */
#define RSPEC_BW_SHIFT		8		/* mimo bw shift */
#define RSPEC_STF_MASK		0x00003800     	/* mimo Space/Time/Frequency mode mask */
#define RSPEC_STF_SHIFT		11		/* mimo Space/Time/Frequency mode shift */
#define RSPEC_CT_MASK		0x0000C000     	/* mimo coding type mask */
#define RSPEC_CT_SHIFT		14		/* mimo coding type shift */
#define RSPEC_STC_MASK		0x00300000	/* mimo num STC streams per PLCP defn. */
#define RSPEC_STC_SHIFT		20		/* mimo num STC streams per PLCP defn. */
#define RSPEC_LDPC_CODING	0x00400000	/* mimo bit indicates adv coding in use */
#define RSPEC_SHORT_GI		0x00800000	/* mimo bit indicates short GI in use */
#define RSPEC_SHORT_PREAMBLE	0x00800000	/* legacy bit indicates DSSS short preable */
#define RSPEC_OVERRIDE		0x80000000	/* bit indicates override both rate & mode */
#define RSPEC_OVERRIDE_MCS_ONLY 0x40000000      /* bit indicates override rate only */

#define WLC_HTPHY		127		/* HT PHY Membership */

#define RSPEC_ACTIVE(rspec)	(rspec & (RSPEC_RATE_MASK | RSPEC_MIMORATE))
#ifdef WL11N
#define RSPEC2RATE(rspec)      	((rspec & RSPEC_MIMORATE) ? \
	MCS_RATE((rspec & RSPEC_RATE_MASK), RSPEC_IS40MHZ(rspec), RSPEC_ISSGI(rspec)) : \
	(rspec & RSPEC_RATE_MASK))
/* return rate in unit of 500Kbps -- for internal use in wlc_rate_sel.c */
#define RSPEC2RATE500K(rspec)	((rspec & RSPEC_MIMORATE) ? \
		MCS_RATE((rspec & RSPEC_RATE_MASK), state->is40bw, RSPEC_ISSGI(rspec))/500 : \
		(rspec & RSPEC_RATE_MASK))
#define CRSPEC2RATE500K(rspec)	((rspec & RSPEC_MIMORATE) ? \
		MCS_RATE((rspec & RSPEC_RATE_MASK), RSPEC_IS40MHZ(rspec), RSPEC_ISSGI(rspec))/500 :\
		(rspec & RSPEC_RATE_MASK))
#else /* WL11N */
#define RSPEC2RATE(rspec)      	(rspec & RSPEC_RATE_MASK)
#define RSPEC2RATE500K(rspec)	(rspec & RSPEC_RATE_MASK)
#define CRSPEC2RATE500K(rspec)	(rspec & RSPEC_RATE_MASK)
#endif /* WL11N */

#define RSPEC2KBPS(rspec)	(IS_MCS(rspec) ? RSPEC2RATE(rspec) : RSPEC2RATE(rspec)*500)
#define RSPEC_PHYTXBYTE2(rspec)	((rspec & 0xff00) >> 8)
#define RSPEC_GET_BW(rspec)	((rspec & RSPEC_BW_MASK) >> RSPEC_BW_SHIFT)
#define RSPEC_IS40MHZ(rspec)	((((rspec & RSPEC_BW_MASK) >> RSPEC_BW_SHIFT) == \
				PHY_TXC1_BW_40MHZ) || (((rspec & RSPEC_BW_MASK) >> \
				RSPEC_BW_SHIFT) == PHY_TXC1_BW_40MHZ_DUP))
#define RSPEC_ISSGI(rspec)	((rspec & RSPEC_SHORT_GI) == RSPEC_SHORT_GI)
#define RSPEC_MIMOPLCP3(rspec)	((rspec & 0xf00000) >> 16)
#define PLCP3_ISSGI(plcp)	(plcp & (RSPEC_SHORT_GI >> 16))
#define RSPEC_STC(rspec)	((rspec & RSPEC_STC_MASK) >> RSPEC_STC_SHIFT)
#define RSPEC_STF(rspec)	((rspec & RSPEC_STF_MASK) >> RSPEC_STF_SHIFT)
#define PLCP3_ISSTBC(plcp)	((plcp & (RSPEC_STC_MASK) >> 16) == 0x10)
#define PLCP3_STC_MASK          0x30
#define PLCP3_STC_SHIFT         4

/* Rate info table; takes a legacy rate or ratespec_t */
#ifdef WL11N
#define	IS_MCS(r)     	(r & RSPEC_MIMORATE)
#else /* WL11N */
#define	IS_MCS(r)     	0
#endif /* WL11N */
#define	IS_OFDM(r)     	(!IS_MCS(r) && (rate_info[(r) & RSPEC_RATE_MASK] & WLC_RATE_FLAG))
#define	IS_CCK(r)	(!IS_MCS(r) && (((r) & RATE_MASK) == WLC_RATE_1M || \
			 ((r) & RATE_MASK) == WLC_RATE_2M || \
			 ((r) & RATE_MASK) == WLC_RATE_5M5 || ((r) & RATE_MASK) == WLC_RATE_11M))
#define IS_STBC(r)	(RSPEC_STF(rspec) == PHY_TXC1_MODE_STBC)
#define IS_SINGLE_STREAM(mcs)	(((mcs) <= HIGHEST_SINGLE_STREAM_MCS) || ((mcs) == 32))
#define CCK_RSPEC(cck)		((cck) & RSPEC_RATE_MASK)
#define OFDM_RSPEC(ofdm)	(((ofdm) & RSPEC_RATE_MASK) |\
	(PHY_TXC1_MODE_CDD << RSPEC_STF_SHIFT))
#define LEGACY_RSPEC(rate)	(IS_CCK(rate) ? CCK_RSPEC(rate) : OFDM_RSPEC(rate))

#define MCS_RSPEC(mcs)		(((mcs) & RSPEC_RATE_MASK) | RSPEC_MIMORATE | \
	(IS_SINGLE_STREAM(mcs) ? (PHY_TXC1_MODE_CDD << RSPEC_STF_SHIFT) : \
	(PHY_TXC1_MODE_SDM << RSPEC_STF_SHIFT)))

/* Convert encoded rate value in plcp header to numerical rates in 500 KHz increments */
#ifdef BCMROMBUILD
extern const uint8 *wlc_phy_get_ofdm_rate_lookup(void);
#define OFDM_PHY2MAC_RATE(rlpt)         ((wlc_phy_get_ofdm_rate_lookup())[rlpt & 0x7])
#else
extern const uint8 ofdm_rate_lookup[];
#define OFDM_PHY2MAC_RATE(rlpt)		(ofdm_rate_lookup[rlpt & 0x7])
#endif
#define CCK_PHY2MAC_RATE(signal)	(signal/5)

/* Rates specified in wlc_rateset_filter() */
#define WLC_RATES_CCK_OFDM	0
#define WLC_RATES_CCK		1
#define WLC_RATES_OFDM		2

/* use the stuct form instead of typedef to fix dependency problems */
struct wlc_rateset;

/* sanitize, and sort a rateset with the basic bit(s) preserved, validate rateset */
extern bool wlc_rate_hwrs_filter_sort_validate(struct wlc_rateset *rs,
                                               const struct wlc_rateset *hw_rs,
                                               bool check_brate, uint8 txstreams);
/* copy rateset src to dst as-is (no masking or sorting) */
extern void wlc_rateset_copy(const struct wlc_rateset *src, struct wlc_rateset *dst);

/* would be nice to have these documented ... */
extern ratespec_t wlc_recv_compute_rspec(wlc_d11rxhdr_t *wrxh, uint8 *plcp);

extern void wlc_rateset_filter(struct wlc_rateset *src, struct wlc_rateset *dst,
	bool basic_only, uint8 rates, uint xmask, bool mcsallow);
extern void wlc_rateset_ofdm_fixup(struct wlc_rateset *rs);
extern void wlc_rateset_default(struct wlc_rateset *rs_tgt, const struct wlc_rateset *rs_hw,
	uint phy_type, int bandtype, bool cck_only, uint rate_mask, bool mcsallow, uint8 bw,
uint8 txstreams);
extern int16 wlc_rate_legacy_phyctl(uint rate);

extern int wlc_dump_rateset(const char *name, struct wlc_rateset *rateset, struct bcmstrbuf *b);
extern int wlc_dump_mcsset(const char *name, uchar *mcs, struct bcmstrbuf *b);
extern ratespec_t wlc_get_highest_rate(struct wlc_rateset *rateset, bool isbw40, bool sgi,
	uint8 txstreams);

#ifdef WL11N
extern void wlc_rateset_mcs_upd(struct wlc_rateset *rs, uint8 txstreams);
extern void wlc_rateset_mcs_clear(struct wlc_rateset *rateset);
extern void wlc_rateset_mcs_build(struct wlc_rateset *rateset, uint8 txstreams);
extern void wlc_rateset_bw_mcs_filter(struct wlc_rateset *rateset, uint8 bw);
#endif /* WL11N */
extern bool wlc_bss_membership_filter(struct wlc_rateset *rs);
extern void wlc_rateset_merge(struct wlc_rateset *rs1, struct wlc_rateset *rs2);

#endif	/* _WLC_RATE_H_ */
