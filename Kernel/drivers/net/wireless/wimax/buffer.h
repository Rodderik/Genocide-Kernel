#ifndef _WIBRO_BUFFER_H
#define _WIBRO_BUFFER_H

/*sangam : Used Q_Send for the control and data packet sending, uses the BUFFER_DESCRIPTOR
				Q_Received used for the control packet receive, data packet sent directly
*/

/*
    Types
*/
typedef struct {
    LIST_ENTRY		    Node;		// list node
    PVOID		    Buffer;		// allocated buffer: sangam It is used for copying and transfer
    struct {
	PUCHAR		    Data;		// current data start pointer : sangam - not used now in linux
	ULONG		    Length;		// current data length
	USHORT		    Type;		// buffer type (used for control buffers)
    } data;
    struct {		//sangam - not needed driver sending skb directly
	PVOID		    CancelId;		// cancel ID
	PNDIS_PACKET	Packet;		// skb packet
	PNDIS_BUFFER	Buffer;			// NDIS buffer
    } ndis;
} BUFFER_DESCRIPTOR, *PBUFFER_DESCRIPTOR;

typedef struct {
//    NPAGED_LOOKASIDE_LIST   DescrPool;		// descriptor pool
    NDIS_HANDLE		    BufferPool;		// buffer pool
    NDIS_HANDLE		    PacketPool;		// packet pool
    BOOLEAN		    Allocated;		// allocated flag
    LIST_ENTRY		    Head;		// list head
    ULONG		    Count;		// current number of free buffers in list
    NDIS_SPIN_LOCK	    Lock;		// protection
} FREE_QUEUE_INFO, *PFREE_QUEUE_INFO;

typedef struct {
    LIST_ENTRY		    Head;		// list head
    ULONG		    Count;		// current number of used buffers in list
    BOOLEAN		    Inited;		// inited flag
    NDIS_SPIN_LOCK	    Lock;		// protection
} USED_QUEUE_INFO, *PUSED_QUEUE_INFO;

/******************************************************************************
 *  QueueInitList -- Macro which will initialize a queue to NULL.
 ******************************************************************************/
#define QueueInitList(_L) ((_L).next = (_L).prev = NULL);


/******************************************************************************
 *  QueueEmpty -- Macro which checks to see if a queue is empty.
 ******************************************************************************/
#define QueueEmpty(_L) (QueueGetHead((_L)) == NULL)


/******************************************************************************
 *  QueueGetHead -- Macro which returns the head of the queue, but does not
 *  remove the head from the queue.
 ******************************************************************************/
#define QueueGetHead(_L) ((PLIST_ENTRY)((_L).next))


/******************************************************************************
 *  QueuePushHead -- Macro which puts an element at the head of the queue.
 ******************************************************************************/
/*
#define QueuePushHead(_L,_E) \
	ASSERT(_L); \
	ASSERT(_E); \
	if (!((_E)->Link.next = (_L)->Link.next)) \
	{ \
		(_L)->Link.prev = (PLIST_ENTRY)(_E); \
	} \
	(_L)->Link.next = (PLIST_ENTRY)(_E);
*/


/******************************************************************************
 *  QueueRemoveHead -- Macro which removes the head of the head of queue.
 ******************************************************************************/
#define QueueRemoveHead(_L)	\
{	\
	PLIST_ENTRY ListElem;	\
	ASSERT(&(_L));	\
	if ((ListElem = (PLIST_ENTRY)(_L).next)) /* then fix up our our list to point to next elem */	\
	{	\
		if(!((_L).next = ListElem->next)) /* rechain list pointer to next link */	\
			/* if the list pointer is null, null out the reverse link */	\
			(_L).prev = NULL;	\
	}	\
}


/******************************************************************************
 *  QueuePutTail -- Macro which puts an element at the tail (end) of the queue.
 ******************************************************************************/
#define QueuePutTail(_L,_E) \
{	\
	ASSERT(&(_L));	\
	ASSERT(&(_E));	\
	if ((_L).prev) 	\
	{ \
		((PLIST_ENTRY)(_L).prev)->next = (PLIST_ENTRY)(&(_E)); \
		(_L).prev = (PLIST_ENTRY)(&(_E)); \
	} \
	else 	\
	{ \
		(_L).next = (_L).prev = (PLIST_ENTRY)(&(_E)); \
	} \
	(_E).next = NULL;	\
}


/******************************************************************************
 *  QueueGetTail -- Macro which returns the tail of the queue, but does not
 *  remove the tail from the queue.
 ******************************************************************************/
/*
#define QueueGetTail(_L) ((PBCM_LIST_ENTRY)((_L)->Link.prev))
*/


//-------------------------------------------------------------------------
// QueuePopHead -- Macro which  will pop the head off of a queue (list), and
//                 return it (this differs only from queueremovehead only in the 1st line)
//-------------------------------------------------------------------------
#define QueuePopHead(_L) \
	(PLIST_ENTRY) (_L).next; QueueRemoveHead(_L);

#endif	// _WBRB_QUEUES_H

/* vim: set tabstop=4 shiftwidth=4: */
