/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "lpc21xx.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


#define RELEASED	0
#define PUSHED		1


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
static void prvSetupHardware( void );
/*-----------------------------------------------------------*/

TaskHandle_t xButton1_TaskHandle = NULL;
TaskHandle_t xButton2_TaskHandle = NULL;
TaskHandle_t xSerialTaskHandle = NULL;
TaskHandle_t xConsumerTaskHandle = NULL;

QueueHandle_t xQueue;

typedef struct
{
	int8_t data[25];
	uint8_t length;
}xMessage;


/* Function to give time for the UART before printing the next string */
void serialDelay( void )
{
	volatile uint32_t i = 0;
	for( i=0; i<10000; i++)
	{
	}
}

void vButton1_Task( void * pvParameters )
{
	
	uint8_t buttonCurrentState = PUSHED;
	uint8_t buttonPrevState = PUSHED;
	
	xMessage rising = { "Button1 rising edge\n",  20 } ;
	xMessage falling = { "Button1 falling edge\n",  21 } ;

	for( ;; )
    {
			buttonCurrentState = GPIO_read( PORT_0, PIN0 );
	
			/* falling edge */
			if( (PIN_IS_HIGH == buttonPrevState) && (PIN_IS_LOW == buttonCurrentState ) )
			{
				xQueueSend( xQueue, ( void * )&falling, portMAX_DELAY );
			}	
			else if( (PIN_IS_LOW == buttonPrevState) && (PIN_IS_HIGH == buttonCurrentState ) )
			{
				xQueueSend( xQueue, ( void * )&rising, portMAX_DELAY );

			}
			buttonPrevState = buttonCurrentState;
			vTaskDelay(100);
	}
}


void vButton2_Task( void * pvParameters )
{
	
	uint8_t buttonCurrentState = PUSHED;
	uint8_t buttonPrevState = PUSHED;
	
	xMessage rising = { "Button2 rising edge\n",  20 } ;
	xMessage falling = { "Button2 falling edge\n",  21 } ;
	
	for( ;; )
    {
			buttonCurrentState = GPIO_read( PORT_0, PIN1 );
	
			/* falling edge */
			if( (PIN_IS_HIGH == buttonPrevState) && (PIN_IS_LOW == buttonCurrentState ) )
			{
				xQueueSend( xQueue, ( void * )&falling, portMAX_DELAY );
			}	
			else if( (PIN_IS_LOW == buttonPrevState) && (PIN_IS_HIGH == buttonCurrentState ) )
			{
				xQueueSend( xQueue, ( void * )&rising, portMAX_DELAY );

			}
			buttonPrevState = buttonCurrentState;
			vTaskDelay(100);
	}
}

/* Task to send a random string every 100ms */
void v100msTask( void * pvParameters )
{
	xMessage randString = { "Sprints100\n",  11 } ;
		
	for( ;; )
	{
			xQueueSend( xQueue, ( void * ) &randString, portMAX_DELAY );
			vTaskDelay(100);
	}
}


void vConsumerTask( void * pvParameters )
{
	xMessage myData;

	for( ;; )
	{
			xQueueReceive( xQueue, &myData, portMAX_DELAY );
			vSerialPutString( myData.data, myData.length );
	}
}



/*
 * Application entry point:
 * Starts all the other tasks, then starts the scheduler. 
 */
int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	/* Create Tasks here */
	xTaskCreate(
								vButton1_Task ,       /* Function that implements the task. */
								"Button1 Task",          /* Text name for the task. */
								100,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								1,/* Priority at which the task is created. */
								&xButton1_TaskHandle );      /* Used to pass out the created task's handle. */

	
	xTaskCreate(
								vButton2_Task ,       /* Function that implements the task. */
								"Button2 Task",          /* Text name for the task. */
								100,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								1,/* Priority at which the task is created. */
								&xButton2_TaskHandle );      /* Used to pass out the created task's handle. */
								
	xTaskCreate(
								v100msTask ,       /* Function that implements the task. */
								"100ms Task",          /* Text name for the task. */
								100,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								1,/* Priority at which the task is created. */
								&xSerialTaskHandle );      /* Used to pass out the created task's handle. */
								
	xTaskCreate(
								vConsumerTask ,       /* Function that implements the task. */
								"Consumer Task",          /* Text name for the task. */
								100,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								1,/* Priority at which the task is created. */
								&xConsumerTaskHandle );      /* Used to pass out the created task's handle. */
								


								
	xQueue = xQueueCreate( 10, sizeof( xMessage ) );	/* each string max num of chars is 20 */
	/* Now all the tasks have been started - start the scheduler.

	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used here. */
	vTaskStartScheduler();

	/* Should never reach here!  If you do then there was not enough heap
	available for the idle task to be created. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


