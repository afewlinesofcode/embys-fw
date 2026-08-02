#include <sstream>
#include <vector>

#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

#define TEST_HOOK(key) Embys::Stm32::Sim::Base::trigger_test_hook(key)

namespace Sim = Embys::Stm32::Sim;

// ── fixture
// ───────────────────────────────────────────────────────────────────

struct SimI2cFixture
{
  SimI2cFixture()
  {
    Sim::reset();
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_I2C1EN);
    SET_BIT_V(I2C1->CR1, I2C_CR1_PE);
  }

  // Register-access helpers — each triggers the matching sim test hook.

  uint32_t
  read_sr1()
  {
    auto v = I2C1->SR1;
    TEST_HOOK("i2c_read_sr1");
    return v;
  }

  uint32_t
  read_sr2()
  {
    auto v = I2C1->SR2;
    TEST_HOOK("i2c_read_sr2");
    return v;
  }

  uint8_t
  read_dr()
  {
    auto v = static_cast<uint8_t>(I2C1->DR);
    TEST_HOOK("i2c_read_dr");
    return v;
  }

  void
  write_dr(uint8_t val)
  {
    I2C1->DR = val;
    TEST_HOOK("i2c_write_dr");
  }

  // Poll SR1 until the given flag is set, cycling the simulation each
  // iteration.
  bool
  wait_sr1(uint32_t flag, int max = 200)
  {
    for (int i = 0; i < max; ++i)
    {
      if (I2C1->SR1 & flag)
        return true;
      __NOP();
    }
    return false;
  }

  // Poll SR2 until the given flag is cleared.
  bool
  wait_sr2_clear(uint32_t flag, int max = 200)
  {
    for (int i = 0; i < max; ++i)
    {
      if (!(I2C1->SR2 & flag))
        return true;
      __NOP();
    }
    return false;
  }

  // Drive the START + address phase for a write direction.
  // After return: condition = Writing, TXE = 1, tx_buffers.back() is empty.
  bool
  start_write(uint8_t addr7)
  {
    SET_BIT_V(I2C1->CR1, I2C_CR1_START);
    if (!wait_sr1(I2C_SR1_SB))
      return false;
    (void)read_sr1(); // signals sim: SB about to be cleared
    write_dr(static_cast<uint8_t>(addr7 << 1u)); // write bit = 0
    if (!wait_sr1(I2C_SR1_ADDR))
      return false;
    (void)read_sr1(); // signals sim: ADDR about to be cleared
    (void)read_sr2(); // clears ADDR, condition → Writing
    return true;
  }

  // Drive the (repeated) START + address phase for a read direction.
  // After return: condition = Reading, rx reception underway.
  bool
  start_read(uint8_t addr7)
  {
    SET_BIT_V(I2C1->CR1, I2C_CR1_START);
    if (!wait_sr1(I2C_SR1_SB))
      return false;
    (void)read_sr1();
    write_dr(static_cast<uint8_t>((addr7 << 1u) | 1u)); // read bit = 1
    if (!wait_sr1(I2C_SR1_ADDR))
      return false;
    (void)read_sr1();
    (void)read_sr2(); // clears ADDR, condition → Reading
    return true;
  }
};

// ── tests
// ─────────────────────────────────────────────────────────────────────

TEST_SUITE("sim_i2c")
{

  TEST_CASE_FIXTURE(SimI2cFixture,
                    "START condition sets SB flag after a short delay")
  {
    SET_BIT_V(I2C1->CR1, I2C_CR1_START);

    // SB must not be set synchronously — the sim models the hardware delay.
    CHECK((I2C1->SR1 & I2C_SR1_SB) == 0);

    // After a few cycles SB is raised and the START bit is cleared by the sim.
    for (int i = 0; i < 20; ++i)
      __NOP();

    CHECK((I2C1->SR1 & I2C_SR1_SB) != 0);
    CHECK((I2C1->CR1 & I2C_CR1_START) == 0);
  }

  TEST_CASE_FIXTURE(
      SimI2cFixture,
      "Write transaction: on_tx receives the correct address and bytes")
  {
    struct Cap
    {
      uint8_t addr = 0;
      std::vector<uint8_t> data;
      bool called = false;
    } cap;

    Sim::I2C::on_tx = {[](void *ctx, uint8_t a, std::vector<uint8_t> d) noexcept
                       {
                         auto *c = static_cast<Cap *>(ctx);
                         c->addr = a;
                         c->data = std::move(d);
                         c->called = true;
                       },
                       &cap};

    CHECK(start_write(0x50u));

    // Write two data bytes, waiting for TXE (DR empty) before each.
    CHECK(wait_sr1(I2C_SR1_TXE));
    write_dr(0xDEu);
    CHECK(wait_sr1(I2C_SR1_TXE));
    write_dr(0xADu);

    // Wait for BTF: both bytes have moved through the shift register.
    CHECK(wait_sr1(I2C_SR1_BTF));
    SET_BIT_V(I2C1->CR1, I2C_CR1_STOP);

    for (int i = 0; i < 100 && !cap.called; ++i)
      __NOP();

    CHECK(cap.called);
    CHECK(cap.addr == 0x50u);
    CHECK(cap.data == std::vector<uint8_t>{0xDEu, 0xADu});
  }

  TEST_CASE_FIXTURE(
      SimI2cFixture,
      "Read transaction: simulate_rx pre-loads bytes that arrive in order")
  {
    // Pre-load three bytes.  The 3-byte read uses the standard N-3 BTF path:
    // wait for BTF (bytes 0 and 1 buffered), then NACK + read byte 0, STOP,
    // read byte 1, read byte 2.
    Sim::I2C::simulate_rx({0xA1u, 0xB2u, 0xC3u});
    SET_BIT_V(I2C1->CR1, I2C_CR1_ACK);

    CHECK(start_read(0x50u));

    // BTF fires when byte 0 is in DR and byte 1 is in the shift register.
    CHECK(wait_sr1(I2C_SR1_BTF));

    // Disable ACK so the slave is NACKed after byte 1 is read.
    CLEAR_BIT_V(I2C1->CR1, I2C_CR1_ACK);

    std::vector<uint8_t> received;

    // Read byte 0 — this also moves byte 1 into DR and starts loading byte 2
    // into the shift register.
    received.push_back(read_dr());
    SET_BIT_V(I2C1->CR1, I2C_CR1_STOP);

    CHECK(wait_sr1(I2C_SR1_RXNE));
    received.push_back(read_dr()); // byte 1

    CHECK(wait_sr1(I2C_SR1_RXNE));
    received.push_back(read_dr()); // byte 2

    CHECK(received == std::vector<uint8_t>{0xA1u, 0xB2u, 0xC3u});
  }

  TEST_CASE_FIXTURE(SimI2cFixture, "simulate_response: plain read (reg = 0) "
                                   "auto-injects data when the read starts")
  {
    Sim::I2C::simulate_response(0x38u, 0x00u, {0x08u});

    // Disable ACK: only one byte expected.
    CLEAR_BIT_V(I2C1->CR1, I2C_CR1_ACK);

    // start_read triggers read_sr2_hook which detects reg = 0, finds the
    // registered response, and calls simulate_rx({0x08}) automatically.
    CHECK(start_read(0x38u));

    SET_BIT_V(I2C1->CR1, I2C_CR1_STOP);
    CHECK(wait_sr1(I2C_SR1_RXNE));

    CHECK(read_dr() == 0x08u);
  }

  TEST_CASE_FIXTURE(SimI2cFixture, "simulate_response: register read "
                                   "auto-injects data after a repeated-START")
  {
    // Register a response for addr=0x38, reg=0x71.
    Sim::I2C::simulate_response(0x38u, 0x71u, {0xCAu});

    // Disable ACK: only one byte will be read back.
    CLEAR_BIT_V(I2C1->CR1, I2C_CR1_ACK);

    // Write phase: send register address 0x71.
    CHECK(start_write(0x38u));
    CHECK(wait_sr1(I2C_SR1_TXE));
    write_dr(0x71u);
    CHECK(wait_sr1(I2C_SR1_BTF)); // register byte fully transmitted

    // Repeated START switches to read direction.  read_sr2_hook sees that the
    // last write contained reg = 0x71 and auto-injects the registered response.
    CHECK(start_read(0x38u));

    SET_BIT_V(I2C1->CR1, I2C_CR1_STOP);
    CHECK(wait_sr1(I2C_SR1_RXNE));

    CHECK(read_dr() == 0xCAu);
  }

  TEST_CASE_FIXTURE(
      SimI2cFixture,
      "simulate_busy: a fresh START never completes when the bus is busy")
  {
    Sim::I2C::simulate_busy();

    // Redirect cout so the expected diagnostic messages don't pollute the test
    // output, and verify that the sim emits the correct log line.
    std::ostringstream captured;
    std::streambuf *old_buf = std::cout.rdbuf(captured.rdbuf());

    SET_BIT_V(I2C1->CR1, I2C_CR1_START);
    for (int i = 0; i < 100; ++i)
      __NOP();

    std::cout.rdbuf(old_buf);

    // The sim ignores START while BUSY is set, so SB is never raised.
    CHECK((I2C1->SR1 & I2C_SR1_SB) == 0);

    // Every line emitted while BUSY is set must be exactly the expected
    // diagnostic — no unexpected output, and at least one line present.
    static constexpr std::string_view expected =
        "start_hook: Already busy, ignoring START condition";
    std::istringstream lines(captured.str());
    std::string line;
    int count = 0;
    while (std::getline(lines, line))
    {
      CHECK(line == expected);
      ++count;
    }
    CHECK(count > 0);
  }

  TEST_CASE_FIXTURE(
      SimI2cFixture,
      "BUSY and MSL flags are cleared after the write STOP condition")
  {
    Sim::I2C::on_tx = {[](void *, uint8_t, std::vector<uint8_t>) noexcept {},
                       nullptr};

    CHECK(start_write(0x27u));
    CHECK(wait_sr1(I2C_SR1_TXE));
    write_dr(0xFFu);
    CHECK(wait_sr1(I2C_SR1_BTF));

    SET_BIT_V(I2C1->CR1, I2C_CR1_STOP);

    // The sim clears BUSY and MSL a few cycles after STOP.
    CHECK(wait_sr2_clear(I2C_SR2_BUSY));
    CHECK((I2C1->SR2 & I2C_SR2_MSL) == 0);
  }

} // TEST_SUITE("sim_i2c")
