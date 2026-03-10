Here I'm starting to collect and evolve various useful utils related to embedded stuff like RPi or STM32 bare metal development.

I will be extracting it from my existing projects and adding information as modules are added.

## Mock

### STM32

This should allow running tests and apps in the linux container without the need to have the actual STM32 hardware. I needed to make applications for STM32F103C8T6 so the implementation may be a little biased. But it is always good to make things better.

This is simply logical simulator, so no real hardware emulation like QEMU should be expected, but it is very light and should allow easy test application logics. Not sure about codes based on HAL, it may use more structures than I've mocked, and it definitely hides some details without any opportunity to extend them to add testing hooks. Will need to check if HAL can allow injection of custom mocks for peripherals.

I tried to simulate the hardware behaviour and sequences as much as possible closer to the real hardware according to my needs and only for those buses that I've used in my projects, probably something is missing, or may be you find the whole thing should be done another way, feel free to open an issue or create a PR.

`Dockerfile` is attached, it has all required dependecies for STM32 for current implementation. If you run `make stm32-mock` right on your Mac host, most likely it will fail due to specifics with clang and CMSIS, so please run `make TARGET=stm32-mock in-docker` instead. This will create a static library `build/stm32/mock/libstm32-mock.a` and also `include` directory in the `build/include/stm32/mock` directory that you can link in your tests.

`make clean-stm32-mock` will remove all build artifacts.

I added a simple test in `tests/stm32/mock/timer.cpp` for the timer peripheral to demonstrate how the mock works, you can run it with `make TARGET=test-stm32-mock in-docker` and `make clean-test-stm32-mock` will remove all test build artifacts.

Will add more test demos as I go.

If adding this feature to your project, you will probably need to replace all includes of `stm32f1xx.h` with conditional includes (`stm32f1xx.h` for real hardware and `stm32f1xx_mock.hpp` for mock) because `stm32f1xx_mock.hpp` is based on `stm32f1xx.h` and redefines pointers and functions to be mock instances and need to avoid conflicts. Also the directory `build/include/stm32/mock` (or where you copy it) should be added to the include path in your tests because the compiler will be looking for `arm_acle.h` required by CMSIS.

My workflows are mainly based on WFI and interrupts for power efficiency, so the code like this should work just fine:

```cpp
static volatile bool active = true;

while (active) {
    __WFI();
    // do stuff
}
```

If you need to check some code without interrupts, then you should call `cycle()` function yourself to simulate the hardware cycles and trigger the events. For example:

```cpp
static volatile bool active = true;

while (active) {
    Embys::STM32::Mock::cycle();
    // do stuff
}
```

If you have defined an interrupt handler, then you need to inform the mock about it:

```cpp
// your interrupt handler
void TIM2_IRQHandler() {
    // do stuff
}

// somewhere in the initialization code
Embys::STM32::Mock::TIM2_IRQ_Handler_ptr = TIM2_IRQHandler;
```

Same for: SysTick, EXTI, USART, I2C, SPI, PendSV

Currently supported:

- Interrupts for TIM, SysTick, EXTI, USART, I2C, SPI, PendSV
- GPIO with triggering EXTI interrupts
- UART sequences for transmitting and receiving data (if your implementation satisfies the requirements listed in `src/stm32/mock/mock/uart/runtime.hpp`)
- I2C sequences for transmitting and receiving data (if your implementation satisfies the requirements listed in `src/stm32/mock/mock/i2c/runtime.hpp`)
