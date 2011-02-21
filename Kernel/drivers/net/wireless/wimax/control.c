#include "headers.h"
#include "ctl_funcs.h"
#include "ethernet.h"

VOID controlEnqueueReceivedBuffer(MINIPORT_ADAPTER *Adapter, USHORT Type, PVOID Buffer, ULONG Length);
#ifdef WIBRO_USB_SELECTIVE_SUSPEND
static BOOLEAN controlIsWakeupPacket(MINIPORT_ADAPTER *Adapter, PVOID Buffer, ULONG Length);
#endif

extern int isDumpEnabled(void);

unsigned int controlInit(MINIPORT_ADAPTER *Adapter)
{
	ENTER;
	// Receive control packets
	QueueInitList(Adapter->ctl.Q_Received.Head);
	spin_lock_init(&Adapter->ctl.Q_Received.Lock);
	
	QueueInitList(Adapter->ctl.Apps.ProcessList);
	spin_lock_init(&Adapter->ctl.Apps.Lock);
	Adapter->ctl.Apps.Inited = TRUE;

	Adapter->SndQLimit = MINIPORT_MAX_SEND_PACKETS_DEFAULT;
	Adapter->RcvQLimit = MINIPORT_MAX_RECV_PACKETS_DEFAULT;
	Adapter->FullQLimit = ((Adapter->SndQLimit + Adapter->RcvQLimit) / 2) - 1;

	LEAVE;
	return STATUS_SUCCESS;
}

void controlRemove(PMINIPORT_ADAPTER Adapter)
{
	PCONTROL_PROCESS_DESCRIPTOR process;
	PBUFFER_DESCRIPTOR dsc;
    ENTER;

	// Free the received control packets queue
	while(!QueueEmpty(Adapter->ctl.Q_Received.Head)) 
	{		
	 // queue is not empty
		DumpDebug(DISPATCH, "<1> Freeing Control Receive Queue");
		dsc = (PBUFFER_DESCRIPTOR)QueueGetHead(Adapter->ctl.Q_Received.Head);
		if (!dsc) {
			DumpDebug(DISPATCH, "<1> Fail...node is null");
			continue;
		}
		QueueRemoveHead(Adapter->ctl.Q_Received.Head);
		--Adapter->stats.tx.Control;
		if(dsc->Buffer)
			kfree(dsc->Buffer);
		if(dsc)
			kfree(dsc);
	}
	// process list	
	// sangam : appln read indicates error status -ENODEV
	if(Adapter->ctl.Apps.Inited) {
		if(!QueueEmpty(Adapter->ctl.Apps.ProcessList)) {	
			// first time gethead needed to get the dsc nodes
			process = (PCONTROL_PROCESS_DESCRIPTOR) QueueGetHead(Adapter->ctl.Apps.ProcessList);	
			while (process != NULL)	{
				if(process->Irp) {	// sangam : process read waiting
					process->Irp = FALSE;
					wake_up_interruptible(&process->read_wait);
				}
				process = (PCONTROL_PROCESS_DESCRIPTOR)process->Node.next;
				DumpDebug (TX_CONTROL,"sangam dbg : waking processes");
			}
	//*************** SANGAM check if needed *****************************
/*	
   			while(!QueueEmpty(Adapter->ctl.Apps.ProcessList)) 	
				DumpDebug (TX_CONTROL,"still waiting for process close");
				msleep(1);
			}
*/
		msleep(100);	//delay for the process release
		}
	}
	Adapter->ctl.Apps.Inited = FALSE;

	LEAVE;
	return;
}

#ifdef WIBRO_USB_SELECTIVE_SUSPEND
BOOLEAN controlIsWakeupPacket(MINIPORT_ADAPTER *Adapter, void *Buffer, unsigned long Length)
{
	PCONTROL_ETHERNET_HEADER hdr = (PCONTROL_ETHERNET_HEADER)Buffer; 
	ENTER;

	if(Length < sizeof (CONTROL_ETHERNET_HEADER))
	{
		DumpDebug (TX_CONTROL,"Packet is too short");
		return FALSE;
	}
	if (hdr->Type == CONTROL_ETH_TYPE_WCM)
	{
		DumpDebug (TX_CONTROL,"Wakeup Control Packet");
		return TRUE;
	}
	LEAVE;
	return FALSE;
}
#endif	

// receive control data
VOID controlReceive(MINIPORT_ADAPTER   *Adapter, void *Buffer, unsigned long Length)
{
    PCONTROL_ETHERNET_HEADER    hdr;
	unsigned long flags;

	ENTER;
#if 1	// rx control frame
		if (isDumpEnabled()) {
			// dump packets
			UINT i, l = Length;
			u_char *  b = (u_char *)Buffer;
			DumpDebug(READ_REG, "Recv packet control [%d] = ", l);

			printk("<1>\x1b[1;33m[WiMAX]");
			for (i = 0; i < Length; i++) {
				printk(" %02x", b[i]);
				if (((i + 1) != Length) && (i % 16 == 15))
				{
					printk("\x1b[0m\n<1>\x1b[1;33m[WiMAX]");
				}
			}
			printk("\x1b[0m\n");
		}
#endif

    // check halt flag
    if (Adapter->bHaltPending) {
		return;
    }

    hdr = (PCONTROL_ETHERNET_HEADER)Buffer;
#if 1	//cky 20100311 him relay
	//cky test
	if (hdr->Type == 0x2115)	// relay type
	{
		DumpDebug(TX, "recv: 0x1521 -> 0x1500");
		hdr->Type = 0x15;
	}
#endif

    spin_lock_irqsave(&Adapter->ctl.Q_Received.Lock, flags); 
    
	// not found, add to pending buffers list
	controlEnqueueReceivedBuffer(Adapter, hdr->Type, Buffer, Length);

//	spin_unlock_irqrestore(&Adapter->Control.Lock, flags);
	spin_unlock_irqrestore(&Adapter->ctl.Q_Received.Lock, flags);
	LEAVE;
}

// add received packet to pending list
VOID controlEnqueueReceivedBuffer(MINIPORT_ADAPTER *Adapter, USHORT Type, PVOID Buffer, ULONG Length) 
{
    PBUFFER_DESCRIPTOR  dsc;
	PCONTROL_PROCESS_DESCRIPTOR process;

	ENTER;
	// Queue and wake read only if process exist.
	process = controlFindProcessByType(Adapter,Type);
	if(process) {
		dsc = (PBUFFER_DESCRIPTOR)kmalloc(sizeof(BUFFER_DESCRIPTOR),GFP_ATOMIC);
		if(dsc == NULL)
		{
			DumpDebug (TX_CONTROL,"dsc Memory Alloc Failure *****");
			return;
		}
		dsc->Buffer = kmalloc(Length,GFP_ATOMIC);
		if(dsc->Buffer == NULL) {
			kfree(dsc);
			DumpDebug (TX_CONTROL,"Memory Alloc Failure *****");
			return;
		}

		memcpy(dsc->Buffer, Buffer, Length);
		// fill out descriptor
  		dsc->data.Length = Length;
		dsc->data.Type   = Type;
 		// add to pending list
		 QueuePutTail(Adapter->ctl.Q_Received.Head, dsc->Node)

		if(process->Irp)
		{
			process->Irp = FALSE;
			wake_up_interruptible(&process->read_wait);
		}
	}
	else
		DumpDebug (TX_CONTROL,"Waiting process not found skip the packet");
		
	LEAVE;	
}

unsigned int hwSendControl(MINIPORT_ADAPTER *Adapter, void *Buffer, u_long Length)
{
	PUCHAR pTempBuf;
	PHW_PACKET_HEADER hdr;
	PBUFFER_DESCRIPTOR   dsc;
	ULONG lCount = 0;

	ENTER;
	if((Length + sizeof(HW_PACKET_HEADER)) >= MINIPORT_MAX_TOTAL_SIZE) {
		return STATUS_RESOURCES;	//changed from SUCCESS return status: sangam dbg 
	}

	lCount = bufFindCount(Adapter->hw.Q_Send.Head);
	if(lCount > Adapter->FullQLimit) {
		DumpDebug(MP_SEND, "Queue count (%lu) exceeds queue limit (%lu)", lCount, Adapter->FullQLimit);
		dsc = (PBUFFER_DESCRIPTOR)QueueGetHead(Adapter->hw.Q_Send.Head);
		if (dsc) {
			QueueRemoveHead(Adapter->hw.Q_Send.Head);
			--Adapter->stats.tx.Control;
			if(dsc->Buffer)
				kfree(dsc->Buffer);
			if(dsc)
				kfree(dsc);
		}
		else {
			DumpDebug(DISPATCH, "Error...node is null");
			return 0;
		}
	}
#ifdef WIBRO_USB_SELECTIVE_SUSPEND
	if( (Adapter->Power.DevicePowerState == PowerDeviceD2) || Adapter->Power.IdleImminent ) {
		if(controlIsWakeupPacket(Adapter, Buffer, Length)) 	{
			DumpDebug(TX_CONTROL, "WCM data awake device\n");
			Adapter->Power.DevicePowerState = PowerDeviceD0;
			Adapter->Power.IdleImminent = FALSE;
		}
		else {
			DumpDebug(TX_CONTROL, "Idle Mode..Control Packet skipped");
			return STATUS_SUCCESS;	// return success to free the queue
		}
	}
#endif

	dsc = (PBUFFER_DESCRIPTOR) kmalloc(sizeof(BUFFER_DESCRIPTOR),GFP_ATOMIC);
	if(dsc == NULL){ 
		return STATUS_RESOURCES;
	}
	dsc->Buffer = kmalloc(BUFFER_DATA_SIZE, GFP_ATOMIC);  
	if(dsc->Buffer == NULL) {
		kfree(dsc);
		return STATUS_RESOURCES;
	}

	pTempBuf = dsc->Buffer;
	hdr = (PHW_PACKET_HEADER)dsc->Buffer;

	pTempBuf += sizeof(HW_PACKET_HEADER);
#ifdef HARDWARE_USE_ALIGN_HEADER
	pTempBuf += 2;
#endif

	memcpy(pTempBuf, Buffer + (MINIPORT_LENGTH_OF_ADDRESS * 2), Length -(MINIPORT_LENGTH_OF_ADDRESS * 2));

	 // add packet header 
	hdr->Id0	 = 'W';
	hdr->Id1	 = 'C';
	hdr->Length = (USHORT)Length - (MINIPORT_LENGTH_OF_ADDRESS * 2);

	// set length 
	dsc->data.Length = (USHORT)Length - (MINIPORT_LENGTH_OF_ADDRESS * 2) + sizeof(HW_PACKET_HEADER);
#ifdef HARDWARE_USE_ALIGN_HEADER	
	dsc->data.Length += 2;
#endif	

	//DumpDebug(TX_CONTROL, "sending CONTROL PACKET------------------------------------------->>>");
#if 1
	if (isDumpEnabled()) {
	    // dump packets
	    UINT i;
		u_long Length = dsc->data.Length; 
	    PUCHAR  b = (PUCHAR)dsc->Buffer;
		DumpDebug(TX_CONTROL, "Send packet control [%d] = ",Length);

		printk("<1>\x1b[1;33m[WiMAX]");
	    for (i = 0; i < Length; i++) {
			printk(" %02x", b[i]);
			if (((i + 1) != Length) && (i % 16 == 15))
			{
				printk("\x1b[0m\n<1>\x1b[1;33m[WiMAX]");
			}
	    }
		printk("\x1b[0m\n");
	}
#endif

	Adapter->stats.tx.Control++;
	QueuePutTail(Adapter->hw.Q_Send.Head, dsc->Node);
	schedule_work(&Adapter->work);

	LEAVE;
	return STATUS_SUCCESS;
}

PCONTROL_PROCESS_DESCRIPTOR controlFindProcessById(PMINIPORT_ADAPTER Adapter, UINT Id)
{
	PCONTROL_PROCESS_DESCRIPTOR process;
	PCONTROL_PROCESS_DESCRIPTOR next_process;	//cky test

	ENTER;
	if(QueueEmpty(Adapter->ctl.Apps.ProcessList)) {	
		DumpDebug(TX_CONTROL, "controlFindProcessById: Empty process list");
		return NULL;
	}
	// first time gethead needed to get the dsc nodes
	process = (PCONTROL_PROCESS_DESCRIPTOR) QueueGetHead(Adapter->ctl.Apps.ProcessList);
	while (process != NULL)	{
		if(process->Id == Id) {	// sangam : process found
			//DumpDebug(TX_CONTROL, "controlFindProcessById: process Id %lu found", process->Id);
			return process;
		}
		process = (PCONTROL_PROCESS_DESCRIPTOR)process->Node.next;
	}
	LEAVE;
	DumpDebug(TX_CONTROL, "controlFindProcessById: process not found");
	return NULL;
}

#ifdef WAKEUP_BY_GPIO
ULONG ModemUpNotiMsg(PUCHAR Buffer)
{
	ctl_pkt_t*  h = (ctl_pkt_t*)Buffer;
	him_modem_up_noti_t* m = (him_modem_up_noti_t*)(Buffer + sizeof(ctl_pkt_t));
   	h->type = be16_to_cpu(ETHERTYPE_HIM);
	h->him_type = be16_to_cpu(0x0001);
	h->subtype = be16_to_cpu(0x0007);
	h->length = be16_to_cpu(0x0002);
	m->data[0] = 0x00;
	m->data[1] = 0x02;
	return (sizeof(ctl_pkt_t) + sizeof(him_modem_up_noti_t));
}

ULONG ModemResetMsg(PUCHAR Buffer)
{
   	ctl_pkt_t* h = (ctl_pkt_t *)Buffer;
	ENTER;
	h->type = be16_to_cpu(ETHERTYPE_HIM);
	h->him_type = be16_to_cpu(0x0006);
	h->subtype = be16_to_cpu(0x0044);
	h->length = 0x0;
	LEAVE;
	return sizeof(ctl_pkt_t);
}
#endif

PCONTROL_PROCESS_DESCRIPTOR controlFindProcessByType(PMINIPORT_ADAPTER Adapter, USHORT Type)
{
	PCONTROL_PROCESS_DESCRIPTOR process;

	ENTER;
	if(QueueEmpty(Adapter->ctl.Apps.ProcessList)) {	
		DumpDebug(TX_CONTROL, "controlFindProcessByType: Empty process list");
		return NULL;
	}
	// first time gethead needed to get the dsc nodes
	process = (PCONTROL_PROCESS_DESCRIPTOR) QueueGetHead(Adapter->ctl.Apps.ProcessList);	
	while (process != NULL)	{
		if(process->Type == Type) {	// sangam : process found
			//DumpDebug(TX_CONTROL, "controlFindProcessByType: process type 0x%x found", process->Type);
			return process;
		}
		process = (PCONTROL_PROCESS_DESCRIPTOR)process->Node.next;
	}
	LEAVE;
	DumpDebug(TX_CONTROL, "controlFindProcessByType: process not found");
	return NULL;
}

void controlDeleteProcessById(PMINIPORT_ADAPTER Adapter, UINT Id)
{
	PCONTROL_PROCESS_DESCRIPTOR curElem, prevElem = NULL;

	ENTER;
	if(QueueEmpty(Adapter->ctl.Apps.ProcessList)) {	
		return ;
	}
	
	// first time get head needed to get the dsc nodes
	curElem = (PCONTROL_PROCESS_DESCRIPTOR) QueueGetHead(Adapter->ctl.Apps.ProcessList);	

	for ( ; curElem != NULL; prevElem = curElem, curElem  = (PCONTROL_PROCESS_DESCRIPTOR)curElem->Node.next) {	
		
		if(curElem->Id == Id) {	// sangam : process found
			if(prevElem == NULL) { // only one node present
				if(!((Adapter->ctl.Apps.ProcessList).next = ((PLIST_ENTRY)curElem)->next))  {/*rechain list pointer to next link*/
					DumpDebug(TX_CONTROL, "sangam dbg first and only process delete");
					(Adapter->ctl.Apps.ProcessList).prev = NULL;	
				}
			}
			else if (((PLIST_ENTRY)curElem)->next == NULL){ // last node
				DumpDebug(TX_CONTROL, "sangam dbg only last packet");
				((PLIST_ENTRY)prevElem)->next = NULL;
				(Adapter->ctl.Apps.ProcessList).prev = (PLIST_ENTRY)(prevElem);
			}
			else {	
				// middle node	
				DumpDebug(TX_CONTROL, "sangam dbg middle node");
				((PLIST_ENTRY)prevElem)->next = ((PLIST_ENTRY)curElem)->next;
			}

			if(curElem)
				kfree(curElem);
			break;
		}
	}
	LEAVE;
}

// find buffer by buffer type
unsigned long bufFindCount(LIST_ENTRY ListHead)
{
	PBUFFER_DESCRIPTOR dsc;
	unsigned long lCount = 0;

	ENTER;
	if(QueueEmpty(ListHead)) {	
		return lCount;
	}
	// first time gethead needed to get the dsc nodes
	dsc = (PBUFFER_DESCRIPTOR) QueueGetHead(ListHead);	
	while (dsc != NULL)	{
		dsc = (PBUFFER_DESCRIPTOR)dsc->Node.next;
		lCount++;
	}
	LEAVE;
	return lCount;
}

// find buffer by buffer type
PBUFFER_DESCRIPTOR bufFindByType(LIST_ENTRY ListHead, USHORT Type)
{
	PBUFFER_DESCRIPTOR dsc;

	ENTER;
	if(QueueEmpty(ListHead)) {	
		//DumpDebug(TX_CONTROL, "sangam dbg Empty buf List");
		return NULL;
	}
	// first time gethead needed to get the dsc nodes
	dsc = (PBUFFER_DESCRIPTOR) QueueGetHead(ListHead);	
	while (dsc != NULL)	{
		if(dsc->data.Type == Type) {	// sangam : process found
			//DumpDebug(TX_CONTROL, "sangam dbg return buf len = %lu  Type = 0x%x",dsc->data.Length, dsc->data.Type);
			return dsc;
		}
		dsc = (PBUFFER_DESCRIPTOR)dsc->Node.next;
	}
	LEAVE;
	return NULL;
}

// Return packet for control buffer freeing 
void hwReturnPacket(PMINIPORT_ADAPTER Adapter, USHORT Type)
{
	PBUFFER_DESCRIPTOR curElem, prevElem = NULL;

	ENTER;
	if(QueueEmpty(Adapter->ctl.Q_Received.Head)) {	
		return ;
	}
	
	// first time get head needed to get the dsc nodes
	curElem = (PBUFFER_DESCRIPTOR) QueueGetHead(Adapter->ctl.Q_Received.Head);	

	for ( ; curElem != NULL; prevElem = curElem, curElem  = (PBUFFER_DESCRIPTOR)curElem->Node.next) {	
		
		if(curElem->data.Type == Type) {	// sangam : process found
			if(prevElem == NULL)	// First node or only one node present to delete
			{
				//DumpDebug(TX_CONTROL, "sangam dbg First node");
				if(!((Adapter->ctl.Q_Received.Head).next = ((PLIST_ENTRY)curElem)->next)) /*rechain list pointer to next link*/
					/* if the list pointer is null, null out the reverse link */
					(Adapter->ctl.Q_Received.Head).prev = NULL;	
			}
			else if (((PLIST_ENTRY)curElem)->next == NULL){ // last node
				//DumpDebug(TX_CONTROL, "sangam dbg only last packet");
				((PLIST_ENTRY)prevElem)->next = NULL;
				(Adapter->ctl.Q_Received.Head).prev = (PLIST_ENTRY)(&prevElem);	
			}
			else {	
				// middle node	
					//DumpDebug(TX_CONTROL, "sangam dbg middle node");
					((PLIST_ENTRY)prevElem)->next = ((PLIST_ENTRY)curElem)->next;
			}

			if(curElem->Buffer)
				kfree(curElem->Buffer);
			if(curElem)
				kfree(curElem);
			break;
		}
	}
	LEAVE;
}
