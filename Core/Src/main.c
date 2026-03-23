/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "HC_SR04.h"
#include "delay.h"
#include "esp8266.h"
#include "ds1302.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern volatile uint8_t timer1_second_flag;
extern volatile uint16_t timer1_counter;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  uint8_t wifi_try = 0, mqtt_try = 0;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  HC_SR04_Init();
  DS1302_Init();
  
  Delay_Init();
  
  // 启动TIM1
  HAL_TIM_Base_Start_IT(&htim1);
  
  // 初始设置时间（2026-03-23 12:00:00）
  DS1302_Time init_time;
  init_time.year = 26;
  init_time.month = 3;
  init_time.date = 23;
  init_time.day = 6; // 0=Sunday, 6=Saturday
  init_time.hour = 12;
  init_time.minute = 0;
  init_time.second = 0;
  DS1302_SetTime(&init_time);
  
  while (wifi_try < 5 && !ESP8266_ConnectWiFi())
  {
		 printf("WiFi connect retry\r\n");
      wifi_try++;
      HAL_Delay(1000);
  }
	
	
	while(mqtt_try<20&&!ESP8266_ConnectCloud())
	{
		 printf("ConnectCloud retry\r\n");
      mqtt_try++;

      delay_ms(2000);
	}
	
	if(mqtt_try>20)
	{
							OLED_ShowString(0,0,(uint8_t*)"ConnectCloud failed",8,1);
			OLED_Refresh();
		  while(1);
	}
	 printf("ConnectCloud ok\r\n");

	if(!ESP8266_MQTT_Subscribe(MQTT_TOPIC_POST_REPLY,1))
	{
		 printf("MQTT subscribe post_reply failed\r\n");
		  OLED_Clear();
			OLED_ShowString(0,0,(uint8_t*)"ConnectCloud failed",8,1);
			OLED_Refresh();
		  while(1);
	}
	
	
	OLED_ShowString(0,0,(uint8_t*)"Distance:",8,1);
	OLED_ShowString(0,8,(uint8_t*)"Time:",8,1);
	OLED_ShowString(0,16,(uint8_t*)"Date:",8,1);
	OLED_Refresh();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	printf("go\r\n");
    float distance = HC_SR04_MeasureDistance();
    if(distance >= 0)
    {
        uint32_t int_part = (uint32_t)distance;
        uint32_t decimal_part = (uint32_t)((distance - int_part) * 100);
        OLED_ShowNum(70,0,int_part,1,8,1);
        OLED_ShowChar(78,0,'.',8,1);
        OLED_ShowNum(86,0,decimal_part,2,8,1);
        OLED_ShowString(94,0,(uint8_t*)"cm",8,1);
        
        // 刷新距离显示
        OLED_Refresh();
    }
    else
    {
        //OLED_ShowString(70,0,(uint8_t*)"Err",8,1);
    }
    
    // 1秒显示一次时间
    if(timer1_second_flag)
    {
      timer1_second_flag = 0;
      DS1302_Time time;
      DS1302_GetTime(&time);
      
      // 显示时间：HH:MM:SS (第二行，位置8)
      OLED_ShowNum(40,8,time.hour,2,8,1);
      OLED_ShowChar(56,8,':',8,1);
      OLED_ShowNum(64,8,time.minute,2,8,1);
      OLED_ShowChar(80,8,':',8,1);
      OLED_ShowNum(88,8,time.second,2,8,1);
      
      // 显示日期：YYYY-MM-DD (第三行，位置16)
      OLED_ShowNum(40,16,2000 + time.year,4,8,1);
      OLED_ShowChar(72,16,'-',8,1);
      OLED_ShowNum(80,16,time.month,2,8,1);
      OLED_ShowChar(96,16,'-',8,1);
      OLED_ShowNum(104,16,time.date,2,8,1);
      
      // 只有在更新时间时才刷新OLED显示
      OLED_Refresh();
    }
    
    // 保持主循环运行，不使用delay_ms阻塞
    delay_ms(100);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
