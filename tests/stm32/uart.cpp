#include <array>
#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/sim/sim.hpp>
#include <embys/stm32/uart/bus.hpp>
#include <embys/stm32/uart/hal.hpp>

#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;
using namespace Embys::Stm32;

static_assert(Uart::instance_available<Stm32f103xb, Uart::Instance::Usart3>);
static_assert(!Uart::instance_available<Stm32f103xb, Uart::Instance::Usart6>);
static_assert(Uart::instance_available<Stm32f407xx, Uart::Instance::Usart3>);
static_assert(Uart::instance_available<Stm32f411xe, Uart::Instance::Usart6>);
static_assert(!Uart::instance_available<Stm32f411xe, Uart::Instance::Usart3>);

struct ReceivedBytes
{
  std::array<uint8_t, 16> values{};
  std::array<Uart::Error, 1> errors{};
  size_t size = 0;
  size_t error_count = 0;

  static void
  record(void *context, Uart::ReceiveResult result) noexcept
  {
    auto *received = static_cast<ReceivedBytes *>(context);
    if (result)
    {
      received->values[received->size++] = result.value();
      return;
    }

    received->errors[received->error_count++] = result.error();
  }
};

struct TransmitResult
{
  bool called = false;
  bool succeeded = false;
  Uart::Error error = Uart::Error::NotEnabled;

  static void
  record(void *context, Uart::Status result) noexcept
  {
    auto *transmit = static_cast<TransmitResult *>(context);
    transmit->called = true;
    transmit->succeeded = result.has_value();
    if (!result)
      transmit->error = result.error();
  }
};

// ── fixtures ──────────────────────────────────────────────────────────────

struct UartBaseFixture
{
  inline static Base::Timer *timer_ptr = nullptr;
  inline static Uart::BusCore *uart_bus_ptr = nullptr;

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

  Base::Timer timer;
  Base::Loop<events_capacity, modules_capacity> loop;
  Uart::Bus<Uart::Instance::Usart2, 16, 32> bus;

  UartLoopFixture() : timer(TIM2), loop(timer), bus(loop)
  {
    timer_ptr = &timer;
    uart_bus_ptr = &bus;
  }
};

// For the REDE test: a dummy Gpio::Bus gives Pin a valid bus pointer.
// gpio_bus is never enabled so no module slot is consumed by it.
struct UartRedeFixture : UartLoopFixture
{
  Gpio::Bus<1> gpio_bus;
  Gpio::Pin<Gpio::Port::A, 5, Gpio::PinCfg::OUT> rede;

  UartRedeFixture() : gpio_bus(loop), rede(gpio_bus)
  {
  }
};

// ── calc_frame_bits ───────────────────────────────────────────────────────

TEST_SUITE("uart")
{

  TEST_CASE("calc_frame_bits: W8 + 1 stop bit = 10 total bits")
  {
    CHECK(Uart::calc_frame_bits(Uart::WordLength::W8, Uart::StopBits::One) ==
          10);
  }

  TEST_CASE("calc_frame_bits: W9 + 2 stop bits = 12 total bits")
  {
    CHECK(Uart::calc_frame_bits(Uart::WordLength::W9, Uart::StopBits::Two) ==
          12);
  }

  // ── enable_uart / disable_uart ────────────────────────────────────────────

  TEST_CASE_FIXTURE(
      UartBaseFixture,
      "enable_uart: enables APB1 clock and sets UE, TE, RE, RXNEIE")
  {
    REQUIRE(Uart::enable_uart(USART2, 115200, Uart::WordLength::W8,
                              Uart::StopBits::One, Uart::Parity::None));

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
    REQUIRE(Uart::enable_uart(USART2, baud, Uart::WordLength::W8,
                              Uart::StopBits::One, Uart::Parity::None));

    CHECK(USART2->BRR == SystemCoreClock / 2 / baud);
  }

  TEST_CASE_FIXTURE(UartBaseFixture,
                    "enable_uart: two stop bits configures CR2.STOP")
  {
    REQUIRE(Uart::enable_uart(USART2, 9600, Uart::WordLength::W8,
                              Uart::StopBits::Two, Uart::Parity::None));

    CHECK((USART2->CR2 & USART_CR2_STOP) ==
          (static_cast<uint32_t>(Uart::StopBits::Two) << USART_CR2_STOP_Pos));
  }

  TEST_CASE_FIXTURE(UartBaseFixture,
                    "enable_uart: even parity sets PCE, clears PS")
  {
    REQUIRE(Uart::enable_uart(USART2, 9600, Uart::WordLength::W8,
                              Uart::StopBits::One, Uart::Parity::Even));

    CHECK((USART2->CR1 & USART_CR1_PCE) != 0);
    CHECK((USART2->CR1 & USART_CR1_PS) == 0);
  }

  TEST_CASE_FIXTURE(UartBaseFixture, "enable_uart: odd parity sets PCE and PS")
  {
    REQUIRE(Uart::enable_uart(USART2, 9600, Uart::WordLength::W8,
                              Uart::StopBits::One, Uart::Parity::Odd));

    CHECK((USART2->CR1 & USART_CR1_PCE) != 0);
    CHECK((USART2->CR1 & USART_CR1_PS) != 0);
  }

  TEST_CASE_FIXTURE(UartBaseFixture,
                    "enable_uart: rejects an unknown peripheral")
  {
    const Uart::Status result =
        Uart::enable_uart(nullptr, 9600, Uart::WordLength::W8,
                          Uart::StopBits::One, Uart::Parity::None);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::InvalidInstance);
  }

  TEST_CASE_FIXTURE(UartBaseFixture, "enable_uart: rejects a zero baud rate")
  {
    const Uart::Status result =
        Uart::enable_uart(USART2, 0, Uart::WordLength::W8, Uart::StopBits::One,
                          Uart::Parity::None);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::InvalidBaudRate);
  }

  TEST_CASE_FIXTURE(UartBaseFixture,
                    "disable_uart: clears UE and disables APB1 clock")
  {
    REQUIRE(Uart::enable_uart(USART2, 115200, Uart::WordLength::W8,
                              Uart::StopBits::One, Uart::Parity::None));
    REQUIRE(Uart::disable_uart(USART2));

    CHECK((USART2->CR1 & USART_CR1_UE) == 0);
    CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) == 0);
  }

  // ── Bus::enable / disable ─────────────────────────────────────────────────

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::enable registers module and reports is_enabled")
  {
    CHECK(!bus.is_enabled());
    CHECK(bus.enable(115200));
    CHECK(bus.is_enabled());
    CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) != 0);
    CHECK((USART2->CR1 & USART_CR1_UE) != 0);
  }

  TEST_CASE_FIXTURE(UartLoopFixture, "Bus::enable is idempotent")
  {
    REQUIRE(bus.enable(115200));
    uint32_t cr1 = USART2->CR1;

    REQUIRE(bus.enable(9600)); // second call with different rate — no-op
    CHECK(USART2->CR1 == cr1);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::disable deregisters module and disables peripheral")
  {
    REQUIRE(bus.enable(115200));
    CHECK(bus.disable());
    CHECK(!bus.is_enabled());
    CHECK((USART2->CR1 & USART_CR1_UE) == 0);
    CHECK((RCC->APB1ENR & RCC_APB1ENR_USART2EN) == 0);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::enable reports module capacity exhaustion")
  {
    REQUIRE(bus.enable(115200));
    Uart::Bus<Uart::Instance::Usart1, 16, 32> second_bus(loop);

    const Uart::Status result = second_bus.enable(115200);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::ModuleCapacity);
  }

  // ── TX ────────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::write reports not enabled when the bus is disabled")
  {
    const uint8_t data[] = {0x01};
    const Uart::Status result = bus.write(data);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::NotEnabled);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::write enables TXEIE and reports is_tx_busy")
  {
    REQUIRE(bus.enable(115200));

    const uint8_t data[] = {0xAB};
    CHECK(bus.write(data));

    CHECK(bus.is_tx_busy());
    CHECK((USART2->CR1 & USART_CR1_TXEIE) != 0);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::write returns TX_BUSY while transmit is in progress")
  {
    REQUIRE(bus.enable(115200));

    const uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    REQUIRE(bus.write(data));
    const Uart::Status result = bus.write(data);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::TransmitBusy);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::write rejects data larger than owned TX storage")
  {
    REQUIRE(bus.enable(115200));
    uint8_t data[33] = {};
    const Uart::Status result = bus.write(data);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::BufferTooSmall);
    CHECK(!bus.is_tx_busy());
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::write reports scheduler capacity exhaustion")
  {
    Base::Event first(loop, Base::EventMode::Persistent,
                      {[](void *) noexcept {}, nullptr});
    Base::Event second(loop, Base::EventMode::Persistent,
                       {[](void *) noexcept {}, nullptr});
    Base::Event third(loop, Base::EventMode::Persistent,
                      {[](void *) noexcept {}, nullptr});
    Base::Event fourth(loop, Base::EventMode::Persistent,
                       {[](void *) noexcept {}, nullptr});
    REQUIRE(first.enable(std::chrono::seconds{1}));
    REQUIRE(second.enable(std::chrono::seconds{1}));
    REQUIRE(third.enable(std::chrono::seconds{1}));
    REQUIRE(fourth.enable(std::chrono::seconds{1}));
    REQUIRE(bus.enable(115200));

    const uint8_t data[] = {0x01};
    const Uart::Status result = bus.write(data);
    REQUIRE(!result);
    CHECK(result.error() == Uart::Error::Schedule);
    CHECK(!bus.is_tx_busy());
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus::write owns data for the asynchronous transfer")
  {
    REQUIRE(bus.enable(115200));
    uint8_t data[] = {0x12, 0x34};
    REQUIRE(bus.write(data));
    data[0] = 0xFF;
    data[1] = 0xFF;

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    REQUIRE(!Sim::Uart::tx_buffers.empty());
    CHECK(Sim::Uart::tx_buffers.back() == std::vector<uint8_t>({0x12, 0x34}));
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: transmitted bytes appear in sim tx_buffers, "
                    "callback receives a successful result")
  {
    REQUIRE(bus.enable(115200));

    TransmitResult tx_result;
    bus.set_tx_callback({TransmitResult::record, &tx_result});

    const uint8_t data[] = {0xDE, 0xAD};
    REQUIRE(bus.write(data));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(tx_result.called);
    CHECK(tx_result.succeeded);
    CHECK(!bus.is_tx_busy());
    REQUIRE(!Sim::Uart::tx_buffers.empty());
    CHECK(Sim::Uart::tx_buffers.back() == std::vector<uint8_t>({0xDE, 0xAD}));
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: an empty write completes asynchronously")
  {
    REQUIRE(bus.enable(115200));

    TransmitResult tx_result;
    bus.set_tx_callback({TransmitResult::record, &tx_result});
    REQUIRE(bus.write(std::span<const uint8_t>{}));
    CHECK(bus.is_tx_busy());

    REQUIRE(loop.stop(std::chrono::microseconds{100}));
    loop.run();

    CHECK(tx_result.called);
    CHECK(tx_result.succeeded);
    CHECK(!bus.is_tx_busy());
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: a deferred transmit timeout reaches the callback")
  {
    REQUIRE(bus.enable(115200));

    TransmitResult tx_result;
    bus.set_tx_callback({TransmitResult::record, &tx_result});
    const uint8_t data[] = {0x01};
    REQUIRE(bus.write(data));
    Sim::USART2_IRQHandler_ptr = nullptr;

    REQUIRE(loop.stop(std::chrono::microseconds{100}));
    loop.run();

    CHECK(tx_result.called);
    CHECK(!tx_result.succeeded);
    CHECK(tx_result.error == Uart::Error::TransmitTimeout);
    CHECK(!bus.is_tx_busy());
  }

  // ── RX ────────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: single received byte delivered to RX callback")
  {
    REQUIRE(bus.enable(115200));

    ReceivedBytes received;
    bus.set_rx_callback({ReceivedBytes::record, &received});

    Sim::Uart::simulate_rx({'A'});

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    REQUIRE(received.size == 1);
    CHECK(received.values[0] == 'A');
    CHECK(received.error_count == 0);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: multiple received bytes delivered in order")
  {
    REQUIRE(bus.enable(115200));

    ReceivedBytes received;
    bus.set_rx_callback({ReceivedBytes::record, &received});

    Sim::Uart::simulate_rx({0x11, 0x22, 0x33});

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    REQUIRE(received.size == 3);
    CHECK(received.values[0] == 0x11);
    CHECK(received.values[1] == 0x22);
    CHECK(received.values[2] == 0x33);
    CHECK(received.error_count == 0);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: byte 0x18 is delivered as data, not an error")
  {
    REQUIRE(bus.enable(115200));

    ReceivedBytes received;
    bus.set_rx_callback({ReceivedBytes::record, &received});
    Sim::Uart::simulate_rx({0x18});

    REQUIRE(loop.stop(std::chrono::microseconds{100}));
    loop.run();

    REQUIRE(received.size == 1);
    CHECK(received.values[0] == 0x18);
    CHECK(received.error_count == 0);
  }

  TEST_CASE_FIXTURE(UartLoopFixture,
                    "Bus: RX storage overflow is delivered as a typed error")
  {
    REQUIRE(bus.enable(115200));

    ReceivedBytes received;
    bus.set_rx_callback({ReceivedBytes::record, &received});
    for (size_t index = 0; index < 17; ++index)
    {
      USART2->DR = 0xA5;
      USART2->SR = USART2->SR | USART_SR_RXNE;
      bus.handle_irq();
    }

    REQUIRE(loop.stop(std::chrono::microseconds{100}));
    loop.run();

    CHECK(received.size == 16);
    REQUIRE(received.error_count == 1);
    CHECK(received.errors[0] == Uart::Error::ReceiveOverflow);
  }

  // ── REDE pin ──────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(UartRedeFixture,
                    "Bus: REDE pin driven high on write, low after TC")
  {
    REQUIRE(bus.enable(115200));
    bus.set_rede_pin(&rede);

    TransmitResult tx_result;
    bus.set_tx_callback({TransmitResult::record, &tx_result});

    const uint8_t data[] = {0xCC};
    GPIOA->BSRR = 0;
    REQUIRE(bus.write(data));

    // REDE must be asserted immediately when write() is called
    CHECK((GPIOA->BSRR & (1u << 5)) != 0);

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    // REDE must be de-asserted after the TC interrupt fires
    CHECK(tx_result.called);
    CHECK(tx_result.succeeded);
    CHECK((GPIOA->BSRR & (1u << (5 + 16))) != 0);
  }

} // TEST_SUITE("uart")
