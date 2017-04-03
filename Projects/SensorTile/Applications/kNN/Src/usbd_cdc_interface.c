/**
  ******************************************************************************
  * @file    DataLog/Src/usbd_cdc_interface.c
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    29-January-2016
  * @brief   Source file for USBD CDC interface
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define APP_RX_DATA_SIZE  2048
#define APP_TX_DATA_SIZE  2048

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
USBD_CDC_LineCodingTypeDef LineCoding =
  {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
  };

uint8_t UserRxBuffer[APP_RX_DATA_SIZE];/* Received Data over USB are stored in this buffer */
uint8_t UserTxBuffer[APP_TX_DATA_SIZE];/* Received Data over UART (CDC interface) are stored in this buffer */
uint32_t BuffLength;

uint32_t UserTxBufPtrIn = 0;	/* Increment this pointer or roll it back to
                               start address when data are received over USART */
uint32_t UserTxBufPtrOut = 0; 	/* Increment this pointer or roll it back to
                                 start address when data are sent over USB */

//This is incremented by receive_complete function
uint8_t packetsReceived = 0;

volatile uint8_t USB_RxBuffer[USB_RxBufferDim];
volatile uint16_t USB_RxBufferStart_idx = 0;
volatile uint16_t packet_start = 0;		//Tracks the start of every packet
volatile uint16_t sizeOfData;
volatile uint16_t headerSize = 3;		//This includes the character header + payload size indicator (2 bytes)

/* TIM handler declaration */
TIM_HandleTypeDef  TimHandle;
/* USB handler declaration */
extern USBD_HandleTypeDef  USBD_Device;

/* Private function prototypes -----------------------------------------------*/
static int8_t CDC_Itf_Init     (void);
static int8_t CDC_Itf_DeInit   (void);
static int8_t CDC_Itf_Control  (uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Itf_Receive  (uint8_t* pbuf, uint32_t *Len);

static void Error_Handler(void);
static void TIM_Config(void);

USBD_CDC_ItfTypeDef USBD_CDC_fops =
{
  CDC_Itf_Init,
  CDC_Itf_DeInit,
  CDC_Itf_Control,
  CDC_Itf_Receive
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  CDC_Itf_Init
  *         Initializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_Init(void)
{
  /*##-2- Enable TIM peripherals Clock #######################################*/
  TIMx_CLK_ENABLE();
  
   /*##-3- Configure the NVIC for TIMx ########################################*/
  /* Set Interrupt Group Priority */
  HAL_NVIC_SetPriority(TIMx_IRQn, 0x6, 0);
  
  /* Enable the TIMx global Interrupt */
  HAL_NVIC_EnableIRQ(TIMx_IRQn);
  
  /*##-3- Configure the TIM Base generation  #################################*/
  TIM_Config();
  
  /*##-4- Start the TIM Base generation in interrupt mode ####################*/
  /* Start Channel1 */
  if(HAL_TIM_Base_Start_IT(&TimHandle) != HAL_OK)
  {
    /* Starting Error */
    Error_Handler();
  }
  
  /*##-5- Set Application Buffers ############################################*/
  USBD_CDC_SetTxBuffer(&USBD_Device, UserTxBuffer, 0);
  USBD_CDC_SetRxBuffer(&USBD_Device, UserRxBuffer);
  
  return (USBD_OK);
}

/**
  * @brief  CDC_Itf_DeInit
  *         DeInitializes the CDC media low layer
  * @param  None
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_DeInit(void)
{
  return (USBD_OK);
}


/**
  * @brief  CDC_Itf_Control
  *         Manage the CDC class requests
  * @param  Cmd: Command code            
  * @param  Buf: Buffer containing command data (request parameters)
  * @param  Len: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_Control (uint8_t cmd, uint8_t* pbuf, uint16_t length)
{ 
  switch (cmd)
  {
  case CDC_SEND_ENCAPSULATED_COMMAND:
    /* Add your code here */
    break;

  case CDC_GET_ENCAPSULATED_RESPONSE:
    /* Add your code here */
    break;

  case CDC_SET_COMM_FEATURE:
    /* Add your code here */
    break;

  case CDC_GET_COMM_FEATURE:
    /* Add your code here */
    break;

  case CDC_CLEAR_COMM_FEATURE:
    /* Add your code here */
    break;

  case CDC_SET_LINE_CODING:
    LineCoding.bitrate    = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |\
                            (pbuf[2] << 16) | (pbuf[3] << 24));
    LineCoding.format     = pbuf[4];
    LineCoding.paritytype = pbuf[5];
    LineCoding.datatype   = pbuf[6];
    
    /* Set the new configuration */
//    ComPort_Config();
    break;

  case CDC_GET_LINE_CODING:
    pbuf[0] = (uint8_t)(LineCoding.bitrate);
    pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
    pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
    pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
    pbuf[4] = LineCoding.format;
    pbuf[5] = LineCoding.paritytype;
    pbuf[6] = LineCoding.datatype;     
    break;

  case CDC_SET_CONTROL_LINE_STATE:
    /* Add your code here */
    break;

  case CDC_SEND_BREAK:
     /* Add your code here */
    break;    
    
  default:
    break;
  }
  
  return (USBD_OK);
}

/**
  * @brief  Fill the usb tx buffer
  * @param  Buf: pointer to the tx buffer
  * @param  TotalLen: number of bytes to be sent
  * @retval Result of the operation: USBD_OK if all operations are OK
  */
uint8_t CDC_Fill_Buffer(uint8_t* Buf, uint32_t TotalLen)
{
  uint16_t i;
  for (i = 0; i < TotalLen; i++)
  {
	  UserTxBuffer[UserTxBufPtrIn] = Buf[i];
	  UserTxBufPtrIn = (UserTxBufPtrIn + 1) % APP_RX_DATA_SIZE;
  }
  return (USBD_OK);
}

/**
  * @brief  TIM period elapsed callback
  * @param  htim: TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint32_t buffptr;
  uint32_t buffsize;

  if(UserTxBufPtrOut != UserTxBufPtrIn)
  {
    if(UserTxBufPtrOut > UserTxBufPtrIn) /* Rollback */
    {
      buffsize = APP_RX_DATA_SIZE - UserTxBufPtrOut;
    }
    else 
    {
      buffsize = UserTxBufPtrIn - UserTxBufPtrOut;
    }
    
    buffptr = UserTxBufPtrOut;
    
    USBD_CDC_SetTxBuffer(&USBD_Device, (uint8_t*)&UserTxBuffer[buffptr], buffsize);
    
    if(USBD_CDC_TransmitPacket(&USBD_Device) == USBD_OK)
    {
      UserTxBufPtrOut += buffsize;
      if (UserTxBufPtrOut == APP_RX_DATA_SIZE)
      {
        UserTxBufPtrOut = 0;
      }
    }
  }
}


/**
  * @brief  This function reads the size of data from the header and asserts
  * 		dataReceiveComplete high when the whole data packet is received.
  * @param  None
  * @retval None
  */
void receive_complete(void)
{
	sizeOfData = USB_RxBuffer[packet_start] + (USB_RxBuffer[packet_start+1] << 8);
	uint8_t prev_numpac=packetsReceived;
	if (USB_RxBufferStart_idx == (packet_start + sizeOfData + headerSize) % APP_RX_DATA_SIZE)
	{
		packet_start = (packet_start + sizeOfData + headerSize) % APP_RX_DATA_SIZE;
		packetsReceived++;
	} else {
	    packetsReceived = prev_numpac;
	}
}


/**
  * @brief  CDC_Itf_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  * @param  Buf: Buffer of data to be transmitted
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Itf_Receive(uint8_t* Buf, uint32_t *Len)
{
  uint16_t numByteToCopy;
  if(((USB_RxBufferStart_idx) + (uint16_t)*Len) > USB_RxBufferDim)
  {
    numByteToCopy = USB_RxBufferDim - (USB_RxBufferStart_idx);
    uint16_t Buf_idx = 0;
    memcpy((uint8_t*)&USB_RxBuffer[USB_RxBufferStart_idx], (uint8_t*)&Buf[Buf_idx], numByteToCopy);
    USB_RxBufferStart_idx = 0;
    Buf_idx = numByteToCopy;
    numByteToCopy = (uint16_t)(*Len - numByteToCopy);
    memcpy((uint8_t*)&USB_RxBuffer[USB_RxBufferStart_idx], (uint8_t*)&Buf[Buf_idx], numByteToCopy);
    USB_RxBufferStart_idx = numByteToCopy;
  }
  else
  {
	numByteToCopy = (uint16_t) * Len;
    memcpy((uint8_t*)&USB_RxBuffer[USB_RxBufferStart_idx], (uint8_t*)&Buf[0], numByteToCopy);
    //USB_RxBufferStart_idx always points to the immediate next element after the last written byte
    USB_RxBufferStart_idx = (USB_RxBufferStart_idx + numByteToCopy) % APP_RX_DATA_SIZE;
   }

  //Check for complete packet receive
  receive_complete();

  // Initiate next USB packet transfer
  USBD_CDC_ReceivePacket(&USBD_Device);
  return (USBD_OK);
}

char read_header_char(void)
{
	char header = '\0';
	//Added buffer size to packet_start-size of data to avoid taking a modulo of a negative number
	header = USB_RxBuffer[(APP_RX_DATA_SIZE + packet_start - sizeOfData - 1) % APP_RX_DATA_SIZE];
	return header;
}

uint16_t get_sizeOfData(void)
{
	return sizeOfData;
}

void read_data(uint8_t *data_array)
{
//	CDC_Fill_Buffer(&USB_RxBuffer[0], 259);
	int16_t ind=(packet_start - sizeOfData);
	if (ind > 0)
	{
		memcpy((uint8_t *)&data_array[0], (uint8_t *)&USB_RxBuffer[ind], sizeOfData);
	} else {
		memcpy((uint8_t *)&data_array[0], (uint8_t *)&USB_RxBuffer[APP_RX_DATA_SIZE+ind], -ind);
		memcpy((uint8_t *)&data_array[-ind], (uint8_t *)&USB_RxBuffer[0], ((int16_t) sizeOfData + ind));
	}
}



/*
 * the Read_Rx_Buffer() routine returns any characters that have been placed into
 * the RxBuffer by the USB receive FIFO process which emulates the UART
 * DMA circular buffer
 *
 * The UART receiver is set up with DMA in circular buffer mode.  This means that
 * it will continue receiving characters forever regardless of whether any are
 * taken out.  It will just write over the previous buffer if it overflows
 *
 * To know whether there are any characters in the buffer we look at the counter
 * register in the DMA channel assigned to the UART Rx buffer.  This count starts
 * at the size of the transfer and is decremented after a transfer to memory is done.
 *
 * We maintain a mirror DMA count register value readIndex.  If they are the same no data is
 * available.  If they are different, the DMA has decremented its counter
 * so we transfer data until they are the same or the rx buffer is full.  We
 * wrap our down counter the same way the DMA does in circular mode.
 */
/*
uint8_t Read_Rx_Buffer(char *instring, uint32_t count)
{
    uint32_t bytesread = 0;
//  extern USBD_CDC_ItfTypeDef  USBD_Interface_fops_FS;
    if(count > bytesread)
    {
        while(readIndex == (APP_RX_DATA_SIZE - USB_RxBufferStart_idx)) {}
        {
            while((count > bytesread) & (readIndex !=(APP_RX_DATA_SIZE - USB_RxBufferStart_idx )))
            {
                instring[bytesread] = USB_RxBuffer[APP_RX_DATA_SIZE - readIndex];
                if(readIndex == (0))
                    readIndex = APP_RX_DATA_SIZE;
                else readIndex--;
                bytesread++;
            }
        }
    }
    HAL_Delay(500);
    BSP_LED_Off(LED1);
    return (int)bytesread;
}
*/
/*
uint8_t Read_Header()
{
	uint8_t header = USB_RxBuffer[4];
	return header;
}

uint32_t Read_SizeOfData(void)
{
	int dataSize = USB_RxBuffer[0] + (USB_RxBuffer[1] << 8) + (USB_RxBuffer[2] << 16) + (USB_RxBuffer[3] << 24);
	return dataSize;
}
*/

/*
uint8_t Read_Rx_Buffer(char *instring)
{
    uint32_t bytesread = 0;
//  extern USBD_CDC_ItfTypeDef  USBD_Interface_fops_FS;
        while(readIndex !=(APP_RX_DATA_SIZE - USB_RxBufferStart_idx ))
        {
            instring[bytesread] = USB_RxBuffer[APP_RX_DATA_SIZE - readIndex];
            if(readIndex == (0))
                readIndex = APP_RX_DATA_SIZE;
            else readIndex--;
            bytesread++;
        }
    return (int)bytesread;
}

uint8_t Read_Rx_Buffer(char *rxString)
{
	uint16_t byteCount = 0;
	//memset(rxString, 0, strlen(rxString));
	while(readIndex == (USB_RxBufferStart_idx)){}
	{
		while(readIndex != USB_RxBufferStart_idx)
		{
			rxString[byteCount] = USB_RxBuffer[readIndex];
			if(readIndex == APP_RX_DATA_SIZE)
				readIndex = 0;
			else
				readIndex++;
			byteCount++;
		}
	}
	return (int)byteCount;
}
*/

/**
  * @brief  TIM_Config: Configure TIMx timer
  * @param  None.
  * @retval None.
  */
static void TIM_Config(void)
{  
  /* Set TIMx instance */
  TimHandle.Instance = TIMx;
  
  /* Initialize TIM3 peripheral as follow:
       + Period = 10000 - 1
       + Prescaler = ((SystemCoreClock/2)/10000) - 1
       + ClockDivision = 0
       + Counter direction = Up
  */
  TimHandle.Init.Period = (CDC_POLLING_INTERVAL*1000) - 1;
  TimHandle.Init.Prescaler = 80-1;
  TimHandle.Init.ClockDivision = 0;
  TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* Add your own code here */
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
