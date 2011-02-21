#ifndef _WIBRO_ADAPTER_H
#define _WIBRO_ADAPTER_H

#define ADAPTER_MAX_NUMBER		128

/*
    Types
*/

/*
    Functions
*/
// initalize
VOID adpInit(VOID);

// save adapter to first free index
ULONG adpSave(PMINIPORT_ADAPTER Adapter);

// remove adapter from array
VOID adpRemove(PMINIPORT_ADAPTER Adapter);

// get adapter index
ULONG adpIndex(PMINIPORT_ADAPTER Adapter);

// get adapter by DeviceObject
PMINIPORT_ADAPTER adpGetByObject(void); // sangam dbg: need to add later for the comparision

// is adapter halted?
BOOLEAN adpIsHalted(PMINIPORT_ADAPTER Adapter);

/*
    Macros
*/
// set as halted
#define adpSetHaltFlag(i, f)	g_HaltedFlags[i] = f;


/*
    Variable
*/
extern BOOLEAN g_HaltedFlags[ADAPTER_MAX_NUMBER];

#endif
