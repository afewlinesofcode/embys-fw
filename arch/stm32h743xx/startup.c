#include "isr_vector.h"
#include "clock_init.h"
#include "panic.h"

// Declared in system_stm32h7xx.c — not in any header.
void ExitRun0Mode(void);

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int
main(void);

void
Reset_Handler(void)
{
  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;
  while (dst < &_edata)
    *dst++ = *src++;

  dst = &_sbss;
  while (dst < &_ebss)
    *dst++ = 0;

  ExitRun0Mode();
  SystemInit();
  clock_init_480mhz();
  main();
  while (1) {}
}

void
Default_Handler(void)
{
  panic(-1);
}

__attribute__((section(".isr_vector"), used))
const void *g_pfnVectors[] = {
  [0]   = (void *)&_estack,
  [1]   = Reset_Handler,
  [2]   = NMI_Handler,
  [3]   = HardFault_Handler,
  [4]   = MemManage_Handler,
  [5]   = BusFault_Handler,
  [6]   = UsageFault_Handler,
  [11]  = SVC_Handler,
  [12]  = DebugMon_Handler,
  [14]  = PendSV_Handler,
  [15]  = SysTick_Handler,
  [16]  = WWDG_IRQHandler,
  [17]  = PVD_AVD_IRQHandler,
  [18]  = TAMP_STAMP_IRQHandler,
  [19]  = RTC_WKUP_IRQHandler,
  [20]  = FLASH_IRQHandler,
  [21]  = RCC_IRQHandler,
  [22]  = EXTI0_IRQHandler,
  [23]  = EXTI1_IRQHandler,
  [24]  = EXTI2_IRQHandler,
  [25]  = EXTI3_IRQHandler,
  [26]  = EXTI4_IRQHandler,
  [27]  = DMA1_Stream0_IRQHandler,
  [28]  = DMA1_Stream1_IRQHandler,
  [29]  = DMA1_Stream2_IRQHandler,
  [30]  = DMA1_Stream3_IRQHandler,
  [31]  = DMA1_Stream4_IRQHandler,
  [32]  = DMA1_Stream5_IRQHandler,
  [33]  = DMA1_Stream6_IRQHandler,
  [34]  = ADC_IRQHandler,
  [35]  = FDCAN1_IT0_IRQHandler,
  [36]  = FDCAN2_IT0_IRQHandler,
  [37]  = FDCAN1_IT1_IRQHandler,
  [38]  = FDCAN2_IT1_IRQHandler,
  [39]  = EXTI9_5_IRQHandler,
  [40]  = TIM1_BRK_IRQHandler,
  [41]  = TIM1_UP_IRQHandler,
  [42]  = TIM1_TRG_COM_IRQHandler,
  [43]  = TIM1_CC_IRQHandler,
  [44]  = TIM2_IRQHandler,
  [45]  = TIM3_IRQHandler,
  [46]  = TIM4_IRQHandler,
  [47]  = I2C1_EV_IRQHandler,
  [48]  = I2C1_ER_IRQHandler,
  [49]  = I2C2_EV_IRQHandler,
  [50]  = I2C2_ER_IRQHandler,
  [51]  = SPI1_IRQHandler,
  [52]  = SPI2_IRQHandler,
  [53]  = USART1_IRQHandler,
  [54]  = USART2_IRQHandler,
  [55]  = USART3_IRQHandler,
  [56]  = EXTI15_10_IRQHandler,
  [57]  = RTC_Alarm_IRQHandler,
  [59]  = TIM8_BRK_TIM12_IRQHandler,
  [60]  = TIM8_UP_TIM13_IRQHandler,
  [61]  = TIM8_TRG_COM_TIM14_IRQHandler,
  [62]  = TIM8_CC_IRQHandler,
  [63]  = DMA1_Stream7_IRQHandler,
  [64]  = FMC_IRQHandler,
  [65]  = SDMMC1_IRQHandler,
  [66]  = TIM5_IRQHandler,
  [67]  = SPI3_IRQHandler,
  [68]  = UART4_IRQHandler,
  [69]  = UART5_IRQHandler,
  [70]  = TIM6_DAC_IRQHandler,
  [71]  = TIM7_IRQHandler,
  [72]  = DMA2_Stream0_IRQHandler,
  [73]  = DMA2_Stream1_IRQHandler,
  [74]  = DMA2_Stream2_IRQHandler,
  [75]  = DMA2_Stream3_IRQHandler,
  [76]  = DMA2_Stream4_IRQHandler,
  [77]  = ETH_IRQHandler,
  [78]  = ETH_WKUP_IRQHandler,
  [79]  = FDCAN_CAL_IRQHandler,
  [84]  = DMA2_Stream5_IRQHandler,
  [85]  = DMA2_Stream6_IRQHandler,
  [86]  = DMA2_Stream7_IRQHandler,
  [87]  = USART6_IRQHandler,
  [88]  = I2C3_EV_IRQHandler,
  [89]  = I2C3_ER_IRQHandler,
  [90]  = OTG_HS_EP1_OUT_IRQHandler,
  [91]  = OTG_HS_EP1_IN_IRQHandler,
  [92]  = OTG_HS_WKUP_IRQHandler,
  [93]  = OTG_HS_IRQHandler,
  [94]  = DCMI_PSSI_IRQHandler,
  [96]  = RNG_IRQHandler,
  [97]  = FPU_IRQHandler,
  [98]  = UART7_IRQHandler,
  [99]  = UART8_IRQHandler,
  [100] = SPI4_IRQHandler,
  [101] = SPI5_IRQHandler,
  [102] = SPI6_IRQHandler,
  [103] = SAI1_IRQHandler,
  [104] = LTDC_IRQHandler,
  [105] = LTDC_ER_IRQHandler,
  [106] = DMA2D_IRQHandler,
  [107] = SAI2_IRQHandler,
  [108] = QUADSPI_IRQHandler,
  [109] = LPTIM1_IRQHandler,
  [110] = CEC_IRQHandler,
  [111] = I2C4_EV_IRQHandler,
  [112] = I2C4_ER_IRQHandler,
  [113] = SPDIF_RX_IRQHandler,
  [114] = OTG_FS_IRQHandler,
  [115] = DMAMUX1_OVR_IRQHandler,
  [116] = HRTIM1_Master_IRQHandler,
  [117] = HRTIM1_TIMA_IRQHandler,
  [118] = HRTIM1_TIMB_IRQHandler,
  [119] = HRTIM1_TIMC_IRQHandler,
  [120] = HRTIM1_TIMD_IRQHandler,
  [121] = HRTIM1_TIME_IRQHandler,
  [122] = HRTIM1_FLT_IRQHandler,
  [123] = DFSDM1_FLT0_IRQHandler,
  [124] = DFSDM1_FLT1_IRQHandler,
  [125] = DFSDM1_FLT2_IRQHandler,
  [126] = DFSDM1_FLT3_IRQHandler,
  [127] = SAI3_IRQHandler,
  [128] = SWPMI1_IRQHandler,
  [129] = TIM15_IRQHandler,
  [130] = TIM16_IRQHandler,
  [131] = TIM17_IRQHandler,
  [132] = MDIOS_WKUP_IRQHandler,
  [133] = MDIOS_IRQHandler,
  [134] = JPEG_IRQHandler,
  [135] = MDMA_IRQHandler,
  [137] = SDMMC2_IRQHandler,
  [138] = HSEM1_IRQHandler,
  [140] = ADC3_IRQHandler,
  [141] = DMAMUX2_OVR_IRQHandler,
  [142] = BDMA_Channel0_IRQHandler,
  [143] = BDMA_Channel1_IRQHandler,
  [144] = BDMA_Channel2_IRQHandler,
  [145] = BDMA_Channel3_IRQHandler,
  [146] = BDMA_Channel4_IRQHandler,
  [147] = BDMA_Channel5_IRQHandler,
  [148] = BDMA_Channel6_IRQHandler,
  [149] = BDMA_Channel7_IRQHandler,
  [150] = COMP1_IRQHandler,
  [151] = LPTIM2_IRQHandler,
  [152] = LPTIM3_IRQHandler,
  [153] = LPTIM4_IRQHandler,
  [154] = LPTIM5_IRQHandler,
  [155] = LPUART1_IRQHandler,
  [157] = CRS_IRQHandler,
  [160] = SAI4_IRQHandler,
  [164] = WAKEUP_PIN_IRQHandler,
};
