#ifndef _XT_TCPMSS_H
#define _XT_TCPMSS_H

#include <linux/types.h>

struct xt_tcpmss_info {
	__u16 mss;
};

#define XT_TCPMSS_CLAMP_PMTU 0xffff

#endif /* _XT_TCPMSS_H */
#if defined CONFIG_S5PV210_VICTORY
#ifndef _XT_TCPMSS_MATCH_H
#define _XT_TCPMSS_MATCH_H

struct xt_tcpmss_match_info {
    u_int16_t mss_min, mss_max;
    u_int8_t invert;
};

#endif /*_XT_TCPMSS_MATCH_H*/
#endif
