#include "main.h"
#include "lcd.h"
#include <stdio.h>

ADC_HandleTypeDef hadc1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
uint32_t Read_ADC(uint32_t channel);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();

  initLCD();
  clearLCD();

  char buffer[17];
  uint32_t raw_v = 0;
  uint32_t raw_i = 0;
  float v_adc = 0.0f;
  float i_adc = 0.0f;
  float v_bat = 0.0f;
  float i_bat = 0.0f;

  while (1)
  {
    /* 1. Baca Nilai ADC */
    raw_v = Read_ADC(ADC_CHANNEL_0); // PA0 (Tegangan)[cite: 4]
    raw_i = Read_ADC(ADC_CHANNEL_1); // PA1 (Arus)[cite: 4]

    /* 2. Hitung Tegangan ADC */
    v_adc = ((float)raw_v / 4095.0f) * 3.3f;
    i_adc = ((float)raw_i / 4095.0f) * 3.3f;

    /* 3. Kalkulasi Nilai Baterai */
    v_bat = v_adc * ((38.0f + 4.7f) / 4.7f); // Pembagi tegangan R1=4.7k, R2=38k
    i_bat = i_adc * 10.0f;                   // Skala simulasi arus

    /* 4. Format Nilai Tegangan Tanpa %f */
    int v_int = (int)v_bat;
    int v_dec = (int)((v_bat - v_int) * 100);
    if (v_dec < 0) v_dec = 0;
    snprintf(buffer, sizeof(buffer), "V: %2d.%02d V", v_int, v_dec);
    setCursor(0, 0);
    writeLCD(buffer);

    /* 5. Format Nilai Arus Tanpa %f */
    int i_int = (int)i_bat;
    int i_dec = (int)((i_bat - i_int) * 100);
    if (i_dec < 0) i_dec = 0;
    snprintf(buffer, sizeof(buffer), "I: %2d.%02d A", i_int, i_dec);
    setCursor(0, 1);
    writeLCD(buffer);

    HAL_Delay(250);
  }
}

uint32_t Read_ADC(uint32_t channel)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  sConfig.Channel = channel;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 100);
  uint32_t val = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  return val;
}

static void MX_ADC1_Init(void)
{
  __HAL_RCC_ADC1_CLK_ENABLE();

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  HAL_ADC_Init(&hadc1);
}

static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1; // PA0 & PA1 Mode Analog[cite: 4]
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}
