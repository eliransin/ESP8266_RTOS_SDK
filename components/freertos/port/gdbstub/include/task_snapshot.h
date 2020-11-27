#ifndef TASK_SNAPSHOT_H
#define TASK_SNAPSHOT_H
#include "FreeRTOS.h"
#include "task.h"

/**
 * Used with the uxTaskGetSnapshotAll() function to save memory snapshot of each task in the system.
 * We need this struct because TCB_t is defined (hidden) in tasks.c.
 */
typedef struct xTASK_SNAPSHOT
{
	void        *pxTCB;         /*!< Address of task control block. */
	StackType_t *pxTopOfStack;  /*!< Points to the location of the last item placed on the tasks stack. */
	StackType_t *pxEndOfStack;  /*!< Points to the end of the stack. pxTopOfStack < pxEndOfStack, stack grows hi2lo
									pxTopOfStack > pxEndOfStack, stack grows lo2hi*/
} TaskSnapshot_t;

/*
 * This function fills array with TaskSnapshot_t structures for every task in the system.
 * Used by core dump facility to get snapshots of all tasks in the system.
 * Only available when configENABLE_TASK_SNAPSHOT is set to 1.
 * @param pxTaskSnapshotArray Pointer to array of TaskSnapshot_t structures to store tasks snapshot data.
 * @param uxArraySize Size of tasks snapshots array.
 * @param pxTcbSz Pointer to store size of TCB.
 * @return Number of elements stored in array.
 */
UBaseType_t uxTaskGetSnapshotAll( TaskSnapshot_t * const pxTaskSnapshotArray, const UBaseType_t uxArraySize, UBaseType_t * const pxTcbSz );




#endif