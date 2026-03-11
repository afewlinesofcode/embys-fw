Here I'm collecting and evolving various useful utilities related to embedded stuff like RPi or STM32 bare metal development.

I'll be extracting them from my existing projects and adding information as modules are added.

## Simulation

### STM32

This allows running tests and apps in the Linux container without needing actual STM32 hardware. I needed to make applications for STM32F103C8T6, so the implementation may be a little biased. But it's always good to make things better.

This is simply a logical simulator, so no real hardware emulation like QEMU should be expected, but it is very lightweight and allows easy testing of application logic. Not sure about code based on HAL - it may use more structures than I've mocked, and adding testing hooks for read/write operations may be not that straightforward. I'll need to check if HAL can allow injection or use of custom mocks for peripherals and hooks.

I tried to simulate the hardware behavior and sequences as closely as possible to the real hardware according to my needs, and only for those buses that I've used in my projects. Probably something is missing, or maybe you think the whole thing should be done another way - feel free to open an issue or create a PR.

The long-term vision of this project is to create a software simulator that behaves almost like a real STM32, covering as much of the microcontroller functionality as possible. Contributions are very welcome.

#### Build

`Dockerfile` is attached; it has all required dependencies for STM32 for the current implementation. If you run `make stm32-sim` directly on your Mac host, it will most likely fail due to specifics with clang and CMSIS (can be improved), so please run `make stm32-sim-in-docker` instead. This will create a static library `build/stm32/sim/libstm32-sim.a` and also `include` directory in the `build/include/stm32/sim` directory that you can link in your tests.

`make clean-stm32-sim` will remove all build artifacts.

#### Test

I added a simple test in `tests/stm32/sim/timer.cpp` for the timer peripheral to demonstrate how the simulator works. You can run it with `make test-stm32-sim-in-docker`, and `make clean-test-stm32-sim` will remove all test build artifacts.

Make sure to run `make stm32-sim-in-docker` first, targets aren't linked.

I'll add more test demos as I go.

#### Link

If adding this feature to your project, you will probably need to replace all includes of `stm32f1xx.h` with conditional includes (`stm32f1xx.h` for real hardware and `stm32f1xx_sim.hpp` for simulation) because `stm32f1xx_sim.hpp` is based on `stm32f1xx.h` and redefines pointers and functions to be simulation instances and needs to avoid conflicts. Also, the directory `build/include/stm32/sim` (or wherever you copy it) should be added to the include path in your tests because the compiler will look for `arm_acle.h` required by CMSIS.

#### Example

##### Loop

My workflows are mainly based on WFI and interrupts for power efficiency, so the code like this should work just fine:

```cpp
static volatile bool active = true;

while (active) {
    __WFI();
    // do stuff
}
```

If you need to check code with super loop, then you should call `cycle()` function yourself to simulate the hardware cycles and trigger the events. For example:

```cpp
static volatile bool active = true;

while (active) {
    Embys::STM32::Sim::cycle();
    // __NOP(); // also works, but cycle() is more explicit
    // do stuff
}
```

##### Interrupts

If you have defined an interrupt handler, then you need to inform the simulation about it:

```cpp
// your interrupt handler
void TIM2_IRQHandler() {
    // do stuff
}

// somewhere in the initialization code
Embys::STM32::Sim::TIM2_IRQ_Handler_ptr = TIM2_IRQHandler;
```

Same for: SysTick, EXTI, USART, I2C, SPI, PendSV

#### Status

Currently supported:

- Interrupts for TIM, SysTick, EXTI, USART, I2C, SPI, PendSV
- GPIO with triggering EXTI interrupts
- UART sequences for transmitting and receiving data (if your implementation satisfies the requirements listed in `src/stm32/sim/sim/uart/runtime.hpp`)
- I2C sequences for transmitting and receiving data (if your implementation satisfies the requirements listed in `src/stm32/sim/sim/i2c/runtime.hpp`)
