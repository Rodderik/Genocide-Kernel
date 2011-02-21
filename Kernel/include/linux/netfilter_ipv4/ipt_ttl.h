/* IP tables module for matching the value of the TTL
 * (C) 2000 by Harald Welte <laforge@gnumonks.org> */

#ifndef _IPT_TTL_H
#define _IPT_TTL_H

enum {
	IPT_TTL_EQ = 0,		/* equals */
	IPT_TTL_NE,		/* not equals */
	IPT_TTL_LT,		/* less than */
	IPT_TTL_GT,		/* greater than */
};
#if defined CONFIG_S5PV210_VICTORY
enum {
	IPT_TTL_SET = 0,
	IPT_TTL_INC,
	IPT_TTL_DEC
};

#define IPT_TTL_MAXMODE	IPT_TTL_DEC
#endif
struct ipt_ttl_info {
	u_int8_t	mode;
	u_int8_t	ttl;
};
#if defined CONFIG_S5PV210_VICTORY
struct ipt_TTL_info {
	u_int8_t	mode;
	u_int8_t	ttl;
};
#endif
#endif
