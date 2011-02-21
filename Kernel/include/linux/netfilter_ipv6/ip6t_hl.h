/* ip6tables module for matching the Hop Limit value
 * Maciej Soltysiak <solt@dns.toxicfilms.tv>
 * Based on HW's ttl module */

#ifndef _IP6T_HL_H
#define _IP6T_HL_H

enum {
	IP6T_HL_EQ = 0,		/* equals */
	IP6T_HL_NE,		/* not equals */
	IP6T_HL_LT,		/* less than */
	IP6T_HL_GT,		/* greater than */
};
#if defined CONFIG_S5PV210_VICTORY
enum {
	IP6T_HL_SET = 0,
	IP6T_HL_INC,
	IP6T_HL_DEC
};

#define IP6T_HL_MAXMODE	IP6T_HL_DEC
#endif
struct ip6t_hl_info {
	u_int8_t	mode;
	u_int8_t	hop_limit;
};
#if defined CONFIG_S5PV210_VICTORY
struct ip6t_HL_info {
	u_int8_t	mode;
	u_int8_t	hop_limit;
};
#endif
#endif
