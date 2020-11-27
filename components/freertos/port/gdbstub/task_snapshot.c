#include "task_snapshot.h"

static void prvTaskGetSnapshot( TaskSnapshot_t *pxTaskSnapshotArray, UBaseType_t *uxTask, TCB_t *pxTCB )
{
    if (pxTCB == NULL) {
        return;
    }
    pxTaskSnapshotArray[ *uxTask ].pxTCB = pxTCB;
    pxTaskSnapshotArray[ *uxTask ].pxTopOfStack = (StackType_t *)pxTCB->pxTopOfStack;
    #if( portSTACK_GROWTH < 0 )
    {
        pxTaskSnapshotArray[ *uxTask ].pxEndOfStack = pxTCB->pxEndOfStack;
    }
    #else
    {
        pxTaskSnapshotArray[ *uxTask ].pxEndOfStack = pxTCB->pxStack;
    }
    #endif
    (*uxTask)++;
}

static void prvTaskGetSnapshotsFromList( TaskSnapshot_t *pxTaskSnapshotArray, UBaseType_t *uxTask, const UBaseType_t uxArraySize, List_t *pxList )
{
    TCB_t *pxNextTCB, *pxFirstTCB;

    if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
    {
        listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );
        do
        {
            if( *uxTask >= uxArraySize )
                break;

            listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
            prvTaskGetSnapshot( pxTaskSnapshotArray, uxTask, pxNextTCB );
        } while( pxNextTCB != pxFirstTCB );
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}

UBaseType_t uxTaskGetSnapshotAll( TaskSnapshot_t * const pxTaskSnapshotArray, const UBaseType_t uxArraySize, UBaseType_t * const pxTcbSz )
{
    UBaseType_t uxTask = 0, i = 0;


    *pxTcbSz = sizeof(TCB_t);
    /* Fill in an TaskStatus_t structure with information on each
    task in the Ready state. */
    i = configMAX_PRIORITIES;
    do
    {
        i--;
        prvTaskGetSnapshotsFromList( pxTaskSnapshotArray, &uxTask, uxArraySize, &( pxReadyTasksLists[ i ] ) );
    } while( i > ( UBaseType_t ) tskIDLE_PRIORITY ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

    /* Fill in an TaskStatus_t structure with information on each
    task in the Blocked state. */
    prvTaskGetSnapshotsFromList( pxTaskSnapshotArray, &uxTask, uxArraySize, ( List_t * ) pxDelayedTaskList );
    prvTaskGetSnapshotsFromList( pxTaskSnapshotArray, &uxTask, uxArraySize, ( List_t * ) pxOverflowDelayedTaskList );
    for (i = 0; i < portNUM_PROCESSORS; i++) {
        if( uxTask >= uxArraySize )
            break;
        prvTaskGetSnapshotsFromList( pxTaskSnapshotArray, &uxTask, uxArraySize, &xPendingReadyList );
    }

    #if( INCLUDE_vTaskDelete == 1 )
    {
        prvTaskGetSnapshotsFromList( pxTaskSnapshotArray, &uxTask, uxArraySize, &xTasksWaitingTermination );
    }
    #endif

    #if ( INCLUDE_vTaskSuspend == 1 )
    {
        prvTaskGetSnapshotsFromList( pxTaskSnapshotArray, &uxTask, uxArraySize, &xSuspendedTaskList );
    }
    #endif
    return uxTask;
}