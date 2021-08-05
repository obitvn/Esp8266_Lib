#include "esp8266.h"

uint8_t  Esp8266_UartBuffer[ESP8266_UART_BUFFER_MAX_SIZE];
uint16_t Esp8266_UartRxIndex;
uint32_t Esp8266_UartLastTime;
uint8_t  Esp8266_DataBuff;

uint8_t ESP8266_Init()
{
	//HAL_UART_Receive_IT(&ESP8266_UART,&Esp8266_DataBuff,1);
	Esp8266_SendAtCommand((uint8_t*)"ATE0\r\n",6,2000,(uint8_t*)"\r\nOK\r\n",6,1);
	Esp8266_SendAtNotWaitResponse((uint8_t*)"AT+RST\r\n",8);
	HAL_Delay(300);
	if(Esp8266_SendAtCommand((uint8_t*)"AT\r\n",4,2000,(uint8_t*)"\r\nOK\r\n",6,1)) 
		return 1;
	else 
		return 0;
}

uint8_t ESP8266_JoinAccessPoint(const char* _SSID, const char* _PASSWORD)
{
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
	_atCommand[59] = 0;
	if(Esp8266_SendAtCommand((uint8_t *)_atCommand,strlen(_atCommand),5000,(uint8_t*)"\r\nWIFI CONNECTED\r\n",18,1))
		return ESP8266_WIFI_CONNECTED;
	else
	{
		if(strstr((char *)Esp8266_UartBuffer, "+CWJAP:1"))
		return ESP8266_CONNECTION_TIMEOUT;
		else if(strstr((char *)Esp8266_UartBuffer, "+CWJAP:2"))
		return ESP8266_WRONG_PASSWORD;
		else if(strstr((char *)Esp8266_UartBuffer, "+CWJAP:3"))
		return ESP8266_NOT_FOUND_TARGET_AP;
		else if(strstr((char *)Esp8266_UartBuffer, "+CWJAP:4"))
		return ESP8266_CONNECTION_FAILED;
		else
		return ESP8266_JOIN_UNKNOWN_ERROR;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
/*
Hardware layer
*/
////////////////////////////////////////////////////////////////////////////////////////////////

void ESP8266_USART_IRQHandler()
{
	  Esp8266_UartLastTime = HAL_GetTick();
		if(Esp8266_UartRxIndex < ESP8266_UART_BUFFER_MAX_SIZE - 1)
		{
			Esp8266_UartRxIndex++;
		}
		Esp8266_UartBuffer[Esp8266_UartRxIndex] = 0; // clear 
		Esp8266_UartBuffer[Esp8266_UartRxIndex] = Esp8266_DataBuff;
		HAL_UART_Receive_IT(&ESP8266_UART,&Esp8266_DataBuff,1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Esp8266_SendAtCommand(uint8_t* AtCommand, uint8_t AtLength, uint32_t Time_out, uint8_t* answer, uint8_t AnsLength, uint8_t clear_buffer)
{
	
	// clear_buffer = 1 ==> xoa buffer khi nhan duoc chuoi chinh xac
	uint8_t str1;
	uint16_t SizeUartBuffer;
	uint32_t SendCommandStartTime = HAL_GetTick();
	uint32_t TimeCommandWait;
	//Esp8266_SendAtNotWaitResponse(AtCommand);
	while(HAL_GetTick() - SendCommandStartTime < Time_out)
	{
		TimeCommandWait = HAL_GetTick() - Esp8266_UartLastTime ;
		if(( TimeCommandWait > 50)&&(Esp8266_UartRxIndex> AnsLength-1))
		{
        SizeUartBuffer = Esp8266_UartRxIndex;
				str1 = obit_strcmp(answer,AnsLength,Esp8266_UartBuffer,SizeUartBuffer);
			  //str1 = obit_strcmp("\r\nOK\r\n",6,"\r\n\r\nss\r\nOK\r\n",12);
				if(str1==1)
				{
					if(clear_buffer)
					{
					Esp8266_UartRxIndex=0;
					memset((char*)Esp8266_UartBuffer,0,ESP8266_UART_BUFFER_MAX_SIZE);
					}
					return 1;
				}
				else
		    {
			  Esp8266_SendAtNotWaitResponse(AtCommand, sizeof(AtCommand));
			  HAL_Delay(200);
		    }
		}
		
		else
		{
			Esp8266_SendAtNotWaitResponse(AtCommand, sizeof(AtCommand));
			HAL_Delay(200);
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////

void Esp8266_SendAtNotWaitResponse(uint8_t *str, uint16_t str_len)
{
	HAL_UART_Transmit(&ESP8266_UART, str, str_len,100);
}

//////////////////////////////////////////////////////////////////////////////////////////

uint8_t WaitForStringInTimeOut( uint8_t *str, uint16_t str_len, uint16_t time_out)
{
	uint32_t  StartTime = HAL_GetTick();
	while(HAL_GetTick() - StartTime < time_out)
	{
		if((HAL_GetTick() - Esp8266_UartLastTime>50)&&(Esp8266_UartRxIndex>4))
	  {
	   if(obit_strcmp(str,str_len,Esp8266_UartBuffer,Esp8266_UartRxIndex)) return 1;
		}
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

void WaitForString( uint8_t *str, uint16_t str_len)
{
	while(obit_strcmp(str,str_len,Esp8266_UartBuffer,Esp8266_UartRxIndex) == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////

