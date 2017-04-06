/**
******************************************************************************
* @file    DataLog/Src/main.c
* @author  Central Labs
* @version V1.1.1
* @date    06-Dec-2016
* @brief   Main program body
******************************************************************************
* @attention
*
* <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of STMicroelectronics nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/

#include <string.h> /* strlen */
#include <stdio.h>  /* sprintf */
#include <math.h>   /* trunc */
#include "main.h"

#include "datalog_application.h"
#include "usbd_cdc_interface.h"

/* FatFs includes component */
#include "ff_gen_drv.h"
#include "sd_diskio.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Data acquisition period [ms] */
#define DATA_PERIOD_MS (2000)
//#define NOT_DEBUGGING

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t DATA_DIM = 3;


/* SendOverUSB = 0  --> Save sensors data on SDCard (enable with double click) */
/* SendOverUSB = 1  --> Send sensors data via USB */
uint8_t SendOverUSB = 1;

USBD_HandleTypeDef  USBD_Device;
static volatile uint8_t MEMSInterrupt = 0;
static volatile uint8_t acquire_data_enable_request  = 1;
static volatile uint8_t acquire_data_disable_request = 0;
static volatile uint8_t no_H_HTS221 = 0;
static volatile uint8_t no_T_HTS221 = 0;
static volatile uint8_t no_GG = 0;

static RTC_HandleTypeDef RtcHandle;
static void *LSM6DSM_X_0_handle = NULL;
static void *LSM6DSM_G_0_handle = NULL;
static void *LSM303AGR_X_0_handle = NULL;
static void *LSM303AGR_M_0_handle = NULL;
static void *LPS22HB_P_0_handle = NULL;
static void *LPS22HB_T_0_handle = NULL; 
static void *HTS221_H_0_handle = NULL; 
static void *HTS221_T_0_handle = NULL;
static void *GG_handle = NULL;

/* Private function prototypes -----------------------------------------------*/

static void Error_Handler( void );
static void RTC_Config( void );
static void RTC_TimeStampConfig( void );
static void initializeAllSensors( void );

/* Functions created by Prateek ----------------------------------------------*/
void kNNclassify (uint8_t *testdata, uint32_t testSize, uint8_t *meansdata, uint32_t meansSize, uint8_t *classes, uint8_t *predictedClassArray);
uint8_t calculateMinDistance(uint8_t **point, uint8_t *meansArray, uint32_t meansSize, uint8_t *classPtr);
uint32_t nDimDistance(uint8_t **testPoint, uint8_t **meansPoint);
void incrementDimensionPtr(uint8_t **dimPtr);

/* Private functions ---------------------------------------------------------*/
 
/**
  * @brief  Main program
  * @param  None
  * @retval None0
  */
int main( void )
{
  //initialise_monitor_handles();
  uint32_t msTick, msTickPrev = 0;
  uint8_t doubleTap = 0;
  
  /* STM32L4xx HAL library initialization:
  - Configure the Flash prefetch, instruction and Data caches
  - Configure the Systick to generate an interrupt each 1 msec
  - Set NVIC Group Priority to 4
  - Global MSP (MCU Support Package) initialization
  */
  HAL_Init();
  
  /* Configure the system clock */
  SystemClock_Config();
  
  if(SendOverUSB)
  {
    /* Initialize LED */
    BSP_LED_Init(LED1);
    //BSP_LED_On(LED1);
  }
#ifdef NOT_DEBUGGING     
  else
  {
    /* Initialize LEDSWD: Cannot be used during debug because it overrides SWDCLK pin configuration */
    BSP_LED_Init(LEDSWD);
    BSP_LED_Off(LEDSWD);
  }
#endif
  
  /* Initialize RTC */
  RTC_Config();
  RTC_TimeStampConfig();
  
  /* enable USB power on Pwrctrl CR2 register */
  HAL_PWREx_EnableVddUSB();
  
  if(SendOverUSB) /* Configure the USB */
  {
    /*** USB CDC Configuration ***/
    /* Init Device Library */
    USBD_Init(&USBD_Device, &VCP_Desc, 0);
    /* Add Supported Class */
    USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
    /* Add Interface callbacks for AUDIO and CDC Class */
    USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
    /* Start Device Process */
    USBD_Start(&USBD_Device);
  }
  else /* Configure the SDCard */
  {
    DATALOG_SD_Init();
  }
  HAL_Delay(200);
  
  /* Configure and disable all the Chip Select pins */
  Sensor_IO_SPI_CS_Init_All();
  
  /* Initialize and Enable the available sensors */
  initializeAllSensors();
  //Disable all the sensors
  disableAllSensors();
  //Just enable Accelerometer
  BSP_ACCELERO_Sensor_Enable( LSM6DSM_X_0_handle );

/*********************************************
 TODO: need to get these variables at the right places
 */
uint32_t meansSize = 0;
uint32_t testSize = 0;
uint32_t usbCommTestSize = 0;
uint32_t classSize = 0;
uint8_t *usbCommTestPtr;
uint8_t *meansptr;
uint8_t *testptr;
uint8_t *classptr;
uint8_t *predictedClassPtr;
//

  while (1)
  {
/*
    // Get sysTick value and check if it's time to execute the task
    msTick = HAL_GetTick();
    if(msTick % DATA_PERIOD_MS == 0 && msTickPrev != msTick)
    {
      msTickPrev = msTick;
      if(SendOverUSB)
      {
        BSP_LED_On(LED1);
      }
#ifdef NOT_DEBUGGING     
      else if (SD_Log_Enabled) 
      {
        BSP_LED_On(LEDSWD);
      }
#endif
    }
      RTC_Handler( &RtcHandle );
      
      Accelero_Sensor_Handler( LSM6DSM_X_0_handle );
      */

	  if (packetsReceived > 0)
	  {
		  char header = read_header_char();
		  switch (header)
		  {
		  	  case 'a' :										/* Fetching the Means array */
		  		  meansSize = get_sizeOfData();					/* Get the size of pay-load */
		  		  meansptr = (uint8_t *) malloc(meansSize);		/* Allocate memory for the means array to avoid garbles and overwrite */
		  		  read_data(meansptr);
		  		  packetsReceived--;
		  		  //CDC_Fill_Buffer(dimensionPtr[0], meansSize/3);
		  		  //CDC_Fill_Buffer(meansDimensionPtr[1], meansSize);
		  		  break;
		  	  case 'b' :
		  		  testSize = get_sizeOfData();
		  		  testptr = (uint8_t *) malloc(testSize);
		  		  read_data(testptr);
		  		  packetsReceived--;
		  		  break;
		  	  case 'c' :										/* Fetching the Classes array */
		  		  classSize = get_sizeOfData();					/* Get the size of pay-load */
		  		  classptr = (uint8_t *) malloc(classSize);		/* Allocate memory for the class array to avoid garbles and overwrite */
		  		  read_data(classptr);
		  		  packetsReceived--;
		  		  //CDC_Fill_Buffer(&classptr[1], 1);
		  		  //CDC_Fill_Buffer(meansDimensionPtr[1], meansSize);
		  		  break;
		  	  case 'k':
		  		  packetsReceived--;
		  		  predictedClassPtr = (uint8_t *) malloc(testSize/DATA_DIM);
		  		  kNNclassify (testptr, testSize, meansptr, meansSize, classptr, predictedClassPtr);
		  		  CDC_Fill_Buffer(predictedClassPtr, testSize/DATA_DIM);
		  		  break;
		  	  case 'x' :
		  		  usbCommTestSize = get_sizeOfData();
		  		  usbCommTestPtr = (uint8_t *) malloc(usbCommTestSize);
		  		  read_data(usbCommTestPtr);
		  		  packetsReceived--;
		  		  CDC_Fill_Buffer(usbCommTestPtr, usbCommTestSize);
		  		  break;
		  }
	  }

    // Go to Sleep
    __WFI();

  }
}

/**
 * @ Brief: Classifies a test point based on k nearest neighbors
 * @Param1:	POinter to the test data array
 * @Param2: size of test data
 * @Param3: Pointer to means data array
 * @Param4: size of the means array
 * @Param5: Pointer to array of original classes (class[i] carries the class of i'th means
 * @Param6: Pointer to the predicted class array
 * @Return: Modifies the predicted class array
 */
void kNNclassify (uint8_t *testdata, uint32_t testSize, uint8_t *meansdata, uint32_t meansSize, uint8_t *classes, uint8_t *predictedClassArray)
{
	/* Create local pointer array which points to each the start of points for each dimension
	 * Ex. [x1 x2 x3 y4 y1 y2 y3 y4 z1 z2 z3 z4 ...... k1 k2 k3 k4]
	 * *localPointerArray[0] ----(points to)---> x1
	 * *localPointerArray[1] ----(points to)---> y1
	 * *localPointerArray[2] ----(points to)---> z1
	 */
	uint8_t *testDimPtr[DATA_DIM];
	for (uint8_t i = 0; i < DATA_DIM; i++)
	{
		testDimPtr[i] = testdata + (i*testSize)/DATA_DIM;
	}

	for (uint32_t i = 0; i < testSize/DATA_DIM; i++)
	{
		//CDC_Fill_Buffer(*testDimPtr, 1);
		predictedClassArray[i] = calculateMinDistance(testDimPtr, meansdata, meansSize, classes);
		incrementDimensionPtr(testDimPtr);
	}
}

/**
 * @ Brief:	 This function calculates the minimum distance between a point and a set of points (means array)
 * 			 The point/means passed to this function could be N dimensional
 * @ Param1: Requires a pointer to a pointer array each element of which points to the i'th dimension
 * 			 of the test point (eg. point = [x1, y1, z1, k1, u1]
 * 			 pointer[2] -----> [y1], pointer[3] -----> [z1]
 * @ Param2: pointer to means array
 * @ Param3: Means size (this is required here because meansSize is not a global var)
 * @ Param3: Pointer to original class array
 * @ Return: returns the predicted class for that point
 */
uint8_t calculateMinDistance(uint8_t **point, uint8_t *meansArray, uint32_t meansSize, uint8_t *classPtr)
{
	/* Create local pointer array which points to each the start of points for each dimension
	 * Ex. [x1 x2 x3 y4 y1 y2 y3 y4 z1 z2 z3 z4 ...... k1 k2 k3 k4]
	 * *localPointerArray[0] ----(points to)---> x1
	 * *localPointerArray[1] ----(points to)---> y1
	 * *localPointerArray[2] ----(points to)---> z1
	 */
	uint8_t *meansDimPtr[DATA_DIM];
	for (uint8_t i = 0; i < DATA_DIM; i++)
	{
		meansDimPtr[i] = meansArray + (i*meansSize)/DATA_DIM;
	}

	uint32_t distance = 0;
	uint32_t minDistance = UINT32_MAX;
	uint8_t predictedClass;
	for (uint16_t i = 0; i < meansSize/DATA_DIM; i++)
	{
		distance = nDimDistance(point, meansDimPtr);			/* Find distance between test point and 1 means point */
		if (distance < minDistance)
		{
			minDistance = distance;
			predictedClass = classPtr[i];
		}
		incrementDimensionPtr(meansDimPtr);						/* Change the mean point to next one */
	}
	return predictedClass;
}

/**
 * @ Brief: This function increments every element in an array of pointers
 * 			by one. This is required to increment every element in array of
 * 			pointers (which are pointing to different dimensions) to be incremented
 * 			Number of dimensions are given by the global var DATA_DIM
 * @ Param: This function accepts an array of pointers
 * @ Ret:	No return since it modifies the original pointer array
 */
void incrementDimensionPtr(uint8_t **dimPtr)
{
	for (uint8_t j = 0; j < DATA_DIM; j++)
	{
		dimPtr[j]++;
	}
}


/**
 * @ Brief: 	This function calculates the distance of one point from another point
 * 				in an N-Dimensional space
 * @ Param: 	The function takes in two arrays of pointers, each element of this array
 * 				is pointing to the corresponding dimension of the point
 * 				Eg. i'th element of the array a[i] points to the i'th dimension point
 * @ReturnVal: 	This function returns the distance between two N-Dimensional points in
 * 				in a uint32_t format
 * @Caveat:		Loss of accuracy since everything in computed in int
 */
uint32_t nDimDistance(uint8_t **testPoint, uint8_t **meansPoint)
{
	uint32_t sumOfSquares = 0;							/* Since this will always be positive it can be unsigned */
	uint32_t distance = 0;
	for (uint32_t i = 0; i < DATA_DIM; i++)				/* Loop through each dimension */
	{
		sumOfSquares = sumOfSquares + pow((int)(*testPoint[i] - *meansPoint[i]), 2);	/* Xi - Yi can go negative hence casted to int */
	}
	/* square root casted to uint32 which will simply ignore the decimal
	 * part (i.e 5.99 and 5.01 will be same in this case
	 */
	distance = (uint32_t) sqrt(sumOfSquares);
	return distance;
}


/**
* @brief  Initialize all sensors
* @param  None
* @retval None
*/
static void initializeAllSensors( void )
{
  if (BSP_ACCELERO_Init( LSM6DSM_X_0, &LSM6DSM_X_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
  
  if (BSP_GYRO_Init( LSM6DSM_G_0, &LSM6DSM_G_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
  
  if (BSP_ACCELERO_Init( LSM303AGR_X_0, &LSM303AGR_X_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
  
  if (BSP_MAGNETO_Init( LSM303AGR_M_0, &LSM303AGR_M_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
  
  if (BSP_PRESSURE_Init( LPS22HB_P_0, &LPS22HB_P_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
  
  if (BSP_TEMPERATURE_Init( LPS22HB_T_0, &LPS22HB_T_0_handle ) != COMPONENT_OK)
  {
    while(1);
  }
  
  if(BSP_TEMPERATURE_Init( HTS221_T_0, &HTS221_T_0_handle ) == COMPONENT_ERROR)
  {
    no_T_HTS221 = 1;
  }
  
  if(BSP_HUMIDITY_Init( HTS221_H_0, &HTS221_H_0_handle ) == COMPONENT_ERROR)
  {
    no_H_HTS221 = 1;
  }
  
  /* Inialize the Gas Gauge if the battery is present */
  if(BSP_GG_Init(&GG_handle) == COMPONENT_ERROR)
  {
    no_GG=1;
  }
  
  if(!SendOverUSB)
  {
    /* Enable HW Double Tap detection */
    BSP_ACCELERO_Enable_Double_Tap_Detection_Ext(LSM6DSM_X_0_handle);
    BSP_ACCELERO_Set_Tap_Threshold_Ext(LSM6DSM_X_0_handle, LSM6DSM_TAP_THRESHOLD_MID);
  }
  
  
}

/**
* @brief  Enable all sensors
* @param  None
* @retval None
*/
void enableAllSensors( void )
{
  BSP_ACCELERO_Sensor_Enable( LSM6DSM_X_0_handle );
  BSP_GYRO_Sensor_Enable( LSM6DSM_G_0_handle );
  BSP_ACCELERO_Sensor_Enable( LSM303AGR_X_0_handle );
  BSP_MAGNETO_Sensor_Enable( LSM303AGR_M_0_handle );
  BSP_PRESSURE_Sensor_Enable( LPS22HB_P_0_handle );
  BSP_TEMPERATURE_Sensor_Enable( LPS22HB_T_0_handle );
  if(!no_T_HTS221)
  {
    BSP_TEMPERATURE_Sensor_Enable( HTS221_T_0_handle );
    BSP_HUMIDITY_Sensor_Enable( HTS221_H_0_handle );
  }
  
}



/**
* @brief  Disable all sensors
* @param  None
* @retval None
*/
void disableAllSensors( void )
{
  BSP_ACCELERO_Sensor_Disable( LSM6DSM_X_0_handle );
  BSP_ACCELERO_Sensor_Disable( LSM303AGR_X_0_handle );
  BSP_GYRO_Sensor_Disable( LSM6DSM_G_0_handle );
  BSP_MAGNETO_Sensor_Disable( LSM303AGR_M_0_handle );
  BSP_HUMIDITY_Sensor_Disable( HTS221_H_0_handle );
  BSP_TEMPERATURE_Sensor_Disable( HTS221_T_0_handle );
  BSP_TEMPERATURE_Sensor_Disable( LPS22HB_T_0_handle );
  BSP_PRESSURE_Sensor_Disable( LPS22HB_P_0_handle );
}



/**
* @brief  Configures the RTC
* @param  None
* @retval None
*/
static void RTC_Config( void )
{
  /*##-1- Configure the RTC peripheral #######################################*/
  RtcHandle.Instance = RTC;
  
  /* Configure RTC prescaler and RTC data registers */
  /* RTC configured as follow:
  - Hour Format    = Format 12
  - Asynch Prediv  = Value according to source clock
  - Synch Prediv   = Value according to source clock
  - OutPut         = Output Disable
  - OutPutPolarity = High Polarity
  - OutPutType     = Open Drain */
  RtcHandle.Init.HourFormat     = RTC_HOURFORMAT_12;
  RtcHandle.Init.AsynchPrediv   = RTC_ASYNCH_PREDIV;
  RtcHandle.Init.SynchPrediv    = RTC_SYNCH_PREDIV;
  RtcHandle.Init.OutPut         = RTC_OUTPUT_DISABLE;
  RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  RtcHandle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
  
  if ( HAL_RTC_Init( &RtcHandle ) != HAL_OK )
  {
    
    /* Initialization Error */
    Error_Handler();
  }
}

/**
* @brief  Configures the current time and date
* @param  None
* @retval None
*/
static void RTC_TimeStampConfig( void )
{
  
  RTC_DateTypeDef sdatestructure;
  RTC_TimeTypeDef stimestructure;
  
  /*##-3- Configure the Date using BCD format ################################*/
  /* Set Date: Monday January 1st 2000 */
  sdatestructure.Year    = 0x00;
  sdatestructure.Month   = RTC_MONTH_JANUARY;
  sdatestructure.Date    = 0x01;
  sdatestructure.WeekDay = RTC_WEEKDAY_MONDAY;
  
  if ( HAL_RTC_SetDate( &RtcHandle, &sdatestructure, FORMAT_BCD ) != HAL_OK )
  {
    
    /* Initialization Error */
    Error_Handler();
  }
  
  /*##-4- Configure the Time using BCD format#################################*/
  /* Set Time: 00:00:00 */
  stimestructure.Hours          = 0x00;
  stimestructure.Minutes        = 0x00;
  stimestructure.Seconds        = 0x00;
  stimestructure.TimeFormat     = RTC_HOURFORMAT12_AM;
  stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE ;
  stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
  
  if ( HAL_RTC_SetTime( &RtcHandle, &stimestructure, FORMAT_BCD ) != HAL_OK )
  {   
    /* Initialization Error */
    Error_Handler();
  }
}

/**
* @brief  Configures the current time and date
* @param  hh the hour value to be set
* @param  mm the minute value to be set
* @param  ss the second value to be set
* @retval None
*/
void RTC_TimeRegulate( uint8_t hh, uint8_t mm, uint8_t ss )
{
  
  RTC_TimeTypeDef stimestructure;
  
  stimestructure.TimeFormat     = RTC_HOURFORMAT12_AM;
  stimestructure.Hours          = hh;
  stimestructure.Minutes        = mm;
  stimestructure.Seconds        = ss;
  stimestructure.SubSeconds     = 0;
  stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;
  
  if ( HAL_RTC_SetTime( &RtcHandle, &stimestructure, FORMAT_BIN ) != HAL_OK )
  {
    /* Initialization Error */
    Error_Handler();
  }
}



/**
* @brief  EXTI line detection callbacks
* @param  GPIO_Pin: Specifies the pins connected EXTI line
* @retval None
*/
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
  MEMSInterrupt=1;
}



/**
* @brief  This function is executed in case of error occurrence
* @param  None
* @retval None
*/
static void Error_Handler( void )
{
  
  while (1)
  {}
}



#ifdef  USE_FULL_ASSERT

/**
* @brief  Reports the name of the source file and the source line number
*   where the assert_param error has occurred
* @param  file: pointer to the source file name
* @param  line: assert_param error line source number
* @retval None
*/
void assert_failed( uint8_t *file, uint32_t line )
{
  
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  while (1)
  {}
}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
