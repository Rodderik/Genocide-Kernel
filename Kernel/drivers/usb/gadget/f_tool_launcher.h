/*
 * Gadget Driver for Android MTP
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 *
 */

//Header file for Tool Launcher.

//#define FEATURE_TOOL_LAUNCHER
#ifdef FEATURE_TOOL_LAUNCHER
#define DEBUG_TL_MSG 1
#endif

#define TOOL_LAUNCHER_STATE_DISABLED 0
#define TOOL_LAUNCHER_STATE_ENABLED_READY 1
#define TOOL_LAUNCHER_STATE_ENABLED_DONE 2
#ifdef FEATURE_TOOL_LAUNCHER
extern void set_tool_launcher_mode();
#endif


#ifdef DEBUG_TL_MSG
#define DEBUG_TL(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_TL(fmt,args...) do {} while(0)
#endif
