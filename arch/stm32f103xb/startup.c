#include "isr_vector.h"
#include "clock_init.h"

// Now define the vector table in .isr_vector
__attribute__((section(".isr_vector"), used))
const void *g_pfnVectors[] = {
    // --- Cortex-M3 core exceptions ---------------------------------
    [0]  = (void *)&_estack,        // 0  Initial Main Stack Pointer
    [1]  = Reset_Handler,           // 1  Reset
    [2]  = NMI_Handler,             // 2  NMI
    [3]  = HardFault_Handler,       // 3  HardFault
    [4]  = MemManage_Handler,       // 4  MemManage
    [5]  = BusFault_Handler,        // 5  BusFault
    [6]  = UsageFault_Handler,      // 6  UsageFault
    [7]  = 0,                       // 7  Reserved
    [8]  = 0,                       // 8  Reserved
    [9]  = 0,                       // 9  Reserved
    [10] = 0,                       // 10 Reserved
    [11] = SVC_Handler,             // 11 SVCall
    [12] = DebugMon_Handler,        // 12 Debug monitor
    [13] = 0,                       // 13 Reserved
    [14] = PendSV_Handler,          // 14 PendSV
    [15] = SysTick_Handler,         // 15 SysTick

    // --- External interrupts (IRQ0..59) -----------------------------
    [16] = WWDG_IRQHandler,         // 16 IRQ0  WWDG
    [17] = PVD_IRQHandler,          // 17 IRQ1  PVD
    [18] = TAMPER_IRQHandler,       // 18 IRQ2  TAMPER
    [19] = RTC_IRQHandler,          // 19 IRQ3  RTC
    [20] = FLASH_IRQHandler,        // 20 IRQ4  FLASH
    [21] = RCC_IRQHandler,          // 21 IRQ5  RCC
    [22] = EXTI0_IRQHandler,        // 22 IRQ6  EXTI Line 0
    [23] = EXTI1_IRQHandler,        // 23 IRQ7  EXTI Line 1
    [24] = EXTI2_IRQHandler,        // 24 IRQ8  EXTI Line 2
    [25] = EXTI3_IRQHandler,        // 25 IRQ9  EXTI Line 3
    [26] = EXTI4_IRQHandler,        // 26 IRQ10 EXTI Line 4
    [27] = DMA1_Channel1_IRQHandler,// 27 IRQ11 DMA1 Channel 1
    [28] = DMA1_Channel2_IRQHandler,// 28 IRQ12 DMA1 Channel 2
    [29] = DMA1_Channel3_IRQHandler,// 29 IRQ13 DMA1 Channel 3
    [30] = DMA1_Channel4_IRQHandler,// 30 IRQ14 DMA1 Channel 4
    [31] = DMA1_Channel5_IRQHandler,// 31 IRQ15 DMA1 Channel 5
    [32] = DMA1_Channel6_IRQHandler,// 32 IRQ16 DMA1 Channel 6
    [33] = DMA1_Channel7_IRQHandler,// 33 IRQ17 DMA1 Channel 7
    [34] = ADC1_2_IRQHandler,       // 34 IRQ18 ADC1 & ADC2
    [35] = USB_HP_CAN_TX_IRQHandler,// 35 IRQ19 USB HP / CAN TX
    [36] = USB_LP_CAN_RX0_IRQHandler,//36 IRQ20 USB LP / CAN RX0
    [37] = CAN_RX1_IRQHandler,      // 37 IRQ21 CAN RX1
    [38] = CAN_SCE_IRQHandler,      // 38 IRQ22 CAN SCE
    [39] = EXTI9_5_IRQHandler,      // 39 IRQ23 EXTI Line 9..5
    [40] = TIM1_BRK_IRQHandler,     // 40 IRQ24 TIM1 Break
    [41] = TIM1_UP_IRQHandler,      // 41 IRQ25 TIM1 Update
    [42] = TIM1_TRG_COM_IRQHandler, // 42 IRQ26 TIM1 Trigger/Com
    [43] = TIM1_CC_IRQHandler,      // 43 IRQ27 TIM1 Capture/Compare
    [44] = TIM2_IRQHandler,         // 44 IRQ28 TIM2
    [45] = TIM3_IRQHandler,         // 45 IRQ29 TIM3
    [46] = TIM4_IRQHandler,         // 46 IRQ30 TIM4
    [47] = I2C1_EV_IRQHandler,      // 47 IRQ31 I2C1 Event
    [48] = I2C1_ER_IRQHandler,      // 48 IRQ32 I2C1 Error
    [49] = I2C2_EV_IRQHandler,      // 49 IRQ33 I2C2 Event
    [50] = I2C2_ER_IRQHandler,      // 50 IRQ34 I2C2 Error
    [51] = SPI1_IRQHandler,         // 51 IRQ35 SPI1
    [52] = SPI2_IRQHandler,         // 52 IRQ36 SPI2
    [53] = USART1_IRQHandler,       // 53 IRQ37 USART1
    [54] = USART2_IRQHandler,       // 54 IRQ38 USART2
    [55] = USART3_IRQHandler,       // 55 IRQ39 USART3
    [56] = EXTI15_10_IRQHandler,    // 56 IRQ40 EXTI Line 15..10
    [57] = RTCAlarm_IRQHandler,     // 57 IRQ41 RTC Alarm
    [58] = USBWakeUp_IRQHandler,    // 58 IRQ42 USB Wakeup

    // high-density / XL only
    [59] = TIM8_BRK_IRQHandler,     // 59 IRQ43 TIM8 Break
    [60] = TIM8_UP_IRQHandler,      // 60 IRQ44 TIM8 Update
    [61] = TIM8_TRG_COM_IRQHandler, // 61 IRQ45 TIM8 Trigger/Com
    [62] = TIM8_CC_IRQHandler,      // 62 IRQ46 TIM8 Capture/Compare
    [63] = ADC3_IRQHandler,         // 63 IRQ47 ADC3
    [64] = FSMC_IRQHandler,         // 64 IRQ48 FSMC
    [65] = SDIO_IRQHandler,         // 65 IRQ49 SDIO
    [66] = TIM5_IRQHandler,         // 66 IRQ50 TIM5
    [67] = SPI3_IRQHandler,         // 67 IRQ51 SPI3
    [68] = UART4_IRQHandler,        // 68 IRQ52 UART4
    [69] = UART5_IRQHandler,        // 69 IRQ53 UART5
    [70] = TIM6_IRQHandler,         // 70 IRQ54 TIM6
    [71] = TIM7_IRQHandler,         // 71 IRQ55 TIM7
    [72] = DMA2_Channel1_IRQHandler,// 72 IRQ56 DMA2 Channel 1
    [73] = DMA2_Channel2_IRQHandler,// 73 IRQ57 DMA2 Channel 2
    [74] = DMA2_Channel3_IRQHandler,// 74 IRQ58 DMA2 Channel 3
    [75] = DMA2_Channel4_5_IRQHandler // 75 IRQ59 DMA2 Channel 4 & 5
};

extern char _sidata[], _sdata[], _edata[], _sbss[], _ebss[];
extern void (*__preinit_array_start [])(void) __attribute__((weak));
extern void (*__preinit_array_end [])(void) __attribute__((weak));
extern void (*__init_array_start [])(void) __attribute__((weak));
extern void (*__init_array_end [])(void) __attribute__((weak));
extern void (*__fini_array_start [])(void) __attribute__((weak));
extern void (*__fini_array_end [])(void) __attribute__((weak));

extern int main(void);
void SystemInit(void);
void SystemCoreClockUpdate(void);

// Function to call global destructors (for virtual destructors)
void __libc_fini_array(void) {
  void (**p)(void);
  for (p = __fini_array_end - 1; p >= __fini_array_start; p--) (*p)();
}

// Required for C++ exception handling (even if not used)
void __cxa_pure_virtual(void) {
  // Pure virtual function called - this should never happen
  // in a well-designed embedded system
  for (;;) __asm volatile("bkpt #0"); // Breakpoint for debugging
}

void Default_Handler(void)
{
    while (1) {
        __asm volatile ("nop");
    }
}

void Reset_Handler(void) {
  // copy .data
  char* src=_sidata, *dst=_sdata;
  while (dst < _edata) *dst++ = *src++;
  // zero .bss
  for (dst=_sbss; dst < _ebss; ) *dst++ = 0;

  // Default: HSE-based 72 MHz. If your board has no HSE crystal/oscillator,
  // build with -DCLOCK_INIT_NO_HSE to use the internal HSI (64 MHz max).
#ifdef CLOCK_INIT_NO_HSE
  clock_init_64mhz_hsi();
#else
  clock_init_72mhz();
#endif
  SystemInit();             // CMSIS system initialization
  SystemCoreClockUpdate();  // Update SystemCoreClock variable

  // call global constructors
  void (**p)(void);
  for (p = __preinit_array_start; p < __preinit_array_end; p++) (*p)();
  for (p = __init_array_start; p < __init_array_end; p++) (*p)();

  (void)main();

  for (;;) __asm volatile("wfi");
}
