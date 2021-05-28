/*
 * This main.c demonstrates the usage of the IPCHandler
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "fsl_debug_console.h"
#include "board.h"
#include "pin_mux.h"

#include "IPCHandler.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Task handles */
static TaskHandle_t handle_task1;
static TaskHandle_t handle_task2;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void MemManage_Handler( void );

void demo_task1( void *param );
void demo_task2( void *param );

/*******************************************************************************
 * Globals
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int main(void)
{
    /* Init board hardware. */
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_InitI2C1Pins();
    BOARD_InitSemcPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    IPC_initIPCHandler();

    /* Init task1 and IPC handler */
    if (pdPASS != xTaskCreate(demo_task1, "task 1", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY, &handle_task1))
    {
        PRINTF("Task creation failed!.\r\n");
        while (1)
            ;
    }
    if (IPC_createHandler( E_IPC_TASK_ID_1, handle_task1 ) != E_IPC_SUCCESS)
    {
        PRINTF("Failed to create ipc handler");
    }
#endif

    /* Init LPUART task and IPC handler */
#if UART == 1
    if (pdPASS != xTaskCreate(demo_task2, "task 2", configMINIMAL_STACK_SIZE + 100, NULL, tskIDLE_PRIORITY, &handle_task2))
    {
        PRINTF("Task creation failed!.\r\n");
        while (1)
            ;
    }
    if (IPC_createHandler( E_IPC_TASK_ID_2, handle_task2 ) != E_IPC_SUCCESS)
    {
        PRINTF("Failed to create ipc handler");
    }
#endif

    vTaskStartScheduler();

    for (;;)
    {
    } /* should never get here */
}

void demo_task1( void *param )
{
    /* Buffer for IPC messages */
    static struct IPC_Msg_t msgBuff = 
    {
            .eIPC_MsgType   = 0,
            .u32DataLen     = 0,
   };
  
  /* Task loop */
  while (1)
  {
    /* variant 1 using E_IPC_RECV_MORE */
//        if (0 != ulTaskNotifyTake( pdTRUE, 2 ))
//        {
//            int error = 0;
//            do
//            {
//                error = IPC_receive( E_IPC_TASK_ID_GUI, &msgBuff );
//                if (E_IPC_SUCCESS == error || E_IPC_RECV_MORE == error)
//                {
//                    PRINT_TO_TERMINAL((char *) msgBuff.u8Data);
//                }
//             } while (E_IPC_RECV_MORE == error);
//        }
    
      /* variant 2 using task notification value */
      uint32_t eventsToProcess = ulTaskNotifyTake( pdFALSE, 1 );
      if (eventsToProcess != 0)
      {
        while (eventsToProcess > 0)
        {
          int error = IPC_receive( E_IPC_TASK_ID_1, &msgBuff );
          if (E_IPC_SUCCESS == error || E_IPC_RECV_MORE == error)
          {
            PRINTF( msgBuff.u8Data );
          }
          eventsToProcess--;
        }
      }
  }
}

void demo_task2( void *param )
{
  /* Task loop */
  while (1)
  {
    vTaskDelay(5000);
    IPC_send( E_IPC_TASK_ID_1, E_IPC_MSG_TYPE_1, "Hello world!", strlen( "Hello world!") + 1);
  }
}


/*!
 * @brief Malloc failed hook.
 */
void vApplicationMallocFailedHook(void)
{
    for (;;)
        ;
}


/*!
 * @brief Stack overflow hook.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    (void)pcTaskName;
    (void)xTask;

    for (;;)
        ;
}

void MemManage_Handler(void)
{
    PRINTF("ERROR\r\n");
    while(1);
}
