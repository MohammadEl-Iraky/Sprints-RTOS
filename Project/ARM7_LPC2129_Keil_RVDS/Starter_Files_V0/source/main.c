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

/* Create a handle for each of our tasks */
TaskHandle_t xTask1Handle = NULL;
TaskHandle_t xTask2Handle = NULL;
TaskHandle_t xTask3Handle = NULL;

/* The period of each of the tasks in ms */
const TickType_t xTask1Frequency = 300 ;
const TickType_t xTask2Frequency = 500 ;
const TickType_t xTask3Frequency = 750 ;

/* a buffer to store the runtime stats */
char runTimeStatusBuffer[200];


/* A dummy delay function to simulate a task load in ms */
void dummyLoad_ms(uint32_t delay)
{
	volatile uint32_t i = 0;
	for(i=0; i<(3130*delay); i++ ){}
	
}

/*-------------------------------       Tasks        ----------------------------*/

/* Task 1, has a period of 300ms with a load of 50ms */
void vTask1_300( void * pvParameters )
{
		TickType_t xLastWakeTime = 0;

		for( ;; )
		{
			dummyLoad_ms(50);
			vTaskDelayUntil( &xLastWakeTime, xTask1Frequency );
		}
}


/* Task 2, has a period of 500ms with a load of 150ms */
void vTask2_500( void * pvParameters )
{
		TickType_t xLastWakeTime = 0;

		for( ;; )
		{
			dummyLoad_ms(150);
			vTaskDelayUntil( &xLastWakeTime, xTask2Frequency );
		}
}

/* Task 3, has a period of 750ms with a load of 100ms and we print the runtime status in this task */
void vTask3_750( void * pvParameters )
{
		TickType_t xLastWakeTime = 0;

		for( ;; )
		{
			/* making the delay 99 instead of 100 to compensate */
			dummyLoad_ms(99);
			/* Here we print the runtime status */
			vTaskGetRunTimeStats( runTimeStatusBuffer );
			xSerialPutChar('\n');
			vSerialPutString( runTimeStatusBuffer, 200 ); 
			vTaskDelayUntil( &xLastWakeTime, xTask3Frequency );
			

		}
}


int main( void )
{
	/* Setup the hardware for use with the Keil demo board. */
	prvSetupHardware();
	
	xTaskPeriodicCreate(	vTask3_750 ,       /* Function that implements the task. */
												"Task3",          /* Text name for the task. */
												1000,      /* Stack size in words, not bytes. */
												( void * ) 0,    /* Parameter passed into the task. */
												1,/* Priority at which the task is created. */
												&xTask3Handle,
												xTask3Frequency/* Used to pass out the created task's handle. */
										);									
								
	xTaskPeriodicCreate(	vTask2_500 ,       /* Function that implements the task. */
												"Task2",          /* Text name for the task. */
												1000,      /* Stack size in words, not bytes. */
												( void * ) 0,    /* Parameter passed into the task. */
												1,/* Priority at which the task is created. */
												&xTask2Handle,
												xTask2Frequency/* Used to pass out the created task's handle. */
										);

	xTaskPeriodicCreate(	vTask1_300 ,       /* Function that implements the task. */
												"Task1",          /* Text name for the task. */
												1000,      /* Stack size in words, not bytes. */
												( void * ) 0,    /* Parameter passed into the task. */
												1,/* Priority at which the task is created. */
												&xTask1Handle,
												xTask1Frequency/* Used to pass out the created task's handle. */
										);
											
	/* Set application tags for each of the task to be used for debugging with the trace APIs */										
	vTaskSetApplicationTaskTag( xTask1Handle,( void * )1 );
	vTaskSetApplicationTaskTag( xTask2Handle,( void * )2 );
	vTaskSetApplicationTaskTag( xTask3Handle,( void * )3 );


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


