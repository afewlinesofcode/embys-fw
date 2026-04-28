#include <stddef.h>
#include <stm32f1xx.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/system.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/i2c-hd44780/device.hpp>
#include <embys/stm32/i2c/api.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"
#include "sim.hpp"

Embys::Stm32::Base::Timer *timer_ptr = nullptr;
Embys::Stm32::Gpio::Bus *gpio_ptr = nullptr;
Embys::Stm32::I2c::Bus *i2c_bus_ptr = nullptr;

extern "C"
{
  void
  TIM2_IRQHandler()
  {
    if (timer_ptr)
      timer_ptr->handle_irq();
    else
      CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
  }

  void
  EXTI0_IRQHandler()
  {
    if (gpio_ptr)
      gpio_ptr->handle_irq(0, 0);
    else
      EXTI->PR = (1 << 0);
  }

  void
  I2C1_EV_IRQHandler()
  {
    if (i2c_bus_ptr)
      i2c_bus_ptr->handle_ev_irq();
  }

  void
  I2C1_ER_IRQHandler()
  {
    if (i2c_bus_ptr)
      i2c_bus_ptr->handle_er_irq();
  }
}

enum class LcdState
{
  Idle,
  UpdatingStatus,
  UpdatingCounter
};

struct AppContext
{
  bool led_on = false;
  bool blink_on = false;
  uint32_t blink_count = 0;
  bool lcd_ready = false;
  LcdState lcd_state = LcdState::Idle;
  bool status_pending = false;
  bool counter_pending = false;
  int result = 0;

  Embys::Stm32::Gpio::Pin *led = nullptr;
  Embys::Stm32::Base::Event *blink_event = nullptr;
  Embys::Stm32::I2c::Dev::Hd44780::Device *lcd = nullptr;

  char counter_buf[32];
};

// Forward declaration
static void
update_lcd(AppContext *ctx);

// Format "Blink counter: N   " (padded to 20 chars) into buf.
static void
write_counter(char *buf, uint32_t n)
{
  static const char prefix[] = "Blink counter: ";
  uint8_t i = 0;
  for (; prefix[i]; ++i)
    buf[i] = prefix[i];

  char digits[11];
  uint8_t dlen = 0;
  if (n == 0)
  {
    digits[dlen++] = '0';
  }
  else
  {
    while (n)
    {
      digits[dlen++] = static_cast<char>('0' + n % 10);
      n /= 10;
    }
  }
  for (int8_t j = static_cast<int8_t>(dlen) - 1; j >= 0; --j)
    buf[i++] = digits[j];
  while (i < 20)
    buf[i++] = ' ';
  buf[i] = '\0';
}

static void
on_lcd_counter_done(void *context, int result)
{
  if (result < 0)
    return;

  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd_state = LcdState::Idle;
  update_lcd(ctx);
}

static void
on_lcd_status_done(void *context, int result)
{
  if (result < 0)
    return;

  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd_state = LcdState::Idle;
  update_lcd(ctx);
}

static void
update_lcd(AppContext *ctx)
{
  if (!ctx->lcd_ready || ctx->lcd_state != LcdState::Idle)
    return;

  if (ctx->status_pending)
  {
    ctx->status_pending = false;
    ctx->lcd_state = LcdState::UpdatingStatus;
    const char *str =
        ctx->blink_on ? "Blink status: on    " : "Blink status: off   ";
    ctx->lcd->print_line(1, str, {on_lcd_status_done, ctx});
    return;
  }

  if (ctx->counter_pending)
  {
    ctx->counter_pending = false;
    ctx->lcd_state = LcdState::UpdatingCounter;

    if (ctx->blink_on)
    {
      write_counter(ctx->counter_buf, ctx->blink_count);
      ctx->lcd->print_line(2, ctx->counter_buf, {on_lcd_counter_done, ctx});
    }
    else
    {
      ctx->lcd->clear_line(2, {on_lcd_counter_done, ctx});
    }
  }
}

static void
on_init_done(void *context, int result)
{
  if (result < 0)
    return;

  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd_ready = true;
  SIM_LOG("LCD ready");
}

static void
on_title_done(void *context, int result)
{
  if (result < 0)
    return;

  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd->print_line(1, "Blink status: off   ", {on_init_done, ctx});
}

static void
on_lcd_enabled(void *context, int result)
{
  if (result < 0)
    return;

  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd->print_line(0, "Blink App", {on_title_done, ctx});
}

static void
on_start(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd->enable({on_lcd_enabled, ctx});
}

static void
toggle_led(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->led_on = !ctx->led_on;
  ctx->led->write(ctx->led_on ? 1 : 0);

  if (ctx->led_on)
  {
    ctx->blink_count++;
    ctx->counter_pending = true;
    update_lcd(ctx);
    SIM_LOG("LED ON  count=" << ctx->blink_count);
  }
  else
  {
    SIM_LOG("LED OFF count=" << ctx->blink_count);
  }
}

static void
toggle_btn(void *context, uint8_t value)
{
  SIM_LOG("Button: " << (value ? "RELEASED" : "PRESSED"));
  auto *ctx = static_cast<AppContext *>(context);

  // Act on button release (button is active-low, value=1 means released)
  if (value == 1)
  {
    ctx->blink_on = !ctx->blink_on;

    if (ctx->blink_on)
    {
      ctx->blink_event->enable(LED_BLINK_INTERVAL_US);
      SIM_LOG("Blinking ON");
    }
    else
    {
      ctx->blink_event->disable();
      ctx->led_on = false;
      ctx->led->write(0);
      ctx->blink_count = 0;
      SIM_LOG("Blinking OFF");
    }

    ctx->status_pending = true;
    ctx->counter_pending = true;
    update_lcd(ctx);
  }
}

int
main()
{
  using GpioMode = Embys::Stm32::Gpio::Mode;
  using GpioCnf = Embys::Stm32::Gpio::Cnf;
  using PinCfg = Embys::Stm32::Gpio::PinCfg;

  // Arrays placed in .bss (static) to avoid compiler-generated memset calls
  // on ARM -nostdlib targets.
  static AppContext context;

  SIM_RESET();

  Embys::Stm32::Base::Timer timer(TIM2);

  // Events:
  //   - loop stop (internal to Loop)
  //   - blink event
  //   - I2C bus timeout event
  //   - I2C device delay event
  constexpr size_t events_capacity = 5;
  static Embys::Stm32::Base::Event *event_slots[events_capacity];
  static Embys::Stm32::Base::Event *active_event_slots[events_capacity];

  // Modules:
  //   - GPIO bus
  //   - I2C bus
  constexpr size_t modules_capacity = 2;
  static Embys::Stm32::Base::Module module_slots[modules_capacity];

  Embys::Stm32::Base::Loop loop(&timer, event_slots, active_event_slots,
                                events_capacity, module_slots,
                                modules_capacity);

  Embys::Stm32::Base::Event blink_event(&loop, Embys::Stm32::Base::EV_PERSIST,
                                        {toggle_led, &context});

  // One-shot startup event: fires on the first loop iteration (us=0)
  Embys::Stm32::Base::Event start_event(&loop, 0, {on_start, &context});

  constexpr size_t gpio_pins_capacity = 4;
  static Embys::Stm32::Gpio::Pin *gpio_pin_slots[gpio_pins_capacity];

  Embys::Stm32::Gpio::Bus gpio_bus(&loop, gpio_pin_slots, gpio_pins_capacity);

  // Button on PA0 (input floating, EXTI)
  Embys::Stm32::Gpio::Pin button_pin(&gpio_bus, GPIOA, 0, GpioMode::IN,
                                     GpioCnf::IN_FL, PinCfg::IRQ);
  button_pin.set_callback({toggle_btn, &context});

  // LED on PC13 (output push-pull, 2 MHz)
  Embys::Stm32::Gpio::Pin led_pin(&gpio_bus, GPIOC, 13, GpioMode::OUT_2,
                                  GpioCnf::OUT_PP, PinCfg::NONE);

  // I2C1 SCL on PB6 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin i2c_scl(&gpio_bus, GPIOB, 6, GpioMode::OUT_50,
                                  GpioCnf::OUT_OD_AF, PinCfg::NONE);

  // I2C1 SDA on PB7 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin i2c_sda(&gpio_bus, GPIOB, 7, GpioMode::OUT_50,
                                  GpioCnf::OUT_OD_AF, PinCfg::NONE);

  Embys::Stm32::I2c::Bus i2c_bus(I2C1, &loop);

  Embys::Stm32::I2c::Dev::Hd44780::Device lcd(&loop, &i2c_bus);

  timer_ptr = &timer;
  gpio_ptr = &gpio_bus;
  i2c_bus_ptr = &i2c_bus;
  context.led = &led_pin;
  context.blink_event = &blink_event;
  context.lcd = &lcd;

  Embys::Stm32::Base::system_init();

  TRY(gpio_bus.enable());
  TRY(button_pin.enable());
  TRY(led_pin.enable());
  TRY(i2c_scl.enable());
  TRY(i2c_sda.enable());
  TRY(i2c_bus.enable(100000));

  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);
  __NVIC_EnableIRQ(EXTI0_IRQn);
  __NVIC_EnableIRQ(I2C1_EV_IRQn);
  __NVIC_SetPriority(I2C1_EV_IRQn, 0x01);
  __NVIC_EnableIRQ(I2C1_ER_IRQn);
  __NVIC_SetPriority(I2C1_ER_IRQn, 0x01);

  // Schedule LCD initialisation on the first loop iteration
  start_event.enable(0);

  loop.run();

  return 0;
}
