/**
 * @file can.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief CAN simulation for the STM32 mock environment.
 *
 * No test hooks are required from firmware code — the simulation detects
 * hardware-level events by monitoring register changes in the persistent
 * hook:
 * - TX: firmware sets CAN_TI0R_TXRQ in a mailbox TIR → frame is captured,
 *   mailbox is marked empty (TMEn), RQCP/TXOK bits are set in TSR, and
 *   on_tx is invoked.
 * - RX: call simulate_rx() from test code to inject a CAN frame into FIFO 0
 *   or FIFO 1; the FMP count is updated and the FMPIE interrupt fires.
 * - Init/Sleep mode: the simulation acknowledges MCR_INRQ with MSR_INAK,
 *   and MCR_SLEEP with MSR_SLAK, on the next cycle.
 * - RFOM: firmware sets RF0R_RFOM0 or RF1R_RFOM1 to release the current
 *   FIFO slot; the simulation dequeues the front entry and clears the bit.
 *
 * @version 0.1
 * @date 2026-05-11
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <array>
#include <vector>

#include <embys/stm32/types.hpp>

#include "core.hpp"

namespace Embys::Stm32::Sim::CAN
{

/**
 * @brief A CAN bus frame.
 */
struct Frame
{
  uint32_t id;     ///< 11-bit (standard) or 29-bit (extended) identifier
  bool ide;        ///< true = extended frame (29-bit ID)
  bool rtr;        ///< true = remote transmission request
  uint8_t dlc;     ///< data length code (0–8)
  uint8_t data[8]; ///< payload bytes (only [0..dlc-1] are meaningful)
};

/**
 * @brief Pointer to the CAN peripheral instance used in the mock environment.
 */
extern CAN_TypeDef *can;

/**
 * @brief Frames transmitted by the firmware (captured from TX mailboxes).
 * Appended each time a mailbox TXRQ is detected and the frame is processed.
 */
extern std::vector<Frame> tx_frames;

/**
 * @brief Callback invoked when the firmware successfully transmits a CAN
 * frame. Receives a copy of the transmitted Frame.
 */
extern Callable<Frame> on_tx;

/**
 * @brief Inject a CAN frame into a receive FIFO.
 * The frame will be placed at the back of the specified FIFO queue.
 * A FMPIE interrupt will fire on the next cycle if enabled.
 * @param frame The CAN frame to inject.
 * @param fifo  The receive FIFO to inject into (0 or 1). Defaults to 0.
 */
void
simulate_rx(Frame frame, uint8_t fifo = 0);

/**
 * @brief Reset the CAN simulation state and re-register all hooks.
 * Resets the CAN pointer to can1.
 */
void
reset();

} // namespace Embys::Stm32::Sim::CAN
