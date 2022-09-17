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

TaskHandle_t xButtonTaskHandle = NULL;
TaskHandle_t xLedTask1_Handle = NULL;
TaskHandle_t xLedTask2_Handle = NULL;

/* Task to toggle the LED every 400ms */
void vLedTask1( void * pvParameters )
{
    for( ;; )
    {
			GPIO_write( PORT_0, PIN1, PIN_IS_HIGH);
			vTaskDelay(400);
			GPIO_write( PORT_0, PIN1, PIN_IS_LOW);
			vTaskDelay(400);
    }
}

/* Task to toggle the LED every 100ms */
void vLedTask2( void * pvParameters )
{
    for( ;; )
    {
			GPIO_write( PORT_0, PIN1, PIN_IS_HIGH);
			vTaskDelay(100);
			GPIO_write( PORT_0, PIN1, PIN_IS_LOW);
			vTaskDelay(100);
    }
}

/* In this task we read the button state every 100ms, and according to how long it is pressed, we either turn off the LED, or toggle it every 100 or 400ms */
void vButtonTask( void * pvParameters )
{
		uint8_t buttonCurrentState = RELEASED;	
		uint8_t buttonPrevState = RELEASED;			/* Stores the value of the previous button read operation */
		uint16_t buttonPressCount = 0;
    
		for( ;; )
    {
				buttonCurrentState = GPIO_read( PORT_0, PIN0 );
				/* Here after reading the current button state, we compare it with previous state.
					- If the button was high in the previous iteration and it is still high, add 100ms to the counter			
			*/
				if( (PIN_IS_HIGH == buttonPrevState) && (PIN_IS_HIGH == buttonCurrentState) )
				{
					buttonPressCount++;			
				}
				else if( (PIN_IS_HIGH == buttonPrevState) && (PIN_IS_LOW == buttonCurrentState ) )
				{
					/* If it was high and then it became low which means the user released the button, then according to how long the button has been pressed, we select one 
					of our 3 states */
					/* If the button was pressed less than 2 secs, turn OFF the LED and suspend the 2 other tasks */
						if( buttonPressCount < 20 ){				
							GPIO_write(PORT_0, PIN1, PIN_IS_LOW );
							vTaskSuspend( xLedTask1_Handle );
							vTaskSuspend( xLedTask2_Handle );
						}
						/* If the button was pressed between 2 and 4s, we toggle the led with 400ms periodicity, so we suspend task 2 and resume task 1 */
						else if( buttonPressCount < 40)
						{
							vTaskSuspend( xLedTask2_Handle );
							vTaskResume( xLedTask1_Handle );
						}
						/* If the button was pressed between 2 and 4s, we toggle the led with 400ms periodicity, so we suspend task 2 and resume task 1 */
						else
						{
							vTaskSuspend( xLedTask1_Handle );
							vTaskResume( xLedTask2_Handle );
						}
						/* Reset the counter as the button was released */
						buttonPressCount = 0;
				}
				/* Store the current state to use it in the next iteration */
				buttonPrevState = buttonCurrentState;
				vTaskDelay(100);
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
	/* Task to toggle the LED every 400ms */
	xTaskCreate(
								vLedTask1 ,       /* Function that implements the task. */
								"LED Task1",          /* Text name for the task. */
								50,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								1,/* Priority at which the task is created. */
								&xLedTask1_Handle );      /* Used to pass out the created task's handle. */
								
	/* Task to toggle the LED every 100ms */
	xTaskCreate(
								vLedTask2 ,       /* Function that implements the task. */
								"LED Task2",          /* Text name for the task. */
								50,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								1,/* Priority at which the task is created. */
								&xLedTask2_Handle );      /* Used to pass out the created task's handle. */
								
	/* Task to read the button state every 100ms and toggle the LED accordingly */
	xTaskCreate(
								vButtonTask ,       /* Function that implements the task. */
								"Button Task3",          /* Text name for the task. */
								50,      /* Stack size in words, not bytes. */
								( void * ) 0,    /* Parameter passed into the task. */
								2,/* Priority at which the task is created. */
								&xButtonTaskHandle );      /* Used to pass out the created task's handle. */
								
	vTaskSuspend( xLedTask1_Handle );
	vTaskSuspend( xLedTask2_Handle );
							
								
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


