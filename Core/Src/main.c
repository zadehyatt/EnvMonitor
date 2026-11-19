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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306.h"
#include "fonts.h"
#include "bmp280.h"
#include <stdio.h>
#include <string.h>
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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t ledState = 0;           // Current LED state (0=off, 1=on)
uint8_t lastButtonState = 1;    // Last button state (1=not pressed)

// Shift register pin definitions (update if labels didn't work)
#define SR_DATA_Port  GPIOA
#define SR_DATA_Pin   GPIO_PIN_9
#define SR_CLOCK_Port GPIOC
#define SR_CLOCK_Pin  GPIO_PIN_7
#define SR_LATCH_Port GPIOB
#define SR_LATCH_Pin  GPIO_PIN_4

// Function to send byte to shift register
void shiftOut(uint8_t value) {
  // Latch low (preparing to send data)
  HAL_GPIO_WritePin(SR_LATCH_Port, SR_LATCH_Pin, GPIO_PIN_RESET);
  
  // Send 8 bits, MSB first
  for(int i = 7; i >= 0; i--) {
    // Set data pin
    if(value & (1 << i)) {
      HAL_GPIO_WritePin(SR_DATA_Port, SR_DATA_Pin, GPIO_PIN_SET);
    } else {
      HAL_GPIO_WritePin(SR_DATA_Port, SR_DATA_Pin, GPIO_PIN_RESET);
    }
    
    // Pulse clock to shift in the bit
    HAL_GPIO_WritePin(SR_CLOCK_Port, SR_CLOCK_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SR_CLOCK_Port, SR_CLOCK_Pin, GPIO_PIN_RESET);
  }
  
  // Latch high (display the data)
  HAL_GPIO_WritePin(SR_LATCH_Port, SR_LATCH_Pin, GPIO_PIN_SET);
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
// Calculate heat index ("feels like" temperature)
float calculate_heat_index(float tempC, float humidity) {
    // Convert to Fahrenheit for the formula
    float tempF = tempC * 9.0f / 5.0f + 32.0f;
    
    // Simple formula for temperatures below 80°F
    if(tempF < 80.0f) {
        return tempC;  // No heat index adjustment needed
    }
    
    // Rothfusz regression for heat index
    float hi = -42.379f + 
               2.04901523f * tempF + 
               10.14333127f * humidity - 
               0.22475541f * tempF * humidity - 
               0.00683783f * tempF * tempF - 
               0.05481717f * humidity * humidity + 
               0.00122874f * tempF * tempF * humidity + 
               0.00085282f * tempF * humidity * humidity - 
               0.00000199f * tempF * tempF * humidity * humidity;
    
    // Convert back to Celsius
    return (hi - 32.0f) * 5.0f / 9.0f;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Redirect printf to UART2 for serial output
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// PWM Buzzer control functions
void buzzer_set_frequency(uint16_t frequency) {
    if(frequency == 0) {
        // Stop PWM
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
    } else {
        // Calculate period for desired frequency
        // Timer runs at 1 MHz (84 MHz / 84 prescaler)
        uint32_t period = 1000000 / frequency;
        __HAL_TIM_SET_AUTORELOAD(&htim1, period - 1);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, period / 2);  // 50% duty cycle
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    }
}

void buzzer_tone(uint16_t frequency, uint16_t duration_ms) {
    buzzer_set_frequency(frequency);
    HAL_Delay(duration_ms);
    buzzer_set_frequency(0);
}

void buzzer_stop(void) {
    buzzer_set_frequency(0);
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  // Initialize OLED
  ssd1306_Init(&hi2c1);
  
  // Initialize BMP280
  if(BMP280_Init(&hi2c1) == 0) {
    // BMP280 not found - show error and halt
    ssd1306_Fill(Black);
    ssd1306_SetCursor(2, 2);
    ssd1306_WriteString("BMP280", Font_5x8, White);
    ssd1306_SetCursor(2, 12);
    ssd1306_WriteString("ERROR!", Font_5x8, White);
    ssd1306_UpdateScreen(&hi2c1);
    while(1) {
      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
      HAL_Delay(200);
    }
  }
  
  // Play startup melody to confirm system ready
  buzzer_tone(523, 100);  // C5
  HAL_Delay(50);
  buzzer_tone(659, 100);  // E5
  HAL_Delay(50);
  buzzer_tone(784, 200);  // G5
  HAL_Delay(100);
  
  // Print CSV header for data logging
  printf("Time(ms),Temp(C),Humidity(%%),Pressure(hPa),FeelsLike(C)\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // Alarm system variables
  #define TEMP_LOW_THRESHOLD 20.0f   // Low temperature threshold in °C
  #define TEMP_HIGH_THRESHOLD 30.0f  // High temperature threshold in °C
  uint8_t alarmActive = 0;           // 1 if temperature is out of range
  uint8_t alarmSilenced = 0;         // 1 if user pressed button to silence
  uint32_t alarmToggleTime = 0;      // For flashing LED pattern
  
  // Display mode variables
  uint8_t displayMode = 0;           // Current display mode (0-2)
  uint8_t lastButtonRead = 1;        // For button debounce
  
  // Min/Max tracking
  float minTemp = 100.0f;            // Start high so first reading becomes min
  float maxTemp = -100.0f;           // Start low so first reading becomes max
  
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // Read BMP280 sensor data
	  BMP280_Data sensorData;
	  if(BMP280_ReadData(&hi2c1, &sensorData)) {
	    // Update min/max temperatures
	    if(sensorData.temperature < minTemp) minTemp = sensorData.temperature;
	    if(sensorData.temperature > maxTemp) maxTemp = sensorData.temperature;
	    
	    // Calculate feels like temperature
	    float feelsLike = calculate_heat_index(sensorData.temperature, sensorData.humidity);
	    
	    // Send data over serial (CSV format) - convert floats to integers for printf
	    printf("%lu,%d.%02d,%d.%02d,%d.%02d,%d.%02d\r\n", 
	           HAL_GetTick(),           // Time in milliseconds since startup
	           (int)sensorData.temperature, (int)(sensorData.temperature * 100) % 100,
	           (int)sensorData.humidity, (int)(sensorData.humidity * 100) % 100,
	           (int)sensorData.pressure, (int)(sensorData.pressure * 100) % 100,
	           (int)feelsLike, (int)(feelsLike * 100) % 100);
	    
	    // Check if temperature is out of range
	    if(sensorData.temperature < TEMP_LOW_THRESHOLD || sensorData.temperature > TEMP_HIGH_THRESHOLD) {
	      alarmActive = 1;
	    } else {
	      alarmActive = 0;
	      alarmSilenced = 0;  // Reset silence when back in range
	    }
	    
	    // Check button - different behavior based on alarm state
	    uint8_t buttonState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3);
	    if(buttonState == GPIO_PIN_RESET && lastButtonRead == GPIO_PIN_SET) {
	      // Button just pressed
	      if(alarmActive) {
	        alarmSilenced = 1;  // Silence alarm
	      } else {
	        displayMode = (displayMode + 1) % 3;  // Cycle through 3 modes
	      }
	    }
	    lastButtonRead = buttonState;
	    
	    // Increment toggle counter for alarm animations
	    alarmToggleTime++;
	    
	    // Sound buzzer if alarm is active and not silenced
	    if(alarmActive && !alarmSilenced) {
	      // Different tones for hot vs cold
	      if(alarmToggleTime % 2 == 0) {
	        if(sensorData.temperature < TEMP_LOW_THRESHOLD) {
	          buzzer_set_frequency(500);  // Low pitch for cold (500 Hz)
	        } else {
	          buzzer_set_frequency(2000); // High pitch for hot (2000 Hz)
	        }
	      } else {
	        buzzer_stop();
	      }
	    } else {
	      buzzer_stop();
	    }
	    
	    // LED bar graph behavior
	    uint8_t ledPattern = 0;
	    if(alarmActive) {
	      // Flash all LEDs if alarm is active
	      if(alarmToggleTime % 2 == 0) {
	        ledPattern = 0xFF;  // All LEDs on
	      } else {
	        ledPattern = 0x00;  // All LEDs off
	      }
	    } else {
	      // Normal mode: show temperature bar graph (map 20-30°C to 0-8 LEDs)
	      int32_t temp_int = (int32_t)(sensorData.temperature * 10);
	      uint8_t numLeds = 0;
	      
	      if(temp_int < 200) {
	        numLeds = 0;
	      } else if(temp_int >= 300) {
	        numLeds = 8;
	      } else {
	        numLeds = ((temp_int - 200) * 8) / 100;
	      }
	      
	      for(int i = 0; i < numLeds; i++) {
	        ledPattern |= (1 << i);
	      }
	    }
	    shiftOut(ledPattern);
	    
	    // Display on OLED
	    char buffer[16];
	    ssd1306_Fill(Black);
	    
	    // Show alarm status or normal header
	    if(alarmActive) {
	      ssd1306_SetCursor(2, 2);
	      ssd1306_WriteString("ALERT!", Font_5x8, White);
	      
	      // Show if too high or too low
	      ssd1306_SetCursor(2, 12);
	      if(sensorData.temperature < TEMP_LOW_THRESHOLD) {
	        ssd1306_WriteString("TOO LOW", Font_5x8, White);
	      } else {
	        ssd1306_WriteString("TOO HIGH", Font_5x8, White);
	      }
	      
	      // Show temperature on third line
	      sprintf(buffer, "T:%d.%dC", (int)sensorData.temperature, (int)(sensorData.temperature * 10) % 10);
	      ssd1306_SetCursor(2, 22);
	      ssd1306_WriteString(buffer, Font_5x8, White);
	    } else {
	      // Normal mode display - different based on displayMode
	      if(displayMode == 0) {
	        // Mode 0: Feels Like + Humidity
	        ssd1306_SetCursor(2, 2);
	        ssd1306_WriteString("FEELS LIKE", Font_5x8, White);
	        
	        sprintf(buffer, "F:%d.%dC", (int)feelsLike, (int)(feelsLike * 10) % 10);
	        ssd1306_SetCursor(2, 12);
	        ssd1306_WriteString(buffer, Font_5x8, White);
	        
	        sprintf(buffer, "H:%d%%", (int)sensorData.humidity);
	        ssd1306_SetCursor(2, 22);
	        ssd1306_WriteString(buffer, Font_5x8, White);
	        
	      } else if(displayMode == 1) {
	        // Mode 1: Actual Temp + Pressure
	        ssd1306_SetCursor(2, 2);
	        ssd1306_WriteString("ACTUAL", Font_5x8, White);
	        
	        sprintf(buffer, "T:%d.%dC", (int)sensorData.temperature, (int)(sensorData.temperature * 10) % 10);
	        ssd1306_SetCursor(2, 12);
	        ssd1306_WriteString(buffer, Font_5x8, White);
	        
	        sprintf(buffer, "P:%dHPA", (int)sensorData.pressure);
	        ssd1306_SetCursor(2, 22);
	        ssd1306_WriteString(buffer, Font_5x8, White);
	        
	      } else {
	        // Mode 2: Min/Max Temperature
	        ssd1306_SetCursor(2, 2);
	        ssd1306_WriteString("MIN/MAX", Font_5x8, White);
	        
	        sprintf(buffer, "L:%d.%dC", (int)minTemp, (int)(minTemp * 10) % 10);
	        ssd1306_SetCursor(2, 12);
	        ssd1306_WriteString(buffer, Font_5x8, White);
	        
	        sprintf(buffer, "H:%d.%dC", (int)maxTemp, (int)(maxTemp * 10) % 10);
	        ssd1306_SetCursor(2, 22);
	        ssd1306_WriteString(buffer, Font_5x8, White);
	      }
	    }
	    
	    ssd1306_UpdateScreen(&hi2c1);
	  }
	  
	  HAL_Delay(500);  // Update twice per second for responsive alarm
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 84;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|SR_DATA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SR_CLK_GPIO_Port, SR_CLK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SR_LATCH_GPIO_Port, SR_LATCH_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin SR_DATA_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|SR_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SR_CLK_Pin */
  GPIO_InitStruct.Pin = SR_CLK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SR_CLK_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : SR_LATCH_Pin */
  GPIO_InitStruct.Pin = SR_LATCH_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SR_LATCH_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
