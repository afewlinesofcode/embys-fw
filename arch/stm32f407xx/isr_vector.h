#include <stdint.h>

extern uint32_t _estack;

void
Reset_Handler(void);

void
Default_Handler(void);

#define WEAK __attribute__((weak))
#define ALIAS(fn) __attribute__((weak, alias(#fn)))

// Cortex-M4 core exceptions
void WEAK
NMI_Handler(void) ALIAS(Default_Handler);
void WEAK
HardFault_Handler(void) ALIAS(Default_Handler);
void WEAK
MemManage_Handler(void) ALIAS(Default_Handler);
void WEAK
BusFault_Handler(void) ALIAS(Default_Handler);
void WEAK
UsageFault_Handler(void) ALIAS(Default_Handler);
void WEAK
SVC_Handler(void) ALIAS(Default_Handler);
void WEAK
DebugMon_Handler(void) ALIAS(Default_Handler);
void WEAK
PendSV_Handler(void) ALIAS(Default_Handler);
void WEAK
SysTick_Handler(void) ALIAS(Default_Handler);

// External interrupts (F407 — IRQ0..81)
void WEAK
WWDG_IRQHandler(void) ALIAS(Default_Handler); // 0
void WEAK
PVD_IRQHandler(void) ALIAS(Default_Handler); // 1
void WEAK
TAMP_STAMP_IRQHandler(void) ALIAS(Default_Handler); // 2
void WEAK
RTC_WKUP_IRQHandler(void) ALIAS(Default_Handler); // 3
void WEAK
FLASH_IRQHandler(void) ALIAS(Default_Handler); // 4
void WEAK
RCC_IRQHandler(void) ALIAS(Default_Handler); // 5
void WEAK
EXTI0_IRQHandler(void) ALIAS(Default_Handler); // 6
void WEAK
EXTI1_IRQHandler(void) ALIAS(Default_Handler); // 7
void WEAK
EXTI2_IRQHandler(void) ALIAS(Default_Handler); // 8
void WEAK
EXTI3_IRQHandler(void) ALIAS(Default_Handler); // 9
void WEAK
EXTI4_IRQHandler(void) ALIAS(Default_Handler); // 10
void WEAK
DMA1_Stream0_IRQHandler(void) ALIAS(Default_Handler); // 11
void WEAK
DMA1_Stream1_IRQHandler(void) ALIAS(Default_Handler); // 12
void WEAK
DMA1_Stream2_IRQHandler(void) ALIAS(Default_Handler); // 13
void WEAK
DMA1_Stream3_IRQHandler(void) ALIAS(Default_Handler); // 14
void WEAK
DMA1_Stream4_IRQHandler(void) ALIAS(Default_Handler); // 15
void WEAK
DMA1_Stream5_IRQHandler(void) ALIAS(Default_Handler); // 16
void WEAK
DMA1_Stream6_IRQHandler(void) ALIAS(Default_Handler); // 17
void WEAK
ADC_IRQHandler(void) ALIAS(Default_Handler); // 18
void WEAK
CAN1_TX_IRQHandler(void) ALIAS(Default_Handler); // 19
void WEAK
CAN1_RX0_IRQHandler(void) ALIAS(Default_Handler); // 20
void WEAK
CAN1_RX1_IRQHandler(void) ALIAS(Default_Handler); // 21
void WEAK
CAN1_SCE_IRQHandler(void) ALIAS(Default_Handler); // 22
void WEAK
EXTI9_5_IRQHandler(void) ALIAS(Default_Handler); // 23
void WEAK
TIM1_BRK_TIM9_IRQHandler(void) ALIAS(Default_Handler); // 24
void WEAK
TIM1_UP_TIM10_IRQHandler(void) ALIAS(Default_Handler); // 25
void WEAK
TIM1_TRG_COM_TIM11_IRQHandler(void) ALIAS(Default_Handler); // 26
void WEAK
TIM1_CC_IRQHandler(void) ALIAS(Default_Handler); // 27
void WEAK
TIM2_IRQHandler(void) ALIAS(Default_Handler); // 28
void WEAK
TIM3_IRQHandler(void) ALIAS(Default_Handler); // 29
void WEAK
TIM4_IRQHandler(void) ALIAS(Default_Handler); // 30
void WEAK
I2C1_EV_IRQHandler(void) ALIAS(Default_Handler); // 31
void WEAK
I2C1_ER_IRQHandler(void) ALIAS(Default_Handler); // 32
void WEAK
I2C2_EV_IRQHandler(void) ALIAS(Default_Handler); // 33
void WEAK
I2C2_ER_IRQHandler(void) ALIAS(Default_Handler); // 34
void WEAK
SPI1_IRQHandler(void) ALIAS(Default_Handler); // 35
void WEAK
SPI2_IRQHandler(void) ALIAS(Default_Handler); // 36
void WEAK
USART1_IRQHandler(void) ALIAS(Default_Handler); // 37
void WEAK
USART2_IRQHandler(void) ALIAS(Default_Handler); // 38
void WEAK
USART3_IRQHandler(void) ALIAS(Default_Handler); // 39
void WEAK
EXTI15_10_IRQHandler(void) ALIAS(Default_Handler); // 40
void WEAK
RTC_Alarm_IRQHandler(void) ALIAS(Default_Handler); // 41
void WEAK
OTG_FS_WKUP_IRQHandler(void) ALIAS(Default_Handler); // 42
void WEAK
TIM8_BRK_TIM12_IRQHandler(void) ALIAS(Default_Handler); // 43
void WEAK
TIM8_UP_TIM13_IRQHandler(void) ALIAS(Default_Handler); // 44
void WEAK
TIM8_TRG_COM_TIM14_IRQHandler(void) ALIAS(Default_Handler); // 45
void WEAK
TIM8_CC_IRQHandler(void) ALIAS(Default_Handler); // 46
void WEAK
DMA1_Stream7_IRQHandler(void) ALIAS(Default_Handler); // 47
void WEAK
FSMC_IRQHandler(void) ALIAS(Default_Handler); // 48
void WEAK
SDIO_IRQHandler(void) ALIAS(Default_Handler); // 49
void WEAK
TIM5_IRQHandler(void) ALIAS(Default_Handler); // 50
void WEAK
SPI3_IRQHandler(void) ALIAS(Default_Handler); // 51
void WEAK
UART4_IRQHandler(void) ALIAS(Default_Handler); // 52
void WEAK
UART5_IRQHandler(void) ALIAS(Default_Handler); // 53
void WEAK
TIM6_DAC_IRQHandler(void) ALIAS(Default_Handler); // 54
void WEAK
TIM7_IRQHandler(void) ALIAS(Default_Handler); // 55
void WEAK
DMA2_Stream0_IRQHandler(void) ALIAS(Default_Handler); // 56
void WEAK
DMA2_Stream1_IRQHandler(void) ALIAS(Default_Handler); // 57
void WEAK
DMA2_Stream2_IRQHandler(void) ALIAS(Default_Handler); // 58
void WEAK
DMA2_Stream3_IRQHandler(void) ALIAS(Default_Handler); // 59
void WEAK
DMA2_Stream4_IRQHandler(void) ALIAS(Default_Handler); // 60
void WEAK
ETH_IRQHandler(void) ALIAS(Default_Handler); // 61
void WEAK
ETH_WKUP_IRQHandler(void) ALIAS(Default_Handler); // 62
void WEAK
CAN2_TX_IRQHandler(void) ALIAS(Default_Handler); // 63
void WEAK
CAN2_RX0_IRQHandler(void) ALIAS(Default_Handler); // 64
void WEAK
CAN2_RX1_IRQHandler(void) ALIAS(Default_Handler); // 65
void WEAK
CAN2_SCE_IRQHandler(void) ALIAS(Default_Handler); // 66
void WEAK
OTG_FS_IRQHandler(void) ALIAS(Default_Handler); // 67
void WEAK
DMA2_Stream5_IRQHandler(void) ALIAS(Default_Handler); // 68
void WEAK
DMA2_Stream6_IRQHandler(void) ALIAS(Default_Handler); // 69
void WEAK
DMA2_Stream7_IRQHandler(void) ALIAS(Default_Handler); // 70
void WEAK
USART6_IRQHandler(void) ALIAS(Default_Handler); // 71
void WEAK
I2C3_EV_IRQHandler(void) ALIAS(Default_Handler); // 72
void WEAK
I2C3_ER_IRQHandler(void) ALIAS(Default_Handler); // 73
void WEAK
OTG_HS_EP1_OUT_IRQHandler(void) ALIAS(Default_Handler); // 74
void WEAK
OTG_HS_EP1_IN_IRQHandler(void) ALIAS(Default_Handler); // 75
void WEAK
OTG_HS_WKUP_IRQHandler(void) ALIAS(Default_Handler); // 76
void WEAK
OTG_HS_IRQHandler(void) ALIAS(Default_Handler); // 77
void WEAK
DCMI_IRQHandler(void) ALIAS(Default_Handler); // 78
void WEAK
RNG_IRQHandler(void) ALIAS(Default_Handler); // 80
void WEAK
FPU_IRQHandler(void) ALIAS(Default_Handler); // 81
