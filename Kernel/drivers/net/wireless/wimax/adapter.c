// adapter.c

#include "headers.h"

/* 
    Global variables
*/
PMINIPORT_ADAPTER  g_Adapters[ADAPTER_MAX_NUMBER];
BOOLEAN		   g_HaltedFlags[ADAPTER_MAX_NUMBER];
// initalize
VOID adpInit(VOID)
{
   ENTER;

    memset(g_Adapters, 0, sizeof(PMINIPORT_ADAPTER) * ADAPTER_MAX_NUMBER);
    memset(g_HaltedFlags, 0, sizeof(BOOLEAN) * ADAPTER_MAX_NUMBER);
	LEAVE;
}


// save adapter to first free index
ULONG adpSave(PMINIPORT_ADAPTER Adapter)
{
    int		i;
    PMINIPORT_ADAPTER*	p = g_Adapters;

	ENTER;
    for (i = 0; i < ADAPTER_MAX_NUMBER; i++, p++) {
	if (*p == NULL) {
		DumpDebug(HARDWARE, "Adapter (%lu) saved to %d index", (unsigned long)Adapter, i);
	    // initialize
	    *p = Adapter;
	    return i;
	}
    }
	LEAVE;

    return (ULONG)(-1);
}

// remove adapter from array
VOID adpRemove(PMINIPORT_ADAPTER Adapter)
{
    int		i;
    PMINIPORT_ADAPTER*	p = g_Adapters;

	ENTER;
    for (i = 0; i < ADAPTER_MAX_NUMBER; i++, p++) {
	if (*p == Adapter) {
	    DumpDebug(HARDWARE, " Adapter (%lu) removed from %d index", (unsigned long)Adapter, i);
	    // clear
	    *p = NULL;
	    break;
	}
    }
	LEAVE;
}
// sangam dbg : Not usefull as adpsave is success.
// get adapter index
ULONG adpIndex(PMINIPORT_ADAPTER Adapter)
{
    int		i;
    PMINIPORT_ADAPTER*	p = g_Adapters;

	ENTER;
    for (i = 0; i < ADAPTER_MAX_NUMBER; i++, p++) {
	if (*p == Adapter) {
	    // found
	    DumpDebug(HARDWARE, "Index is %d", i);
	    return i;
	}
    }
	LEAVE;
    return (ULONG)(-1);
}
/*
// get adapter by DeviceObject
PMINIPORT_ADAPTER adpGetByObject(PDEVICE_OBJECT Object)
{
    ULONG		i;
    PMINIPORT_ADAPTER*	p = g_Adapters;

    PRINT_TRACE((MINIPORT_PRINT_NAME": "__FUNCTION__"++\n"));

    // check first one
    if (*p != NULL) {
	if (((*p)->ctl.Apps.DeviceObject == Object) || 
	    ((*p)->ctl.Serial.DeviceObject == Object)) {
		PRINT_INFO((MINIPORT_PRINT_NAME": "__FUNCTION__" Adapter found on 0 index\n"));
		return *p;
	}
    }

    p++;    
    // check others
    for (i = 1; i < ADAPTER_MAX_NUMBER; i++, p++) {
	if (*p != NULL) {
	    if (((*p)->ctl.Apps.DeviceObject == Object) || 
		((*p)->ctl.Serial.DeviceObject == Object)) {
		    PRINT_INFO((MINIPORT_PRINT_NAME": "__FUNCTION__" Adapter found on %d index\n", i));
		    return *p;
	    }
	}
    }

    PRINT_TRACE((MINIPORT_PRINT_NAME": "__FUNCTION__"--\n"));

    return NULL;
}
*/
// sangam dbg : used by the set and query info but linux not supported
// is adapter halted?
BOOLEAN adpIsHalted(PMINIPORT_ADAPTER Adapter)
{
    int		i;
    PMINIPORT_ADAPTER*	p = g_Adapters;

	ENTER;
    // check first one
    if (*p == Adapter) {
	// found
		if (g_HaltedFlags[0]) {
		    DumpDebug(HARDWARE, " Adapter (%lu, 0) halted: YES", (unsigned long)Adapter);
		}
		return g_HaltedFlags[0];
    }

    p++;
    // check others
    for (i = 1; i < ADAPTER_MAX_NUMBER; i++, p++) {
	if (*p == Adapter) {
	    // found
	    if (g_HaltedFlags[i]) {
			DumpDebug(HARDWARE, " Adapter (%lu, %d) halted: YES", (unsigned long)Adapter, i);
	    }
	    return g_HaltedFlags[i];
	}
    }
    DumpDebug(HARDWARE, "Adapter (%lu) halted (removed): YES", (unsigned long)Adapter);

    // not found, assume that it was removed already
    return TRUE;
}
