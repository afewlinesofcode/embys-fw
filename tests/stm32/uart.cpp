#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/sim/sim.hpp>
#include <embys/stm32/uart/api.hpp>
#include <embys/stm32/uart/bus.hpp>
#include <embys/stm32/uart/diag.hpp>

#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;
using namespace Embys::Stm32;

// ── fixtures ──────────────────────────────────────────────────────────────

struct UartBaseFixture
{
  inline static Base::Timer *timer_ptr = nullptr;
  inline static Uart::Bus *uart_bus_ptr = nullptr;

  static void
  TIM2_IRQHandler()
  {
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
    if (timer_ptr)
      timer_ptr->handle_irq();
  }

  static void
  USART2_IRQHandler()
  {
    if (uart_bus_ptr)
      uart_bus_ptr->handle_irq();
  }

  UartBaseFixture()
  {
    Sim::reset();
    // Point the uart simulation at USART2 for all Bus tests.
    // uart.cpp::reset() only puts usart1_instance in idle state; replicate
    // that for USART2 so TXE is high and the first TXE IRQ can fire.
    Sim::Uart::usart = USART2;
    USART2->DR = 0;
    USART2->SR = USART_SR_TXE | USART_SR_TC;
    timer_ptr = nullptr;
    uart_bus_ptr = nullptr;
    Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
    Sim::USART2_IRQHandler_ptr = USART2_IRQHandler;
  }
};

struct UartLoopFixture : UartBaseFixture
{
  // events_capacity: 3 for the loop + 1 for Bus::timeout_event
  static constexpr size_t events_capacity = 4;
  static constexpr size_t modules_capacity = 1;

  Base::Event *event_slots[events_capacity];
  Base::Event *active_event_slots[events_capacity];
  Base::Module module_slots[modules_capacity];

  uint8_t rx_buf[16];

  Base::Timer timer;
  Base::Loop loop;
  Uart::Bus bus;

  UartLoopFixture()
    : timer(TIM2), loop(&timer, event_slots, active_event_slots,
                        events_capacity, module_slots, modules_capacity),
      bus(USART2, &loop, rx_buf, sizeof(rx_buf))
  {
    timer_ptr = &timer;
    uart_bus_ptr = &bus;
  }
};

// For the REDE test: a dummy Gpio::Bus gives Pin a valid bus pointer.
// gpio_bus is never enabled so no module slot is consumed by it.
struct UartRedeFixture : UartLoopFixture
{
  Gpio::Pin *dummy_pin_slots[1];
  Gpio::Bus gpio_bus;
  Gpio::Pin rede;

  UartRedeFixture()
    : gpio_bus(&loop, dummy_pin_slots, 1),
      rede(&gpio_bus, GPIOA, 5, Gpio::Mode::OUT_2, Gpio::Cnf::OUT_PP,
           Gpio::PinCfg::NONE)
  {
  }
};

// ── calc_frame_bits ───────────────────────────────────────────────────────

TEST_CASE("calc_frame_bits: W8 + 1 stop bit = 10 total bits")
{
  CHECK(Uart::calc_frame_bits(Uart::WordLength::W8, Uart::StopBits::One) == 10);
}

TEST_CASE("calc_frame_bits: W9 + 2 stop bits = 12 total bits")
{
  CHECK(Uart::calc_frame_bits(Uart::WordLength::W9, Uart::StopBits::Two) == 12);
}

// ── enable_uart / disable_uart ────────────────────────────────────────────

TEST_CASE_FIXTURE(UartBaseFixture,
                  "enable_uart: enables APB1 clock and sets UE, TE, RE, RXNEIE")
{
  Uart::enable_uart(USART2, 115200, Uart::WordLength::W8, Uart::StopBits::One,
                    Uart::Parity::None);

  CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) != 0);
  CHECK((USART2->CR1 & USART_CR1_UE) != 0);
  CHECK((USART2->CR1 & USART_CR1_TE) != 0);
  CHECK((USART2->CR1 & USART_CR1_RE) != 0);
  CHECK((USART2->CR1 & USART_CR1_RXNEIE) != 0);
  CHECK((USART2->CR1 & USART_CR1_TXEIE) == 0);
  CHECK((USART2->CR1 & USART_CR1_TCIE) == 0);
}

TEST_CASE_FIXTURE(UartBaseFixture,
                  "enable_uart: BRR matches APB1 clock divided by baud rate")
{
  constexpr uint32_t baud = 115200;
  Uart::enable_uart(USART2, baud, Uart::WordLength::W8, Uart::StopBits::One,
                    Uart::Parity::None);

  CHECK(USART2->BRR == SystemCoreClock / 2 / baud);
}

TEST_CASE_FIXTURE(UartBaseFixture,
                  "enable_uart: two stop bits configures CR2.STOP")
{
  Uart::enable_uart(USART2, 9600, Uart::WordLength::W8, Uart::StopBits::Two,
                    Uart::Parity::None);

  CHECK((USART2->CR2 & USART_CR2_STOP) ==
        (static_cast<uint32_t>(Uart::StopBits::Two) << USART_CR2_STOP_Pos));
}

TEST_CASE_FIXTURE(UartBaseFixture,
                  "enable_uart: even parity sets PCE, clears PS")
{
  Uart::enable_uart(USART2, 9600, Uart::WordLength::W8, Uart::StopBits::One,
                    Uart::Parity::Even);

  CHECK((USART2->CR1 & USART_CR1_PCE) != 0);
  CHECK((USART2->CR1 & USART_CR1_PS) == 0);
}

TEST_CASE_FIXTURE(UartBaseFixture, "enable_uart: odd parity sets PCE and PS")
{
  Uart::enable_uart(USART2, 9600, Uart::WordLength::W8, Uart::StopBits::One,
                    Uart::Parity::Odd);

  CHECK((USART2->CR1 & USART_CR1_PCE) != 0);
  CHECK((USART2->CR1 & USART_CR1_PS) != 0);
}

TEST_CASE_FIXTURE(UartBaseFixture,
                  "enable_uart: returns INVALID_USART for unknown peripheral")
{
  CHECK(Uart::enable_uart(nullptr, 9600, Uart::WordLength::W8,
                          Uart::StopBits::One,
                          Uart::Parity::None) == Uart::INVALID_USART);
}

TEST_CASE_FIXTURE(UartBaseFixture,
                  "disable_uart: clears UE and disables APB1 clock")
{
  Uart::enable_uart(USART2, 115200, Uart::WordLength::W8, Uart::StopBits::One,
                    Uart::Parity::None);
  Uart::disable_uart(USART2);

  CHECK((USART2->CR1 & USART_CR1_UE) == 0);
  CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) == 0);
}

// ── Bus::enable / disable ─────────────────────────────────────────────────

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus::enable registers module and reports is_enabled")
{
  CHECK(!bus.is_enabled());
  CHECK(bus.enable(115200) == 0);
  CHECK(bus.is_enabled());
  CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) != 0);
  CHECK((USART2->CR1 & USART_CR1_UE) != 0);
}

TEST_CASE_FIXTURE(UartLoopFixture, "Bus::enable is idempotent")
{
  bus.enable(115200);
  uint32_t cr1 = USART2->CR1;

  bus.enable(9600); // second call with different rate — no-op
  CHECK(USART2->CR1 == cr1);
}

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus::disable deregisters module and disables peripheral")
{
  bus.enable(115200);
  CHECK(bus.disable() == 0);
  CHECK(!bus.is_enabled());
  CHECK((USART2->CR1 & USART_CR1_UE) == 0);
  CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) == 0);
}

// ── TX ────────────────────────────────────────────────────────────────────

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus::write returns BUS_NOT_ENABLED when not enabled")
{
  const uint8_t data[] = {0x01};
  CHECK(bus.write(data, sizeof(data)) == Uart::BUS_NOT_ENABLED);
}

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus::write enables TXEIE and reports is_tx_busy")
{
  bus.enable(115200);

  const uint8_t data[] = {0xAB};
  CHECK(bus.write(data, sizeof(data)) == 0);

  CHECK(bus.is_tx_busy());
  CHECK((USART2->CR1 & USART_CR1_TXEIE) != 0);
}

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus::write returns TX_BUSY while transmit is in progress")
{
  bus.enable(115200);

  const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
  CHECK(bus.write(data, sizeof(data)) == 0);
  CHECK(bus.write(data, sizeof(data)) == Uart::TX_BUSY);
}

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus: transmitted bytes appear in sim tx_buffers, "
                  "callback receives zero result")
{
  bus.enable(115200);

  int tx_result = -1;
  bus.set_tx_callback(
      {[](void *ctx, int r) { *static_cast<int *>(ctx) = r; }, &tx_result});

  const uint8_t data[] = {0xDE, 0xAD};
  bus.write(data, sizeof(data));

  loop.stop(5000);
  loop.run();

  CHECK(tx_result == 0);
  CHECK(!bus.is_tx_busy());
  REQUIRE(!Sim::Uart::tx_buffers.empty());
  CHECK(Sim::Uart::tx_buffers.back() == std::vector<uint8_t>({0xDE, 0xAD}));
}

// ── RX ────────────────────────────────────────────────────────────────────

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus: single received byte delivered to RX callback")
{
  bus.enable(115200);

  std::vector<uint8_t> received;
  bus.set_rx_callback(
      {[](void *ctx, uint8_t b)
       { static_cast<std::vector<uint8_t> *>(ctx)->push_back(b); }, &received});

  Sim::Uart::simulate_rx({'A'});

  loop.stop(5000);
  loop.run();

  REQUIRE(received.size() == 1);
  CHECK(received[0] == 'A');
}

TEST_CASE_FIXTURE(UartLoopFixture,
                  "Bus: multiple received bytes delivered in order")
{
  bus.enable(115200);

  std::vector<uint8_t> received;
  bus.set_rx_callback(
      {[](void *ctx, uint8_t b)
       { static_cast<std::vector<uint8_t> *>(ctx)->push_back(b); }, &received});

  Sim::Uart::simulate_rx({0x11, 0x22, 0x33});

  loop.stop(5000);
  loop.run();

  REQUIRE(received.size() == 3);
  CHECK(received[0] == 0x11);
  CHECK(received[1] == 0x22);
  CHECK(received[2] == 0x33);
}

// ── REDE pin ──────────────────────────────────────────────────────────────

TEST_CASE_FIXTURE(UartRedeFixture,
                  "Bus: REDE pin driven high on write, low after TC")
{
  bus.enable(115200);
  bus.set_rede_pin(&rede);

  int tx_result = -1;
  bus.set_tx_callback(
      {[](void *ctx, int r) { *static_cast<int *>(ctx) = r; }, &tx_result});

  const uint8_t data[] = {0xCC};
  GPIOA->BSRR = 0;
  bus.write(data, sizeof(data));

  // REDE must be asserted immediately when write() is called
  CHECK((GPIOA->BSRR & (1u << 5)) != 0);

  loop.stop(5000);
  loop.run();

  // REDE must be de-asserted after the TC interrupt fires
  CHECK(tx_result == 0);
  CHECK((GPIOA->BSRR & (1u << (5 + 16))) != 0);
}
