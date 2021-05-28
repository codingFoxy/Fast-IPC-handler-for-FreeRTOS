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
#ifndef IPC_HANDLER_H_
#define IPC_HANDLER_H_

/*********************
 *      INCLUDES
 *********************/
#include "FreeRTOS.h"
#include "task.h"

/*********************
 *      DEFINES
 *********************/
#define IPC_MAX_DATA_LENGTH     512   /*!< The maximum data size that can be transmitted */
#define IPC_MSG_QUEUE_LENGTH    16    /*!< The size of the message queues */

/**********************
 *      TYPEDEFS
 **********************/
/**
* When your system needs to handle different types of data, it might come in handy
* to use this enum. This way, a receiving task can identify the type of data without
* having to parse the data itself.
*/
typedef enum
{
    E_IPC_MSG_TYPE_1,
    E_IPC_MSG_TYPE_2,
} IPC_eMsgType_t;

/**
* For easy and public task identification, this enum should be used. Make sure that
* each task uses only one task ID so that confusing data can be prevented
*/
typedef enum
{
    E_IPC_TASK_ID_1         = 1,
    E_IPC_TASK_ID_2         = 2,
    E_IPC_TASK_ID_LAST      = UINT8_MAX,
} IPC_eTaskID_t;

/**
* This structure defines an IPC message header
*/
typedef struct IPC_Msg_t
{
    IPC_eMsgType_t  eIPC_MsgType;                 /*!< The message/data type */
    uint8_t         u8Data[IPC_MAX_DATA_LENGTH];  /*!< A data buffer storing all data as byte arrays */
    uint32_t        u32DataLen;                   /*!< The size of data being transmitted in bytes */ 
} IPC_sMsg_t;

typedef enum
{
    E_IPC_SUCCESS           = 0,  /*!< Operation was successful */
    E_IPC_ERR_EXISTS        = 1,  /*!< Tried to redefine an existing handler */
    E_IPC_ERR_CREATE_FAIL   = 2,  /*!< Creation of an IPC handler failed */
    E_IPC_ERR_NO_HANDLER    = 3,  /*!< Data could not be sent/received because the handler does not exist */
    E_IPC_ERR_SEND_FAIL     = 4,  /*!< Data could not be sent */
    E_IPC_ERR_RECV_FAIL     = 5,  /*!< Data could not be received */
    E_IPC_RECV_MORE         = 6,  /*!< Data has been successfully received, but there is more waiting in the queue */
} IPC_eError_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * Create an IPC handler
 * Each task can have max. one handler and each handler must be initialized by
 * an IPC_createHandler() call
 * @param   aRecv       Receiver task ID
 * @param   aHandle     Receiver task handle
 * @return  error
 */
IPC_eError_t IPC_createHandler( IPC_eTaskID_t, TaskHandle_t );

/**
 * Send an IPC message
 * @param   aRecv       Receiver task ID
 * @param   aType       Message type
 * @param   apData      Message data
 * @param   aDataSize   Size of message data in bytes
 * @return  error
 */
IPC_eError_t IPC_send( IPC_eTaskID_t, IPC_eMsgType_t, uint8_t *, int );

/**
 * Receive an IPC message
 * @param   aRecv    Receiver task ID
 * @param   aBuff    Message buffer
 * @return  error
 */
IPC_eError_t IPC_receive( IPC_eTaskID_t, IPC_sMsg_t * );

/**
 * Init IPC Handler module
 */
void IPC_initIPCHandler( void );

#endif /* IPC_HANDLER_H_ */

//EOF
