#ifndef S6E63M0_H
#define S6E63M0_H

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

extern const unsigned short s6e63m0_SEQ_DISPLAY_ON[];
extern const unsigned short s6e63m0_SEQ_DISPLAY_OFF[];
extern const unsigned short s6e63m0_SEQ_STANDBY_ON[];
extern const unsigned short s6e63m0_SEQ_STANDBY_OFF[];
extern const unsigned short s6e63m0_SEQ_SETTING[];
extern const unsigned short gamma_update[];
extern const unsigned short s6e63m0_22gamma_300cd[];
extern const unsigned short s6e63m0_22gamma_290cd[];
extern const unsigned short s6e63m0_22gamma_280cd[];
extern const unsigned short s6e63m0_22gamma_270cd[];
extern const unsigned short s6e63m0_22gamma_260cd[];
extern const unsigned short s6e63m0_22gamma_250cd[];
extern const unsigned short s6e63m0_22gamma_240cd[];
extern const unsigned short s6e63m0_22gamma_230cd[];
extern const unsigned short s6e63m0_22gamma_220cd[];
extern const unsigned short s6e63m0_22gamma_210cd[];
extern const unsigned short s6e63m0_22gamma_200cd[];
extern const unsigned short s6e63m0_22gamma_190cd[];
extern const unsigned short s6e63m0_22gamma_180cd[];
extern const unsigned short s6e63m0_22gamma_170cd[];
extern const unsigned short s6e63m0_22gamma_160cd[];
extern const unsigned short s6e63m0_22gamma_150cd[];
extern const unsigned short s6e63m0_22gamma_140cd[];
extern const unsigned short s6e63m0_22gamma_130cd[];
extern const unsigned short s6e63m0_22gamma_120cd[];
extern const unsigned short s6e63m0_22gamma_110cd[];
extern const unsigned short s6e63m0_22gamma_100cd[];
extern const unsigned short s6e63m0_22gamma_90cd[];
extern const unsigned short s6e63m0_22gamma_80cd[];
extern const unsigned short s6e63m0_22gamma_70cd[];
extern const unsigned short s6e63m0_22gamma_60cd[];
extern const unsigned short s6e63m0_22gamma_50cd[];
extern const unsigned short s6e63m0_22gamma_40cd[];
extern const unsigned short s6e63m0_22gamma_30cd[];
#if defined(CONFIG_ARIES_NTT) // Modify NTTS1
extern const unsigned short s6e63m0_22gamma_20cd[];
#endif

extern const unsigned short s6e63m0_19gamma_300cd[];
extern const unsigned short s6e63m0_19gamma_290cd[];
extern const unsigned short s6e63m0_19gamma_280cd[];
extern const unsigned short s6e63m0_19gamma_270cd[];
extern const unsigned short s6e63m0_19gamma_260cd[];
extern const unsigned short s6e63m0_19gamma_250cd[];
extern const unsigned short s6e63m0_19gamma_240cd[];
extern const unsigned short s6e63m0_19gamma_230cd[];
extern const unsigned short s6e63m0_19gamma_220cd[];
extern const unsigned short s6e63m0_19gamma_210cd[];
extern const unsigned short s6e63m0_19gamma_200cd[];
extern const unsigned short s6e63m0_19gamma_190cd[];
extern const unsigned short s6e63m0_19gamma_180cd[];
extern const unsigned short s6e63m0_19gamma_170cd[];
extern const unsigned short s6e63m0_19gamma_160cd[];
extern const unsigned short s6e63m0_19gamma_150cd[];
extern const unsigned short s6e63m0_19gamma_140cd[];
extern const unsigned short s6e63m0_19gamma_130cd[];
extern const unsigned short s6e63m0_19gamma_120cd[];
extern const unsigned short s6e63m0_19gamma_110cd[];
extern const unsigned short s6e63m0_19gamma_100cd[];
extern const unsigned short s6e63m0_19gamma_90cd[];
extern const unsigned short s6e63m0_19gamma_80cd[];
extern const unsigned short s6e63m0_19gamma_70cd[];
extern const unsigned short s6e63m0_19gamma_60cd[];
extern const unsigned short s6e63m0_19gamma_50cd[];
extern const unsigned short s6e63m0_19gamma_40cd[];
extern const unsigned short s6e63m0_19gamma_30cd[];
#if defined(CONFIG_ARIES_NTT) // Modify NTTS1
extern const unsigned short s6e63m0_19gamma_20cd[];
#endif

#ifdef CONFIG_FB_S3C_TL2796_ACL
extern const unsigned short acl_cutoff_off[];
extern const unsigned short acl_cutoff_init[];
extern const unsigned short acl_cutoff_8p[];
extern const unsigned short acl_cutoff_14p[];
extern const unsigned short acl_cutoff_20p[];
extern const unsigned short acl_cutoff_24p[];
extern const unsigned short acl_cutoff_28p[];
extern const unsigned short acl_cutoff_32p[];
extern const unsigned short acl_cutoff_35p[];
extern const unsigned short acl_cutoff_37p[];
extern const unsigned short acl_cutoff_38p[];
extern const unsigned short acl_cutoff_40p[];
extern const unsigned short acl_cutoff_43p[];
extern const unsigned short acl_cutoff_45p[];
extern const unsigned short acl_cutoff_47p[];
extern const unsigned short acl_cutoff_48p[];
extern const unsigned short acl_cutoff_50p[];
extern const unsigned short acl_cutoff_60p[];
extern const unsigned short acl_cutoff_75p[];
#endif /* CONFIG_FB_S3C_TL2796_ACL */

#endif /* S6E63M0_H */
