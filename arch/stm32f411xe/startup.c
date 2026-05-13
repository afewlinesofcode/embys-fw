#include "isr_vector.h"
#include "clock_init.h"
#include "panic.h"

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
  // Copy .data from flash to SRAM
  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;
  while (dst < &_edata)
    *dst++ = *src++;

  // Zero .bss
  dst = &_sbss;
  while (dst < &_ebss)
    *dst++ = 0;

  SystemInit();
  clock_init_100mhz();
  main();
  while (1) {}
}

void
Default_Handler(void)
{
  panic(-1);
}

// Vector table — Cortex-M4 (F411, 86 external interrupts, IRQ0..85)
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
  [16] = WWDG_IRQHandler,        // IRQ0
  [17] = PVD_IRQHandler,         // IRQ1
  [18] = TAMP_STAMP_IRQHandler,  // IRQ2
  [19] = RTC_WKUP_IRQHandler,    // IRQ3
  [20] = FLASH_IRQHandler,       // IRQ4
  [21] = RCC_IRQHandler,         // IRQ5
  [22] = EXTI0_IRQHandler,       // IRQ6
  [23] = EXTI1_IRQHandler,       // IRQ7
  [24] = EXTI2_IRQHandler,       // IRQ8
  [25] = EXTI3_IRQHandler,       // IRQ9
  [26] = EXTI4_IRQHandler,       // IRQ10
  [27] = DMA1_Stream0_IRQHandler, // IRQ11
  [28] = DMA1_Stream1_IRQHandler, // IRQ12
  [29] = DMA1_Stream2_IRQHandler, // IRQ13
  [30] = DMA1_Stream3_IRQHandler, // IRQ14
  [31] = DMA1_Stream4_IRQHandler, // IRQ15
  [32] = DMA1_Stream5_IRQHandler, // IRQ16
  [33] = DMA1_Stream6_IRQHandler, // IRQ17
  [34] = ADC_IRQHandler,          // IRQ18
  // [35..38] reserved (IRQ19..22, no CAN)
  [39] = EXTI9_5_IRQHandler,           // IRQ23
  [40] = TIM1_BRK_TIM9_IRQHandler,     // IRQ24
  [41] = TIM1_UP_TIM10_IRQHandler,     // IRQ25
  [42] = TIM1_TRG_COM_TIM11_IRQHandler, // IRQ26
  [43] = TIM1_CC_IRQHandler,           // IRQ27
  [44] = TIM2_IRQHandler,              // IRQ28
  [45] = TIM3_IRQHandler,              // IRQ29
  [46] = TIM4_IRQHandler,              // IRQ30
  [47] = I2C1_EV_IRQHandler,           // IRQ31
  [48] = I2C1_ER_IRQHandler,           // IRQ32
  [49] = I2C2_EV_IRQHandler,           // IRQ33
  [50] = I2C2_ER_IRQHandler,           // IRQ34
  [51] = SPI1_IRQHandler,              // IRQ35
  [52] = SPI2_IRQHandler,              // IRQ36
  [53] = USART1_IRQHandler,            // IRQ37
  [54] = USART2_IRQHandler,            // IRQ38
  // [55] reserved (IRQ39, no USART3)
  [56] = EXTI15_10_IRQHandler,         // IRQ40
  [57] = RTC_Alarm_IRQHandler,         // IRQ41
  [58] = OTG_FS_WKUP_IRQHandler,       // IRQ42
  // [59..62] reserved (IRQ43..46, no TIM8)
  [63] = DMA1_Stream7_IRQHandler,      // IRQ47
  // [64] reserved (IRQ48, no FSMC)
  [65] = SDIO_IRQHandler,              // IRQ49
  [66] = TIM5_IRQHandler,              // IRQ50
  [67] = SPI3_IRQHandler,              // IRQ51
  // [68..71] reserved (IRQ52..55, no UART4/5, TIM6_DAC, TIM7)
  [72] = DMA2_Stream0_IRQHandler,      // IRQ56
  [73] = DMA2_Stream1_IRQHandler,      // IRQ57
  [74] = DMA2_Stream2_IRQHandler,      // IRQ58
  [75] = DMA2_Stream3_IRQHandler,      // IRQ59
  [76] = DMA2_Stream4_IRQHandler,      // IRQ60
  // [77..82] reserved (IRQ61..66, no ETH, CAN2)
  [83] = OTG_FS_IRQHandler,            // IRQ67
  [84] = DMA2_Stream5_IRQHandler,      // IRQ68
  [85] = DMA2_Stream6_IRQHandler,      // IRQ69
  [86] = DMA2_Stream7_IRQHandler,      // IRQ70
  [87] = USART6_IRQHandler,            // IRQ71
  [88] = I2C3_EV_IRQHandler,           // IRQ72
  [89] = I2C3_ER_IRQHandler,           // IRQ73
  // [90..96] reserved (IRQ74..80, no OTG_HS, DCMI, RNG)
  [97] = FPU_IRQHandler,               // IRQ81
  // [98..99] reserved (IRQ82..83)
  [100] = SPI4_IRQHandler,             // IRQ84
  [101] = SPI5_IRQHandler,             // IRQ85
};
