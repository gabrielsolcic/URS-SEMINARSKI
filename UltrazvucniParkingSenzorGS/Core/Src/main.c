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
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DIST_SIGURNO   50
#define DIST_OPREZ     30
#define DIST_BLIZU     15
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t sustav_aktivan = 1;
uint8_t mjerenje_pauzirano = 0;
uint8_t sustav_resetiran = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < us);
}

uint32_t mjeri_udaljenost(void)
{
    uint32_t t1, t2;
    uint32_t timeout;

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

    timeout = HAL_GetTick() + 30;
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        if (HAL_GetTick() > timeout) return 999;
    }
    t1 = __HAL_TIM_GET_COUNTER(&htim1);

    timeout = HAL_GetTick() + 30;
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET)
    {
        if (HAL_GetTick() > timeout) return 999;
    }
    t2 = __HAL_TIM_GET_COUNTER(&htim1);

    if (t2 < t1) t2 += 65536;
    return (t2 - t1) * 17 / 1000;
}

#define LCD_ADDR  (0x27 << 1)
#define LCD_RS    0x01
#define LCD_EN    0x04
#define LCD_BL    0x08

void lcd_i2c_send(uint8_t data)
{
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, &data, 1, 10);
}

void lcd_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t d = (nibble & 0xF0) | LCD_BL | rs;
    lcd_i2c_send(d | LCD_EN);
    HAL_Delay(1);
    lcd_i2c_send(d & ~LCD_EN);
    HAL_Delay(1);
}

void lcd_byte(uint8_t b, uint8_t rs)
{
    lcd_nibble(b & 0xF0, rs);
    lcd_nibble((b << 4) & 0xF0, rs);
}

void lcd_cmd(uint8_t cmd)   { lcd_byte(cmd, 0); }
void lcd_char(char c)       { lcd_byte((uint8_t)c, LCD_RS); }

void lcd_print(const char *s)
{
    while (*s) lcd_char(*s++);
}

void lcd_pos(uint8_t row, uint8_t col)
{
    lcd_cmd((row == 0 ? 0x80 : 0xC0) + col);
}

void lcd_clear(void)
{
    lcd_cmd(0x01);
    HAL_Delay(2);
}

void lcd_init(void)
{
    HAL_Delay(50);
    lcd_nibble(0x30, 0); HAL_Delay(5);
    lcd_nibble(0x30, 0); HAL_Delay(1);
    lcd_nibble(0x30, 0); HAL_Delay(1);
    lcd_nibble(0x20, 0); HAL_Delay(1);
    lcd_cmd(0x28);
    lcd_cmd(0x08);
    lcd_cmd(0x01); HAL_Delay(2);
    lcd_cmd(0x06);
    lcd_cmd(0x0C);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1);



  lcd_init();
  lcd_clear();
  lcd_pos(0, 0);
  lcd_print("Parking senzor");
  lcd_pos(1, 0);
  lcd_print("  Pokretanje...");
  HAL_Delay(2000);
  lcd_clear();

  uint32_t udaljenost = 0;
  uint32_t zadnji_blink = 0;
  uint8_t buzzer_stanje = 0;
  uint32_t period_blinka = 0;
  char lcd_redak[17];
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(sustav_resetiran)
	      {
	          sustav_resetiran = 0;
	          lcd_clear();
	          if(sustav_aktivan)
	          {
	              lcd_pos(0, 0);
	              lcd_print("  SUSTAV AKTIVAN");
	              HAL_Delay(1000);
	              lcd_clear();
	          }
	          else
	          {
	              lcd_pos(0, 0);
	              lcd_print("  SUSTAV UGASEN ");
	          }
	      }

	      if(!sustav_aktivan)
	      {
	          HAL_Delay(100);
	          continue;
	      }

	      if(mjerenje_pauzirano)
	      {
	    	  lcd_pos(1, 0);
	    	      lcd_print("   PAUZIRANO    ");
	    	      HAL_Delay(100);
	    	      continue;
	      }

	      udaljenost = mjeri_udaljenost();

	      lcd_pos(0, 0);
	      if (udaljenost >= 999)
	          lcd_print("Dist:  --- cm   ");
	      else
	      {
	          snprintf(lcd_redak, sizeof(lcd_redak), "Dist: %3lu cm    ", udaljenost);
	          lcd_print(lcd_redak);
	      }

	      if (udaljenost > DIST_SIGURNO)
	      {
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
	          HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
	          period_blinka = 0;
	          lcd_pos(1, 0);
	          lcd_print("   SIGURNO      ");
	      }
	      else if (udaljenost > DIST_OPREZ)
	      {
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
	          period_blinka = 500;
	          lcd_pos(1, 0);
	          lcd_print("   OPREZ!       ");
	      }
	      else if (udaljenost > DIST_BLIZU)
	      {
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
	          period_blinka = 150;
	          lcd_pos(1, 0);
	          lcd_print("  PAZI! BLIZU   ");
	      }
	      else
	      {
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
	          __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 500);
	          HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
	          period_blinka = 0;
	          lcd_pos(1, 0);
	          lcd_print("  !! STANI !!   ");
	      }

	      if (period_blinka > 0)
	      {
	          uint32_t sad = HAL_GetTick();
	          if (sad - zadnji_blink >= period_blinka)
	          {
	              zadnji_blink = sad;
	              buzzer_stanje = !buzzer_stanje;
	              if (buzzer_stanje)
	              {
	                  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 500);
	                  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
	              }
	              else
	              {
	                  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
	              }
	          }
	      }

	      HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_13)
	    {
	        sustav_aktivan = !sustav_aktivan;
	        if(!sustav_aktivan)
	        {
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
	            HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
	        }
	        sustav_resetiran = 1;
	    }
	    if(GPIO_Pin == GPIO_PIN_8)
	    {
	        if(sustav_aktivan)
	        {
	            mjerenje_pauzirano = !mjerenje_pauzirano;
	        }
	    }
	}


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
