/*!
 *****************************************************************************************
  *
 * @file    IPCHandler.h
 *
 * @brief   Fast handler for IPC communication
 *          Functionalities:
 *          - Initialize an IPC handle for each task
 *          - Message format structure IPC_Msg_t
 *          - Wrapper functions for send and receive operations
 *          - Error numbers IPC_Error_t
 *
 * @detail  This IPC handler can be used in time-critical systems where FreeRTOS mechanisms
 *          like xQueue would take too much time to process incoming data. It is configurable
 *          via definitions in the header file and transmits data as generically as possible
 *          without reducing performance.
 *
 *          A handler works like a fifo queue. Data can be sent (written to the queue) or
 *          received (read from the queue).
 *
 *          The handler can transmit data between tasks by sotring the data in a static
 *          buffer and informing the waiting task via direct task notification.
 *          
 *          Every task has got it's own handler. A task will
 *              - receive data from their own handler
 *              - send data to another task via the other task's handler.
 *
 * @author  Jasmin Curtz
 *
 * @version @v{ $Revision: 1.1 $ }
 *
 * @date    28.05.2021 - [Jasmin Curtz] - first version
 *
 *****************************************************************************************
 */
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "IPCHandler.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IPC_HANDLER_CNT_MAX     E_IPC_TASK_ID_LAST

/**
* A FIFO queue of IPC messages
*/
typedef struct
{
    IPC_sMsg_t      msgQueue[IPC_MSG_QUEUE_LENGTH]; /*!< Array of messages */
    uint8_t         queueTail;                      /*!< Last written message */
    uint8_t         queueHead;                      /*!< First to read message */
    uint8_t         queueSize;                      /*!< Elements inside the queue (diff(tail, head)) */
} IPC_sMsgQueue_t;

/**
* An IPC handler
*/
typedef struct
{
    IPC_eTaskID_t   recvId; /*!< ID of the receiver task */
    TaskHandle_t    handle; /*!< TaskHandle_t of the receiver task */
    IPC_sMsgQueue_t queue;  /*!< Message queue */
} IPC_sHandler_t;


/*******************************************************************************
 * Static Prototypes
 ******************************************************************************/
static int IPC_getHandlerIdx( IPC_eTaskID_t );

/*******************************************************************************
 * Static Variables
 ******************************************************************************/
static IPC_sHandler_t IPC_arHandler[IPC_HANDLER_CNT_MAX];   /*!< Array of IPC handler structures */
static uint8_t IPC_u8HandlerCnt;                            /*!< Count of initialized handlers */

/*******************************************************************************
 * Code
 ******************************************************************************/
/**
 * Create an IPC handler
 * Each task can have max. one handler and each handler must be initialized by
 * an IPC_createHandler() call
 * @param   aTaskID     Receiver task ID
 * @param   aHandle     Receiver task handle
 * @return  error
 */
IPC_eError_t IPC_createHandler( IPC_eTaskID_t aTaskID, TaskHandle_t aHandle )
{
    if (aHandle == NULL)
    {
        return E_IPC_ERR_CREATE_FAIL;
    }
    else if (IPC_u8HandlerCnt == IPC_HANDLER_CNT_MAX) // There is no space for more handlers
    {
        return E_IPC_ERR_CREATE_FAIL;
    }
    else if (IPC_getHandlerIdx( aTaskID ) != -1)  // A handler already exists for this task ID
    {
        return E_IPC_ERR_EXISTS;
    }
    else
    {
        /*
        * Initialize IPC handler
        */
        IPC_sHandler_t * psHandler = &IPC_arHandler[IPC_u8HandlerCnt];
        psHandler->recvId               = aTaskID;
        psHandler->handle               = aHandle;
        psHandler->queue.queueSize      = 0;
        psHandler->queue.queueTail      = 0;
        psHandler->queue.queueHead      = 0;

        IPC_u8HandlerCnt++;
        return E_IPC_SUCCESS;
    }
}

/**
 * Send an IPC message
 * @param   aRecv       Receiver task ID
 * @param   aType       Message type
 * @param   apData      Message data
 * @param   aDataSize   Size of message data in bytes
 * @return  error
 */
IPC_eError_t IPC_send( IPC_eTaskID_t aRecv, IPC_eMsgType_t aType, uint8_t * apData, int aDataSize )
{
    if (aDataSize >= IPC_MAX_DATA_LENGTH) // Data cannot be sent because it's too large
    {
        return E_IPC_ERR_SEND_FAIL;
    }

    uint8_t handlerIdx  = IPC_getHandlerIdx( aRecv );
    if (handlerIdx == -1) // There is no handler for this task ID
    {
        return E_IPC_ERR_NO_HANDLER;
    }

    /* Copy data to message buffer */
    IPC_sMsgQueue_t * queue = &(IPC_arHandler[handlerIdx].queue);
    uint8_t idx             = queue->queueTail;

    queue->msgQueue[idx].eIPC_MsgType    = aType;
    queue->msgQueue[idx].u32DataLen      = aDataSize;
    for (int i = 0; i < aDataSize; i++)
    {
        queue->msgQueue[idx].u8Data[i]   = apData[i];
    }

    queue->queueSize++;
  
    /*
    * Increment tail pointer - don't use modulo because it might be slower
    */
    if (queue->queueTail == IPC_MSG_QUEUE_LENGTH - 1)
    {
        queue->queueTail = 0;
    }
    else
    {
        queue->queueTail++;
    }

    if (pdPASS == xTaskNotifyGive( IPC_arHandler[handlerIdx].handle ))  // Notify task
    {
        return E_IPC_SUCCESS;
    }
    else
    {
        return E_IPC_ERR_SEND_FAIL;
    }
}

/**
 * Receive an IPC message
 * @param   aRecv    Receiver task ID
 * @param   aBuff    Message buffer
 * @return  error
 */
IPC_eError_t IPC_receive( IPC_eTaskID_t aRecv, IPC_sMsg_t * apBuf )
{
    int handlerIdx = IPC_getHandlerIdx( aRecv );
    if (handlerIdx == -1) // There is no handler for this task ID
    {
        return E_IPC_ERR_NO_HANDLER;
    }

    IPC_sMsgQueue_t * queue     = &(IPC_arHandler[handlerIdx].queue);
    if (queue->queueSize == 0)  // Nothing to be received
    {
        return E_IPC_ERR_RECV_FAIL;
    }

    /*
    * Copy the data from the queue to apBuf.
    * If you can assure that the receiving task will only use the data for a 
    * very short amount of time, this line can be replaced by
    *   *apBuf = queue->msgQueue[queue->queueHead];
    * This operates even faster, but involves the risk of unwanted behaviour.
    */
    memcpy( apBuf, &(queue->msgQueue[queue->queueHead]), sizeof(IPC_sMsg_t));
    queue->queueSize--;

    /*
    * Increment head pointer - don't use modulo because it might be slower
    */
    if (queue->queueHead == IPC_MSG_QUEUE_LENGTH - 1)
    {
        queue->queueHead = 0;
    }
    else
    {
        queue->queueHead++;
    }

    if (queue->queueSize > 0) // There is more data in the queue to be received
    {
        return E_IPC_RECV_MORE;
    }
    else  // All data has been received
    {
        return E_IPC_SUCCESS;
    }
}

/**
 * Get index of the IPC handler
 * @param   taskID    Receiver task ID
 * @return  index
 */
static int IPC_getHandlerIdx( IPC_eTaskID_t taskID )
{
    if (IPC_u8HandlerCnt > 0)
    {
        for (int i = 0; i < IPC_u8HandlerCnt; i++)
        {
            if (IPC_arHandler[i].recvId == taskID)
            {
                return i;
            }
        }
    }
    else
    {
        /* empty */
    }

    return -1;
}

/*******************************************************************************
 * Init function
 ******************************************************************************/
/**
 * Init IPC Handler module
 */
void IPC_initIPCHandler( void )
{
    IPC_u8HandlerCnt = 0;
}

//EOF
