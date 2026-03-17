# Embys FW

A small firmware experimentation framework for exploring interrupt-driven architectures, peripheral behaviour simulation and deterministic firmware loops. Currently focused on STM32 microcontrollers. I have a Blue Pill, so I build and test on it.

Features:

- STM32 peripheral simulation on x86
- interrupt-driven firmware loop
- UART / I2C / ... interrupt-based drivers

These features will be extracted from my existing projects, with documentation added as modules are implemented.

## Structure

The repository contains libraries and examples. Libraries are located in `libs/` and contain reusable code for STM32 and simulation. Examples are located in `examples/` and demonstrate how to use the libraries in real applications.

Every library and example has its own `Makefile` with targets for building, running, and cleaning.

The `make` command accepts the `TC` variable to specify the toolchain. Currently, `arm` is the default. For example, `make TC=arm` (or just `make`) will build for ARM architecture, while `make TC=sim` will build for simulation.

`make clean` will remove all build artifacts for the architecture specified with `TC`.

All library binaries and build artifacts are located in the `build/` directory, organized by architecture and library name.

All library includes are located in the `build/include/` directory, organized by library name.

Each example's binaries and build artifacts are located in its own `build/` sub-directory.

To run an example in the simulator, you can use `make TC=sim run` in the example directory.

There are also global targets defined in the root `Makefile`:

- `make`, `make TC=arm` - build all libraries for ARM architecture
- `make TC=sim` - build all libraries for simulation
- `make test` - run all tests in the simulator
- `make clean-tests` - clean all test build artifacts
- `make examples` - build all examples for ARM architecture
- `make examples TC=sim` - build all examples for simulation
- `make clean`, `make TC=arm clean` - clean all build artifacts for ARM architecture
- `make TC=sim clean` - clean all build artifacts for simulation

### Simulation

Base path: `libs/stm32/sim/`

This allows you to run tests and apps in a Linux container without needing actual STM32 hardware. I needed to develop applications for the STM32F103C8T6, so the implementation may be somewhat biased toward that chip.

This is a logical simulator, so no real hardware emulation like QEMU should be expected, but it is very lightweight and allows easy testing of application logic. I'm not sure about code based on HAL.

I tried to simulate the hardware behavior and sequences as closely as possible to the real hardware according to my needs, covering only the buses that I've used in my projects. Some features may be missing, and you may think things should be done differently - feel free to open an issue or create a PR.

The long-term vision of this project is to create a software simulator that behaves almost like a real STM32, covering as much of the microcontroller functionality as possible. Contributions are very welcome.

#### Link

When adding this feature to your project, you will probably need to replace all includes of `stm32f1xx.h` with conditional includes (`stm32f1xx.h` for real hardware and `embys/stm32/sim/sim.hpp` for simulation) because `embys/stm32/sim/sim.hpp` is based on `stm32f1xx.h` and redefines pointers and functions to be simulation instances, which needs to avoid conflicts. Also, in addition to the `build/include` directory (or wherever you copy it), `build/include_sim` should also be added to the include path in your tests because the compiler will look for `arm_acle.h` required by CMSIS.

#### Example

##### Loop

My workflows are mainly based on WFI and interrupts for power efficiency, so code like this should work just fine:

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
extern "C"
void TIM2_IRQHandler() {
    // do stuff
}

// somewhere in the initialization code
Embys::Stm32::Sim::TIM2_IRQ_Handler_ptr = TIM2_IRQHandler;
```

Same for: SysTick, EXTI, USART, I2C, SPI, PendSV

#### Status

Currently supported:

- Interrupts for TIM, SysTick, EXTI, USART, I2C, SPI, PendSV
- GPIO with triggering EXTI interrupts
- UART sequences for transmitting and receiving data (if your implementation satisfies the requirements listed in `src/stm32/sim/sim/uart.hpp`)
- I2C sequences for transmitting and receiving data (if your implementation satisfies the requirements listed in `src/stm32/sim/sim/i2c.hpp`)

### Base

Base path: `libs/stm32/base/`

Library provides basic building blocks for STM32 firmware development, such as:

- Interrupt-driven firmware loop
- Timer with callback support
- Critical section management

#### Timer

The timer implementation allows you to create timers with callback functions that will be called when the timer expires.

All necessary initialization and configuration of the hardware timer is handled by the library; however, you need to provide the implementation of the timer interrupt handler to clear the interrupt flag and call the timer instance in it to trigger the callbacks. For example:

```cpp
// your timer instance
Embys::STM32::Base::Timer *timer_ptr = nullptr;

// your interrupt handler
extern "C"
void TIM2_IRQHandler() {
    if (timer_ptr) {
        (*timer_ptr)();
    }
}

// your timer callback
void timer_callback(void *context) {
  // do stuff
}

// somewhere in the initialization code
Embys::STM32::Base::Timer timer(TIM2);
timer_ptr = &timer;
timer.set_callback({timer_callback, &ctx});
```

The timer always starts in one-pulse mode and doesn't restart automatically. It allows:

- reset() - to restart the timer with the same duration if it is still enabled
- restart() - to restart the timer with the same duration even if it is already expired
- schedule_us() - to schedule the timer with a new duration and jitter in microseconds

The timer is initialized with the highest interrupt priority.

#### Loop

The loop is the heart of the firmware application. It is based on WFI and Timer.
It is designed to be simple and efficient, allowing events to run according to the desired workflow and timing requirements.

If an application contains any logic related to other peripherals with their own interrupts and handlers, it should be connected to the loop through modules.

A module is technically a callback that can be executed by the loop as soon as the loop returns to the application context after WFI. If such processing is required, the peripheral interrupt handler should notify the loop by calling the `loop->interrupted()` method.

##### Examples

```cpp
// create a loop instance
Embys::Stm32::Base::Timer timer(TIM2);

// The loop instance doesn't allocate any memory for events and modules,
// so you need to provide it yourself
constexpr size_t events_capacity = 10;
Embys::Stm32::Base::Event* events[events_capacity];
Embys::Stm32::Base::Event* active_events[events_capacity];
constexpr size_t modules_capacity = 10;
Embys::Stm32::Base::Module* modules[modules_capacity];

Embys::Stm32::Base::Loop loop(timer,
                              events, active_events,events_capacity,
                              modules, modules_capacity);
```

Then you can add events to the loop:

```cpp
// create an event
void event_callback(void *context) {
    // do stuff
}
Embys::Stm32::Base::Event event1(&loop, 0, {event_callback, &ev1_ctx});
event1.enable(100000); // to run after 100ms

Embys::Stm32::Base::Event event2(&loop, EV_PERSIST, {event_callback, &ev2_ctx});
event2.enable(200000); // to run every 200ms
```

### Tests

Tests are located in the `tests/` directory and are worth exploring. They cover most of the functionality provided by the library. Tests are written using the Doctest library.

### Examples

#### GPIO blink

Located in the `examples/gpio_blink/` directory, this example demonstrates how to use the loop and timer to blink an LED on the PC13 pin. It also shows how to use the simulator to run the example without actual hardware.

You need to have all libraries built for the target architecture (ARM or simulation) to run the example.

If you have a Blue Pill, build it with `make`, then flash the binary to your STM32 microcontroller connected via ST-Link (I have an ST-Link V2 from Aliexpress) with `make flash` and see the LED blinking, toggling every 500ms.

For simulation, you can run the example in the simulator `make TC=sim run` and see the output in the console.
Press Ctrl+C to terminate.
