#include <vector>

#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

#define TEST_HOOK(key) Embys::Stm32::Sim::Base::trigger_test_hook(key)

namespace Sim = Embys::Stm32::Sim;

struct SimUartFixture
{
  SimUartFixture()
  {
    Sim::reset();
  }

  uint8_t
  read_dr()
  {
    auto dr = USART1->DR;
    TEST_HOOK("uart_read_dr");
    return dr;
  }
};

TEST_CASE_FIXTURE(SimUartFixture, "Basic UART reception simulation")
{
  // Buffer to store received data
  std::vector<uint8_t> recv_buffer;

  // Expected data to be received
  std::vector<uint8_t> expected_data = {'H', 'e', 'l', 'l', 'o'};

  // Count iterations waiting for RXNE flag to verify it gets stuck after
  // receiving all data
  int rxne_iterations;

  // Initialize GPIO bus and TX (PA9), RX(PA10) pins for USART1
  // Enable GPIOA clock
  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);
  // Configure PA9 as alternate function push-pull (TX)
  CLEAR_BIT_V(GPIOA->CRH, GPIO_CRH_MODE9 | GPIO_CRH_CNF9);
  SET_BIT_V(GPIOA->CRH, GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1);
  // Configure PA10 as input floating (RX)
  CLEAR_BIT_V(GPIOA->CRH, GPIO_CRH_MODE10 | GPIO_CRH_CNF10);
  SET_BIT_V(GPIOA->CRH, GPIO_CRH_CNF10_0);

  // Enable USART1 clock
  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
  // Configure USART1 for 9600 baud, 8N1
  USART1->BRR = 72000000 / 9600;             // Assuming 72 MHz clock
  USART1->CR1 = USART_CR1_UE | USART_CR1_RE; // Enable USART and receiver

  // Everything is set up, now simulate receiving data on the UART
  Sim::Uart::simulate_rx(expected_data);

  while (1)
  {
    rxne_iterations = 0;

    // Wait for RXNE flag to be set, indicating data is ready to read
    while ((USART1->SR & USART_SR_RXNE) == 0)
    {
      __NOP(); // This calls cycle simulation

      if (++rxne_iterations > 1000)
        goto assert; // terminate the test
    }

    // Read the received byte in DR using the read_dr() function which triggers
    // the test hook
    recv_buffer.push_back(read_dr());
  }

assert:
  // All bytes received
  CHECK(recv_buffer.size() == 5);
  // All bytes are valid
  CHECK(recv_buffer == expected_data);
  // After the last byte was received, polling got stuck as expected
  CHECK(rxne_iterations == 1001);
}
