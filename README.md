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

This is a logical simulator, so no real hardware emulation like QEMU should be expected, but it is very lightweight and allows easy testing of application logic.

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

Provides the core firmware primitives: a hardware timer, a WFI-driven main loop with event scheduling, module (peripheral IRQ) integration, and nestable critical sections.

**Link**: add `libstm32-base.a` to `LDLIBS` and include the headers you need from `<embys/stm32/base/>`.

#### Timer

`Embys::Stm32::Base::Timer` wraps a general-purpose TIM peripheral in one-pulse mode and provides microsecond-precision scheduling. The library handles all peripheral initialisation; you are responsible for enabling the NVIC interrupt and writing the IRQ handler.

**Caller responsibilities:**

- Enable and prioritise `TIMx_IRQn` in NVIC
- Call `timer.handle_irq()` from the handler (it clears `TIM_SR_UIF` internally)

```cpp
Embys::Stm32::Base::Timer *timer_ptr = nullptr;

extern "C" void TIM2_IRQHandler()
{
  if (timer_ptr)
    timer_ptr->handle_irq();
  else
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
}

// In main:
Embys::Stm32::Base::Timer timer(TIM2);
timer_ptr = &timer;

__NVIC_SetPriority(TIM2_IRQn, 0x00);
__NVIC_EnableIRQ(TIM2_IRQn);
```

The `Timer` is used internally by `Loop` — you normally do not call `schedule_us()` directly.

#### Loop and Events

`Embys::Stm32::Base::Loop` is the application main loop. It sleeps with WFI and wakes on timer or module interrupts to run scheduled events and deferred module callbacks.

The Loop owns no dynamic memory — you provide the slot arrays:

```cpp
Embys::Stm32::Base::Timer timer(TIM2);

constexpr size_t events_capacity = 10;
static Embys::Stm32::Base::Event *event_slots[events_capacity];
static Embys::Stm32::Base::Event *active_event_slots[events_capacity];

constexpr size_t modules_capacity = 4;
static Embys::Stm32::Base::Module module_slots[modules_capacity];

Embys::Stm32::Base::Loop loop(&timer, event_slots, active_event_slots,
                              events_capacity, module_slots, modules_capacity);
loop.run();
```

**Events** are scheduled in microseconds. An event with `EV_PERSIST` is re-scheduled automatically after each execution.

| Flag         | Meaning                                                             |
| ------------ | ------------------------------------------------------------------- |
| _(none)_     | One-shot: fires once then is removed                                |
| `EV_PERSIST` | Periodic: re-scheduled after each execution                         |
| `EV_RT`      | Real-time: executed in IRQ context — must be short and non-blocking |

```cpp
void on_event(void *context) { /* ... */ }

// One-shot: fires after 100 ms
Embys::Stm32::Base::Event event1(&loop, 0, {on_event, &ctx});
event1.enable(100000);

// Periodic: fires every 500 ms
Embys::Stm32::Base::Event blink(&loop, Embys::Stm32::Base::EV_PERSIST,
                                {on_event, &ctx});
blink.enable(500000);

// Stop blinking
blink.disable();
```

#### Modules

A `Module` connects a peripheral IRQ to the loop. The IRQ handler calls `loop.interrupted(module)` to set a flag; the loop executes the module callback in application context after waking from WFI. Every peripheral driver (`Gpio::Bus`, `I2c::Bus`, etc.) uses this pattern.

```cpp
// In the IRQ handler:
extern "C" void EXTI0_IRQHandler()
{
  // signal the loop; actual processing happens in app context
  loop.interrupted(my_module);
}
```

Modules are registered automatically when a peripheral's `enable()` is called.

#### Critical Sections

`cs_begin()` / `cs_end()` implement a nestable critical section by saving and restoring `PRIMASK`. Include `<embys/stm32/base/cs.hpp>`.

```cpp
Embys::Stm32::cs_begin(); // disable interrupts, save PRIMASK
// ... access shared state ...
Embys::Stm32::cs_end();   // restore PRIMASK (nesting-aware)
```

### GPIO

Base path: `libs/stm32/gpio/`

Provides GPIO pin configuration and interrupt-driven input callbacks for STM32F1. Two classes make up the public API:

- `Embys::Stm32::Gpio::Bus` — a `Module` registered with `Base::Loop`. Owns a slot array of `Pin*`, configures MODE/CNF/EXTI for each enabled pin, and dispatches pin-level callbacks in loop context from EXTI IRQ handlers.
- `Embys::Stm32::Gpio::Pin` — represents a single GPIO pin. Supports output (push-pull, open-drain, AF) and input (floating, pull-up/pull-down, with optional EXTI interrupt).

**Caller responsibilities:**

- Enable NVIC for each `EXTIx_IRQn` used
- Route `EXTIx_IRQHandler → bus.handle_irq(start_line, end_line)`
- Reserve one module slot in the Loop

**Link**: add `libstm32-gpio.a` to `LDLIBS` and include `<embys/stm32/gpio/bus.hpp>` / `<embys/stm32/gpio/pin.hpp>`.

Key enums (from `api.hpp`):

| Enum     | Values                                                                  |
| -------- | ----------------------------------------------------------------------- |
| `Mode`   | `IN`, `OUT_2`, `OUT_10`, `OUT_50`                                       |
| `Cnf`    | `IN_AN`, `IN_FL`, `IN_PU`, `OUT_PP`, `OUT_OD`, `OUT_PP_AF`, `OUT_OD_AF` |
| `PinCfg` | `NONE`, `PULL_UP`, `PULL_DOWN`, `IRQ`                                   |

#### Example

```cpp
using GpioMode = Embys::Stm32::Gpio::Mode;
using GpioCnf  = Embys::Stm32::Gpio::Cnf;
using PinCfg   = Embys::Stm32::Gpio::PinCfg;

// The Bus doesn't allocate pin memory — provide slot storage.
// Only simultaneously-enabled pins need a slot; slots are reused on disable.
constexpr size_t gpio_pins_capacity = 2;
static Embys::Stm32::Gpio::Pin *gpio_pin_slots[gpio_pins_capacity];

Embys::Stm32::Gpio::Bus gpio_bus(&loop, gpio_pin_slots, gpio_pins_capacity);

// LED on PC13: output 2 MHz, push-pull, no extra config
Embys::Stm32::Gpio::Pin led_pin(&gpio_bus, GPIOC, 13, GpioMode::OUT_2,
                                GpioCnf::OUT_PP, PinCfg::NONE);
led_pin.set_init_value(1); // start with LED off (active-low)

// Button on PA0: input floating, EXTI on both edges
Embys::Stm32::Gpio::Pin button_pin(&gpio_bus, GPIOA, 0, GpioMode::IN,
                                   GpioCnf::IN_FL, PinCfg::IRQ);
button_pin.set_callback({toggle_btn, &context});

// Wire the IRQ handler (in your IRQ handler .cpp)
void EXTI0_IRQHandler() { gpio_bus.handle_irq(0, 0); }

// Enable the bus first, then each pin
gpio_bus.enable();
led_pin.enable();
button_pin.enable();
```

### UART

Base path: `libs/stm32/uart/`

Provides an interrupt-driven UART transceiver for STM32F1. The central class is `Embys::Stm32::Uart::Bus`, which integrates with `Base::Loop` as a `Module` (RX/TX callbacks are dispatched in loop context) and registers an internal TX timeout event.

TX is fully asynchronous — `write()` returns immediately and signals completion (or timeout) via the TX callback. RX bytes are accumulated into a caller-provided buffer and the RX callback is invoked per received byte in loop context.

**Caller responsibilities:**

- Configure TX pin as alternate-function push-pull output and RX pin as floating input before calling `enable()`
- Enable `USARTx_IRQn` in NVIC
- Route `USARTx_IRQHandler → bus.handle_irq()`
- Provide a receive buffer at construction time
- Reserve one extra event slot in the Loop (used for the TX timeout)

**Link**: add `libstm32-uart.a` to `LDLIBS` and include `<embys/stm32/uart/bus.hpp>`.

Configuration enums (from `def.hpp`):

| Enum         | Values                |
| ------------ | --------------------- |
| `Parity`     | `None`, `Even`, `Odd` |
| `StopBits`   | `One`, `Two`          |
| `WordLength` | `W8`, `W9`            |

Error codes are defined in `Embys::Stm32::Uart::Diag` (e.g. `TX_BUSY`, `TX_TIMEOUT`, `RX_OVERFLOW`).

```cpp
static uint8_t rx_buf[64];
Embys::Stm32::Uart::Bus uart(USART1, &loop, rx_buf, sizeof(rx_buf));

uart.set_rx_callback({on_rx, &context});
uart.set_tx_callback({on_tx_done, &context});

// Wire the IRQ handler
extern "C" void USART1_IRQHandler() { uart.handle_irq(); }

// Enable at 115200 8N1 (defaults)
uart.enable(115200);

// Asynchronous transmit
const uint8_t msg[] = "hello\r\n";
uart.write(msg, sizeof(msg) - 1);
```

### I2C

Base path: `libs/stm32/i2c/`

Provides an interrupt-driven I2C master for STM32F1. The central class is `Embys::Stm32::I2c::Bus`, which integrates with `Base::Loop` as a `Module` (completions are dispatched in loop context) and registers a timeout event.

All transfers are fully asynchronous — `read()` and `write()` return immediately; your callback is invoked from the main loop once the transfer completes or fails.

**Caller responsibilities:**

- Configure SCL/SDA GPIO pins as open-drain AF output before calling `enable()`
- Enable and prioritise `I2Cx_EV_IRQn` and `I2Cx_ER_IRQn` in NVIC
- Route the IRQ handlers to `bus.handle_ev_irq()` / `bus.handle_er_irq()`
- Reserve one extra event slot in the Loop (used for the transaction timeout)

**Link**: add `libstm32-i2c.a` to `LDLIBS` and include `<embys/stm32/i2c/bus.hpp>`.

```cpp
// Construct and enable at 100 kHz (default)
Embys::Stm32::I2c::Bus i2c_bus(I2C1, &loop);
i2c_bus.enable();          // or enable(400000) for 400 kHz

// Wire up IRQ handlers (in the same .cpp as your I2Cx_*_IRQHandler definitions)
void I2C1_EV_IRQHandler() { i2c_bus.handle_ev_irq(); }
void I2C1_ER_IRQHandler() { i2c_bus.handle_er_irq(); }

// Asynchronous write — callback fires in loop context
i2c_bus.write(0x27, buf, sizeof(buf), {on_done, &context});

// Asynchronous register-addressed read (write reg, repeated START, read)
i2c_bus.read(0x38, 0xAC, rx_buf, 6, {on_done, &context});
```

Error codes are defined in `Embys::Stm32::I2c::Diag` (e.g. `NACK`, `TIMEOUT`, `BUS_BUSY`).

### Tests

Tests are located in the `tests/` directory and are worth exploring. They cover most of the functionality provided by the library. Tests are written using the Doctest library.

### Examples

#### GPIO blink

Located in the `examples/gpio_blink/` directory, this example demonstrates how to use the loop and timer to blink an LED on the PC13 pin. It also shows how to use the simulator to run the example without actual hardware.

You need to have all libraries built for the target architecture (ARM or simulation) to run the example.

If you have a Blue Pill, build it with `make`, then flash the binary to your STM32 microcontroller connected via ST-Link (I have an ST-Link V2 from Aliexpress) with `make flash` and see the LED blinking, toggling every 500ms.

For simulation, you can run the example in the simulator `make TC=sim run` and see the output in the console.
Press Ctrl+C to terminate.

#### GPIO button blink

Located in the `examples/gpio_btn_blink/` directory, this example demonstrates how to use the loop, timer, and GPIO to toggle blinking of an LED on the PC13 pin when a button connected to the PA0 pin is pressed. It also shows how to use the simulator to run the example without actual hardware.

All libraries must be built for the target architecture (ARM or simulation) to run the example.

Build the example with `make`, then flash the binary to your STM32 microcontroller connected via ST-Link with `make flash` and see the LED blinking, toggling every 500ms when you press the button. Press button once to start blinking, press again to stop.
For simulation, you can run the example in the simulator `make TC=sim run` and see the output in the console. Press Ctrl+C to terminate.
Since there's no hardware button connected, simulator can accept commands through named pipe to trigger the button press event.
You can run `make btn-toggle` in the example directory to simulate a button press and release.
Run this command multiple times to see the effect.

#### UART print

Located in the `examples/uart_print/` directory, this example demonstrates basic UART TX using the interrupt-driven `Uart::Bus` driver. It sends `"Hello from Blue Pill!\r\n"` over USART1 every 2 seconds using a periodic loop event.

All libraries must be built for the target architecture (ARM or simulation) to run the example.

Build with `make`, flash with `make flash`.

Wire up (Blue Pill / STM32F103C8):

- PA9 → TX
- PA10 → RX
- 3.3V → 3.3V
- GND → GND

Swap PA9/PA10 connections if using a USB-to-UART adapter with TX/RX reversed.

Open a serial terminal at **115200 8N1**. You should see the message printed every 2 seconds.

For simulation, run `make TC=sim run` — the message is printed to stdout at an accelerated interval. Press Ctrl+C to terminate.

#### UART echo

Located in the `examples/uart_echo/` directory, this example demonstrates full-duplex UART using `Uart::Bus`. It reads incoming bytes over USART1 and echoes each complete line back (terminated by `\r` or `\n`) followed by `\r\n`. Received bytes are accumulated in a 64-byte line buffer; if TX is busy the buffer continues to fill until the current transmission completes.

All libraries must be built for the target architecture (ARM or simulation) to run the example.

Wire up (Blue Pill / STM32F103C8):

- PA9 → TX (connect to RX of a USB-UART adapter)
- PA10 → RX (connect to TX of a USB-UART adapter)
- 3.3V → 3.3V
- GND → GND

Open a serial terminal at **115200 8N1**. Build with `make`, flash with `make flash`. Type a line and press Enter — the device echoes the whole line back.

For simulation, run `make TC=sim run`. Press Ctrl+C to terminate.

#### I2C button blink

Located in the `examples/i2c_btn_blink/` directory, this example extends the GPIO button blink example by adding an HD44780 LCD display connected over I2C. The LCD shows the current blink status and a running count of LED toggles. Pressing the button on PA0 starts and stops blinking; the LCD updates accordingly.

All libraries must be built for the target architecture (ARM or simulation) to run the example.

Wire up:

- LED on PC13 (active-low, push-pull output)
- Button on PA0 (input floating, EXTI)
- HD44780-compatible LCD (I2C PCF8574 backpack) on I2C1: SCL on PB6, SDA on PB7

Build with `make`, then flash with `make flash`. Press the button to toggle blinking; the LCD updates the blink status and count in real time.

For simulation, run `make TC=sim run`. As with the GPIO button blink example, the simulator accepts commands through the named pipe. Run `make btn-toggle` in the example directory to simulate a button press and release.
