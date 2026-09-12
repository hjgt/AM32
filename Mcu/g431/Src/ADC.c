/*
 * ADC.c
 *
 *  Created on: May 20, 2020
 *      Author: Alka
 */
 #include "ADC.h"


 #ifdef USE_ADC_1_2
 uint16_t ADC1DataDMA[2];
 uint16_t ADC2DataDMA[2];
 #else

 #ifdef USE_ADC_INPUT
 uint16_t ADCDataDMA[4];
 #else
 uint16_t ADCDataDMA[3];
 #endif
#ifdef USE_ADC2_VOLTAGE_SENSE
 uint16_t ADC2SensorDataDMA[2];
#endif
 #endif
 
 extern uint16_t ADC_raw_temp;
 extern uint16_t ADC_raw_volts;
 extern uint16_t ADC_raw_current;
 extern uint16_t ADC_raw_input;
 extern uint16_t ADC_raw_ntc;

#define ADC_DELAY_CALIB_ENABLE_CPU_CYCLES \
    (LL_ADC_DELAY_CALIB_ENABLE_ADC_CYCLES * 64)

#ifndef ADC_INTERNAL_TEMP_SAMPLING_TIME
#define ADC_INTERNAL_TEMP_SAMPLING_TIME LL_ADC_SAMPLINGTIME_47CYCLES_5
#endif

 void ADC_DMA_Callback()
{ // read dma buffer and set extern variables
 #ifdef USE_ADC_1_2
     ADC_raw_temp = ADC1DataDMA[0];
     ADC_raw_ntc = ADC1DataDMA[1];
     ADC_raw_volts = ADC2DataDMA[0];
     ADC_raw_current = ADC2DataDMA[1];
  
 #else

     ADC_raw_temp = ADCDataDMA[0];
#ifdef USE_ADC2_VOLTAGE_SENSE
     ADC_raw_volts = ADC2SensorDataDMA[0];
#ifdef USE_ADC2_CURRENT_SENSE
     ADC_raw_current = ADC2SensorDataDMA[1];
#else
     ADC_raw_current = 0u;
#endif
#else
     ADC_raw_volts = ADCDataDMA[1];
     ADC_raw_current = ADCDataDMA[2];
#endif
 #endif
 }

 void enableADC_DMA()
{
 #ifdef USE_ADC_1_2
//    NVIC_SetPriority(DMA1_Channel3_IRQn, 3);
//    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
//  
//    NVIC_SetPriority(DMA1_Channel2_IRQn, 3);
//    NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    LL_DMA_ConfigAddresses(
        DMA1, LL_DMA_CHANNEL_4,
        LL_ADC_DMA_GetRegAddr(ADC2, LL_ADC_DMA_REG_REGULAR_DATA),
        (uint32_t)&ADC2DataDMA, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  
      LL_DMA_ConfigAddresses(
        DMA1, LL_DMA_CHANNEL_2,
        LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
        (uint32_t)&ADC1DataDMA, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

    /* Set DMA transfer size */

    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, 2);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, 2);
 

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
  
  #else
  // enables channel

    NVIC_SetPriority(DMA1_Channel2_IRQn, 3);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    LL_DMA_ConfigAddresses(
        DMA1, LL_DMA_CHANNEL_2,
        LL_ADC_DMA_GetRegAddr(ADC1, LL_ADC_DMA_REG_REGULAR_DATA),
        (uint32_t)&ADCDataDMA, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

#ifdef USE_ADC2_VOLTAGE_SENSE
    LL_DMA_ConfigAddresses(
        DMA1, LL_DMA_CHANNEL_4,
        LL_ADC_DMA_GetRegAddr(ADC2, LL_ADC_DMA_REG_REGULAR_DATA),
        (uint32_t)&ADC2SensorDataDMA, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
#endif

    /* Set DMA transfer size */
#ifdef USE_ADC2_VOLTAGE_SENSE
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, 1);
#ifdef USE_ADC2_CURRENT_SENSE
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, 2);
#else
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, 1);
#endif
 #elif defined(USE_ADC_INPUT)
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, 4);
 #else
  #ifdef USE_CURRENT_SENSE
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, 3);
  #else
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, 2);
  #endif
 #endif

    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
#ifdef USE_ADC2_VOLTAGE_SENSE
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
#endif
    
    #endif
}

void activateADC(void)
{
#ifdef USE_ADC_1_2
  __IO uint32_t wait_loop_index = 0U;
  #if (USE_TIMEOUT == 1)
  uint32_t Timeout = 0U; /* Variable used for timeout management */
  #endif /* USE_TIMEOUT */
  if (LL_ADC_IsEnabled(ADC1) == 0)
  {
    LL_ADC_DisableDeepPowerDown(ADC1);
    LL_ADC_EnableInternalRegulator(ADC1);
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }
    LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
    #if (USE_TIMEOUT == 1)
    Timeout = ADC_CALIBRATION_TIMEOUT_MS;
    #endif /* USE_TIMEOUT */
        while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
    {
    }
    wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }
    LL_ADC_Enable(ADC1);
    
    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
    {
    }
  } 
  
    if (LL_ADC_IsEnabled(ADC2) == 0)
  {
    LL_ADC_DisableDeepPowerDown(ADC2);
    LL_ADC_EnableInternalRegulator(ADC2);
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }
    LL_ADC_StartCalibration(ADC2, LL_ADC_SINGLE_ENDED);
    #if (USE_TIMEOUT == 1)
    Timeout = ADC_CALIBRATION_TIMEOUT_MS;
    #endif /* USE_TIMEOUT */
        while (LL_ADC_IsCalibrationOnGoing(ADC2) != 0)
    {
    }
    wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }
    LL_ADC_Enable(ADC2);
    
    while (LL_ADC_IsActiveFlag_ADRDY(ADC2) == 0)
    {
    }
  } 
  
  
#else  
  __IO uint32_t wait_loop_index = 0U;
  #if (USE_TIMEOUT == 1)
  uint32_t Timeout = 0U; /* Variable used for timeout management */
  #endif /* USE_TIMEOUT */
  if (LL_ADC_IsEnabled(ADC1) == 0)
  {
    /* Disable ADC deep power down (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(ADC1);
    
    /* Enable ADC internal voltage regulator */
    LL_ADC_EnableInternalRegulator(ADC1);
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }
    
    /* Run ADC self calibration */
    LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED);
    
    /* Poll for ADC effectively calibrated */
    #if (USE_TIMEOUT == 1)
    Timeout = ADC_CALIBRATION_TIMEOUT_MS;
    #endif /* USE_TIMEOUT */
    
    while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
    {
    }
    wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }
    
    /* Enable ADC */
    LL_ADC_Enable(ADC1);
    
    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
    {
    }
  }
#ifdef USE_ADC2_VOLTAGE_SENSE
  if (LL_ADC_IsEnabled(ADC2) == 0u)
  {
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US *
        (SystemCoreClock / (100000 * 2))) / 10);
    while (wait_loop_index != 0u)
    {
      wait_loop_index--;
    }
    LL_ADC_StartCalibration(ADC2, LL_ADC_SINGLE_ENDED);
    while (LL_ADC_IsCalibrationOnGoing(ADC2) != 0u)
    {
    }
    wait_loop_index = (ADC_DELAY_CALIB_ENABLE_CPU_CYCLES >> 1);
    while (wait_loop_index != 0u)
    {
      wait_loop_index--;
    }
    LL_ADC_Enable(ADC2);
    while (LL_ADC_IsActiveFlag_ADRDY(ADC2) == 0u)
    {
    }
  }
#endif
#ifdef USE_ADC_ZCD
  /*
   * Route C: arm the ADC1 injected group once here (after the ADC is enabled).
   * With JEXTSEL = TIM1_CH4 and FALLING-edge trigger (OC4REF falls at
   * CNT == CCR4 == duty/2 == ON-pulse midpoint), this enables external
   * triggering; the sequence is then launched automatically by every TIM1 CH4
   * OC4REF falling edge. Results are read from JDR1/2/3 in getCompOutputLevel()
   * without any further software start. See ADC_Init for the edge rationale.
   */
  LL_ADC_INJ_StartConversion(ADC1);
#endif
#endif
}


#ifdef USE_ADC_1_2

void ADC_Init(void){
  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  LL_ADC_InitTypeDef ADC_InitStruct = {0};
  LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  /**ADC2 GPIO Configuration
  PA6   ------> ADC2_IN3
  PA7   ------> ADC2_IN4
  pb1  -------> adc1_in12
  */

  GPIO_InitStruct.Pin = NTC_ADC_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = VOLTAGE_ADC_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = CURRENT_ADC_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);


  /*BEGIN ADC1 SETUP */

  LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};

  LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);


  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_ADC1);
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);
  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_CIRCULAR);
  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_WORD);
  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_HALFWORD);

  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
  LL_ADC_Init(ADC1, &ADC_InitStruct);
  ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS;
  ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
  ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
  LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
  LL_ADC_SetGainCompensation(ADC1, 0);
  LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
  ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
  ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
  LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);

  LL_ADC_DisableDeepPowerDown(ADC1);
  LL_ADC_EnableInternalRegulator(ADC1);

  uint32_t wait_loop_index;
  wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
  while(wait_loop_index != 0)
  {
    wait_loop_index--;
  }

  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
  LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1,
      ADC_INTERNAL_TEMP_SAMPLING_TIME);
  LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SINGLE_ENDED);
  LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_TEMPSENSOR);

  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2, NTC_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC1, VOLTAGE_ADC_CHANNEL, LL_ADC_SAMPLINGTIME_47CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC1, VOLTAGE_ADC_CHANNEL, LL_ADC_SINGLE_ENDED);
 

  /*ADC 2 setup */
  
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_4, LL_DMAMUX_REQ_ADC2);
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_4, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PRIORITY_LOW);
  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MODE_CIRCULAR);
  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_4, LL_DMA_PDATAALIGN_WORD);
  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MDATAALIGN_HALFWORD);


  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
  LL_ADC_Init(ADC2, &ADC_InitStruct);
  ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS;
  ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
  ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
  LL_ADC_REG_Init(ADC2, &ADC_REG_InitStruct);
  LL_ADC_SetGainCompensation(ADC2, 0);
  LL_ADC_SetOverSamplingScope(ADC2, LL_ADC_OVS_DISABLE);
  LL_ADC_DisableDeepPowerDown(ADC2);
  LL_ADC_EnableInternalRegulator(ADC2);

  wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
  while(wait_loop_index != 0)
  {
    wait_loop_index--;
  }

  LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_3);
  LL_ADC_SetChannelSamplingTime(ADC2, LL_ADC_CHANNEL_3, LL_ADC_SAMPLINGTIME_2CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC2, LL_ADC_CHANNEL_3, LL_ADC_SINGLE_ENDED);

  LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_2, LL_ADC_CHANNEL_4);
  LL_ADC_SetChannelSamplingTime(ADC2, LL_ADC_CHANNEL_4, LL_ADC_SAMPLINGTIME_47CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC2, LL_ADC_CHANNEL_4, LL_ADC_SINGLE_ENDED);
}
#else




void ADC_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  LL_ADC_InitTypeDef ADC_InitStruct = {0};
  LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
  LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_RCC_SetADCClockSource(LL_RCC_ADC12_CLKSOURCE_SYSCLK);

  /* Peripheral clock enable */
  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC12);

  LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
  /**ADC1 GPIO Configuration
  PA3   ------> ADC1_IN4
  */
  GPIO_InitStruct.Pin = VOLTAGE_ADC_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
 #ifdef USE_CURRENT_SENSE
  GPIO_InitStruct.Pin = CURRENT_ADC_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif

#ifdef USE_ADC_ZCD
  /* BEMF A=PA0, B=PA1, C=PA2, Neutral=PA5 — configure as analog inputs */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_5;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif
  /* ADC1 DMA Init */

  /* ADC1 Init */
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_ADC1);

  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_2, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MODE_CIRCULAR);

  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_PDATAALIGN_WORD);

  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_2, LL_DMA_MDATAALIGN_HALFWORD);

#ifdef USE_ADC2_VOLTAGE_SENSE
  LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_4, LL_DMAMUX_REQ_ADC2);
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_4,
      LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
  LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_4,
      LL_DMA_PRIORITY_LOW);
  LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_4, LL_DMA_MODE_CIRCULAR);
  LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_4,
      LL_DMA_PERIPH_NOINCREMENT);
  LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_4,
      LL_DMA_MEMORY_INCREMENT);
  LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_4,
      LL_DMA_PDATAALIGN_WORD);
  LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_4,
      LL_DMA_MDATAALIGN_HALFWORD);
#endif

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
  LL_ADC_Init(ADC1, &ADC_InitStruct);
  ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
#ifdef USE_ADC2_VOLTAGE_SENSE
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
#else
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS;
#endif
  ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
  ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
  LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
  LL_ADC_SetGainCompensation(ADC1, 0);
  LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
  ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
  ADC_CommonInitStruct.Multimode = LL_ADC_MULTI_INDEPENDENT;
  LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);

  /* Disable ADC deep power down (enabled by default after reset state) */
  LL_ADC_DisableDeepPowerDown(ADC1);
  /* Enable ADC internal voltage regulator */
  LL_ADC_EnableInternalRegulator(ADC1);
  /* Delay for ADC internal voltage regulator stabilization. */
  /* Compute number of CPU cycles to wait for, from delay in us. */
  /* Note: Variable divided by 2 to compensate partially */
  /* CPU processing cycles (depends on compilation optimization). */
  /* Note: If system core clock frequency is below 200kHz, wait time */
  /* is only a few CPU processing cycles. */
  uint32_t wait_loop_index;
  wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
  while(wait_loop_index != 0)
  {
    wait_loop_index--;
  }

  /** Configure Regular Channel
  */
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
  LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1,
      ADC_INTERNAL_TEMP_SAMPLING_TIME);
  LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1, LL_ADC_SINGLE_ENDED);
  LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_TEMPSENSOR);

#ifndef USE_ADC2_VOLTAGE_SENSE
  /** Configure Regular Channel */
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2, VOLTAGE_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC1, VOLTAGE_ADC_CHANNEL, LL_ADC_SAMPLINGTIME_47CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC1, VOLTAGE_ADC_CHANNEL, LL_ADC_SINGLE_ENDED);
#endif
#ifdef USE_CURRENT_SENSE
#ifndef USE_ADC2_CURRENT_SENSE
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_3, CURRENT_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC1, CURRENT_ADC_CHANNEL, LL_ADC_SAMPLINGTIME_47CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC1, CURRENT_ADC_CHANNEL, LL_ADC_SINGLE_ENDED);
#endif
#endif

#ifdef USE_ADC2_VOLTAGE_SENSE
  /* Board sensors use ADC2 so ADC1's PWM-synchronous injected BEMF group is
   * left byte-for-byte on its proven control path. */
  LL_ADC_Init(ADC2, &ADC_InitStruct);
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
#ifdef USE_ADC2_CURRENT_SENSE
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS;
#endif
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_LIMITED;
  LL_ADC_REG_Init(ADC2, &ADC_REG_InitStruct);
  LL_ADC_SetGainCompensation(ADC2, 0);
  LL_ADC_SetOverSamplingScope(ADC2, LL_ADC_OVS_DISABLE);
  LL_ADC_DisableDeepPowerDown(ADC2);
  LL_ADC_EnableInternalRegulator(ADC2);

  LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_1,
      VOLTAGE_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC2, VOLTAGE_ADC_CHANNEL,
      LL_ADC_SAMPLINGTIME_47CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC2, VOLTAGE_ADC_CHANNEL,
      LL_ADC_SINGLE_ENDED);
#ifdef USE_ADC2_CURRENT_SENSE
  LL_ADC_REG_SetSequencerRanks(ADC2, LL_ADC_REG_RANK_2,
      CURRENT_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC2, CURRENT_ADC_CHANNEL,
      LL_ADC_SAMPLINGTIME_47CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC2, CURRENT_ADC_CHANNEL,
      LL_ADC_SINGLE_ENDED);
#endif
#endif

#ifdef USE_ADC_ZCD
  /*
   * Route C: PWM-synchronous BEMF sampling via the ADC1 INJECTED group.
   *
   * The injected group is independent of the DMA-driven regular group
   * (temp/volts/current), so telemetry is unaffected. It is hardware-triggered
   * by TIM1 CH4 (JEXTSEL = TIM1_CH4). SET_DUTY_CYCLE_ALL normally places the
   * trigger near the ON-pulse midpoint, but moves it earlier when necessary so
   * the complete three-rank sequence finishes before the PWM falling edge.
   * Pulses too narrow for a clean sequence do not generate a falling trigger.
   *
   * All three phase dividers are sampled every trigger (rank 1/2/3 = A/B/C).
   * getCompOutputSample() reads JDR1/2/3, computes the software neutral
   * (Va+Vb+Vc)/3, and picks the floating phase per the current commutation
   * step. The injected group is armed once (LL_ADC_INJ_StartConversion) in
   * activateADC() after the ADC is enabled; thereafter every TIM1 CH4 event
   * launches a fresh 3-rank conversion with no CPU intervention.
   */
  /*
   * Trigger EDGE must be FALLING, not rising. TIM1 is edge-aligned
   * (COUNTERMODE_UP) and CH4 is PWM1 mode, so OC4REF is HIGH while
   * CNT < CCR4 and drops LOW at CNT == CCR4. The external-trigger source
   * seen by the ADC is OC4REF:
   *   - its RISING edge occurs at CNT ~ 0 (period start, high-side just
   *     switched on -> dead-time + switching ringing -> BEMF reads ~0),
   *   - its FALLING edge occurs at CNT == CCR4 inside the clean ON window.
   * The 1-minute capture with RISING showed va/vb/vc stuck near 0 counts
   * (sampling at the period start), so the injected group never saw real
   * BEMF and the loop never closed. Using FALLING moves the sample to the
   * ON midpoint. See porting notes section 12.13.
   */
  LL_ADC_INJ_SetTriggerSource(ADC1, LL_ADC_INJ_TRIG_EXT_TIM1_CH4);
  LL_ADC_INJ_SetTriggerEdge(ADC1, LL_ADC_INJ_TRIG_EXT_FALLING);
  LL_ADC_INJ_SetSequencerLength(ADC1, LL_ADC_INJ_SEQ_SCAN_ENABLE_3RANKS);

  LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_1, BEMF_A_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC1, BEMF_A_ADC_CHANNEL, LL_ADC_SAMPLINGTIME_6CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC1, BEMF_A_ADC_CHANNEL, LL_ADC_SINGLE_ENDED);

  LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_2, BEMF_B_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC1, BEMF_B_ADC_CHANNEL, LL_ADC_SAMPLINGTIME_6CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC1, BEMF_B_ADC_CHANNEL, LL_ADC_SINGLE_ENDED);

  LL_ADC_INJ_SetSequencerRanks(ADC1, LL_ADC_INJ_RANK_3, BEMF_C_ADC_CHANNEL);
  LL_ADC_SetChannelSamplingTime(ADC1, BEMF_C_ADC_CHANNEL, LL_ADC_SAMPLINGTIME_6CYCLES_5);
  LL_ADC_SetChannelSingleDiff(ADC1, BEMF_C_ADC_CHANNEL, LL_ADC_SINGLE_ENDED);
#endif /* USE_ADC_ZCD */

}
#endif

#ifdef USE_ADC_ZCD
/**
 * readADC_ZCD() — single-shot blocking ADC read on ADC1.
 *
 * Temporarily reconfigures the ADC1 regular sequence to a single rank
 * (the requested BEMF/neutral channel), triggers a software conversion,
 * waits for end-of-conversion, then returns the 12-bit result.
 *
 * The DMA-driven background scan (temp + voltage) uses LL_ADC_REG_DMA_TRANSFER_LIMITED,
 * so DMA stops after each sequence; it is safe to retrigger with a different
 * single-channel sequence between DMA callbacks.
 *
 * @param channel  LL_ADC_CHANNEL_x (e.g. BEMF_A_ADC_CHANNEL)
 * @return         12-bit ADC result
 */
uint16_t readADC_ZCD(uint32_t channel)
{
    /* Stop any ongoing conversion */
    if (LL_ADC_REG_IsConversionOngoing(ADC1)) {
        LL_ADC_REG_StopConversion(ADC1);
        while (LL_ADC_REG_IsStopConversionOngoing(ADC1)) {}
    }

    /* Switch to single-rank, no-DMA mode for this one-shot read */
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE); /* 1 rank */
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_NONE);

    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, channel);
    LL_ADC_SetChannelSamplingTime(ADC1, channel, LL_ADC_SAMPLINGTIME_12CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, channel, LL_ADC_SINGLE_ENDED);

    /* Clear EOC flag and start conversion */
    LL_ADC_ClearFlag_EOC(ADC1);
    LL_ADC_REG_StartConversion(ADC1);

    /* Wait for end of conversion (~1 µs at 170 MHz / DIV4) */
    while (!LL_ADC_IsActiveFlag_EOC(ADC1)) {}

    uint16_t result = LL_ADC_REG_ReadConversionData12(ADC1);

#ifdef USE_ADC2_VOLTAGE_SENSE
    /* ADC1 only carries the internal-temperature regular rank on this target. */
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
#else
    /* Restore DMA-scan sequence: 2 ranks, DMA_LIMITED */
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS);
#endif
    LL_ADC_REG_SetDMATransfer(ADC1, LL_ADC_REG_DMA_TRANSFER_LIMITED);
    /* Restore rank-1 to temperature sensor */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR_ADC1,
        ADC_INTERNAL_TEMP_SAMPLING_TIME);
#ifndef USE_ADC2_VOLTAGE_SENSE
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_2,
        VOLTAGE_ADC_CHANNEL);
    LL_ADC_SetChannelSamplingTime(ADC1, VOLTAGE_ADC_CHANNEL,
        LL_ADC_SAMPLINGTIME_47CYCLES_5);
#endif

    /* 重新触发 DMA 扫描转换（DMA_TRANSFER_LIMITED 模式下每次序列结束后 DMA 停止，
     * 必须重新 StartConversion 才能继续向 ADCDataDMA 更新温度/电压数据） */
    LL_ADC_REG_StartConversion(ADC1);

    return result;
}
#endif /* USE_ADC_ZCD */
