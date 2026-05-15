#include <stm32f4xx.h>
#include "isr_vector.h"
#include "clock_init.h"
#include "panic.h"

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern void (*__preinit_array_start [])(void) __attribute__((weak));
extern void (*__preinit_array_end [])(void) __attribute__((weak));
extern void (*__init_array_start [])(void) __attribute__((weak));
extern void (*__init_array_end [])(void) __attribute__((weak));
extern void (*__fini_array_start [])(void) __attribute__((weak));
extern void (*__fini_array_end [])(void) __attribute__((weak));

extern int
main(void);

void
__libc_fini_array(void)
{
  void (**p)(void);
  for (p = __fini_array_end - 1; p >= __fini_array_start; p--)
    (*p)();
}

void
__cxa_pure_virtual(void)
{
  for (;;)
    __asm volatile("bkpt #0");
}

void
Reset_Handler(void)
{
  // Delay for debugger attach and power stabilisation
  for (volatile int i = 0; i < 1000000; i++) {}

  // Copy .data from flash to SRAM
  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;
  while (dst < &_edata)
    *dst++ = *src++;

  // Zero .bss
  dst = &_sbss;
  while (dst < &_ebss)
    *dst++ = 0;

  clock_init_168mhz();
  SystemInit();
  SystemCoreClockUpdate();

  // Call global constructors
  void (**p)(void);
  for (p = __preinit_array_start; p < __preinit_array_end; p++)
    (*p)();
  for (p = __init_array_start; p < __init_array_end; p++)
    (*p)();

  panic(main());

  while (1)
    __asm volatile("wfi");
}

void
Default_Handler(void)
{
  panic(-1);
}

// Vector table — Cortex-M4 (F407, 82 external interrupts)
__attribute__((section(".isr_vector"), used))
const void *g_pfnVectors[] = {
  [0]  = (void *)&_estack,
  [1]  = Reset_Handler,
  [2]  = NMI_Handler,
  [3]  = HardFault_Handler,
  [4]  = MemManage_Handler,
  [5]  = BusFault_Handler,
  [6]  = UsageFault_Handler,
  [11] = SVC_Handler,
  [12] = DebugMon_Handler,
  [14] = PendSV_Handler,
  [15] = SysTick_Handler,
  [16] = WWDG_IRQHandler,
  [17] = PVD_IRQHandler,
  [18] = TAMP_STAMP_IRQHandler,
  [19] = RTC_WKUP_IRQHandler,
  [20] = FLASH_IRQHandler,
  [21] = RCC_IRQHandler,
  [22] = EXTI0_IRQHandler,
  [23] = EXTI1_IRQHandler,
  [24] = EXTI2_IRQHandler,
  [25] = EXTI3_IRQHandler,
  [26] = EXTI4_IRQHandler,
  [27] = DMA1_Stream0_IRQHandler,
  [28] = DMA1_Stream1_IRQHandler,
  [29] = DMA1_Stream2_IRQHandler,
  [30] = DMA1_Stream3_IRQHandler,
  [31] = DMA1_Stream4_IRQHandler,
  [32] = DMA1_Stream5_IRQHandler,
  [33] = DMA1_Stream6_IRQHandler,
  [34] = ADC_IRQHandler,
  [35] = CAN1_TX_IRQHandler,
  [36] = CAN1_RX0_IRQHandler,
  [37] = CAN1_RX1_IRQHandler,
  [38] = CAN1_SCE_IRQHandler,
  [39] = EXTI9_5_IRQHandler,
  [40] = TIM1_BRK_TIM9_IRQHandler,
  [41] = TIM1_UP_TIM10_IRQHandler,
  [42] = TIM1_TRG_COM_TIM11_IRQHandler,
  [43] = TIM1_CC_IRQHandler,
  [44] = TIM2_IRQHandler,
  [45] = TIM3_IRQHandler,
  [46] = TIM4_IRQHandler,
  [47] = I2C1_EV_IRQHandler,
  [48] = I2C1_ER_IRQHandler,
  [49] = I2C2_EV_IRQHandler,
  [50] = I2C2_ER_IRQHandler,
  [51] = SPI1_IRQHandler,
  [52] = SPI2_IRQHandler,
  [53] = USART1_IRQHandler,
  [54] = USART2_IRQHandler,
  [55] = USART3_IRQHandler,
  [56] = EXTI15_10_IRQHandler,
  [57] = RTC_Alarm_IRQHandler,
  [58] = OTG_FS_WKUP_IRQHandler,
  [59] = TIM8_BRK_TIM12_IRQHandler,
  [60] = TIM8_UP_TIM13_IRQHandler,
  [61] = TIM8_TRG_COM_TIM14_IRQHandler,
  [62] = TIM8_CC_IRQHandler,
  [63] = DMA1_Stream7_IRQHandler,
  [64] = FSMC_IRQHandler,
  [65] = SDIO_IRQHandler,
  [66] = TIM5_IRQHandler,
  [67] = SPI3_IRQHandler,
  [68] = UART4_IRQHandler,
  [69] = UART5_IRQHandler,
  [70] = TIM6_DAC_IRQHandler,
  [71] = TIM7_IRQHandler,
  [72] = DMA2_Stream0_IRQHandler,
  [73] = DMA2_Stream1_IRQHandler,
  [74] = DMA2_Stream2_IRQHandler,
  [75] = DMA2_Stream3_IRQHandler,
  [76] = DMA2_Stream4_IRQHandler,
  [77] = ETH_IRQHandler,
  [78] = ETH_WKUP_IRQHandler,
  [79] = CAN2_TX_IRQHandler,
  [80] = CAN2_RX0_IRQHandler,
  [81] = CAN2_RX1_IRQHandler,
  [82] = CAN2_SCE_IRQHandler,
  [83] = OTG_FS_IRQHandler,
  [84] = DMA2_Stream5_IRQHandler,
  [85] = DMA2_Stream6_IRQHandler,
  [86] = DMA2_Stream7_IRQHandler,
  [87] = USART6_IRQHandler,
  [88] = I2C3_EV_IRQHandler,
  [89] = I2C3_ER_IRQHandler,
  [90] = OTG_HS_EP1_OUT_IRQHandler,
  [91] = OTG_HS_EP1_IN_IRQHandler,
  [92] = OTG_HS_WKUP_IRQHandler,
  [93] = OTG_HS_IRQHandler,
  [94] = DCMI_IRQHandler,
  [96] = RNG_IRQHandler,
  [97] = FPU_IRQHandler,
};
