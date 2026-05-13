/**
 * @file can.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief CAN simulation for the STM32 simulated environment.
 * @version 0.1
 * @date 2026-05-11
 * @copyright Copyright (c) 2026
 */

#include "can.hpp"

#include <queue>

#include "base.hpp"

namespace Embys::Stm32::Sim::CAN
{

CAN_TypeDef *can = &can1_instance; // Default to can1

std::vector<Frame> tx_frames;

Callable<Frame> on_tx;

/**
 * @brief Number of cycles between TXRQ detection and setting TME/RQCP/TXOK,
 * simulating bus transmission time.
 */
static constexpr uint32_t TX_DELAY = 20;

/**
 * @brief Per-FIFO receive queues (FIFO 0 and FIFO 1).
 */
static std::queue<Frame> rx_fifo[2];

/**
 * @brief Load the front frame of @p fifo_idx into the peripheral's FIFO
 * mailbox registers and update FMP / FULL flags.
 */
static void
load_fifo_mailbox(uint8_t fifo_idx)
{
  if (rx_fifo[fifo_idx].empty())
    return;

  const Frame &f = rx_fifo[fifo_idx].front();
  CAN_FIFOMailBox_TypeDef &mb = can->sFIFOMailBox[fifo_idx];

  // Build RIR from local accumulator to avoid compound assignment on volatile
  uint32_t rir = 0;

  if (f.ide)
  {
    rir |= (f.id << CAN_RI0R_EXID_Pos) & CAN_RI0R_EXID_Msk;
    rir |= CAN_RI0R_IDE_Msk;
  }
  else
  {
    rir |= (f.id << CAN_RI0R_STID_Pos) & CAN_RI0R_STID_Msk;
  }

  if (f.rtr)
    rir |= CAN_RI0R_RTR_Msk;

  mb.RIR = rir;

  // DLC
  mb.RDTR = (f.dlc & 0xFU);

  // Data bytes
  mb.RDLR = (static_cast<uint32_t>(f.data[0])) |
            (static_cast<uint32_t>(f.data[1]) << 8) |
            (static_cast<uint32_t>(f.data[2]) << 16) |
            (static_cast<uint32_t>(f.data[3]) << 24);

  mb.RDHR = (static_cast<uint32_t>(f.data[4])) |
            (static_cast<uint32_t>(f.data[5]) << 8) |
            (static_cast<uint32_t>(f.data[6]) << 16) |
            (static_cast<uint32_t>(f.data[7]) << 24);

  // Update FMP and FULL
  const uint32_t fmp = static_cast<uint32_t>(rx_fifo[fifo_idx].size());

  if (fifo_idx == 0)
  {
    can->RF0R = (can->RF0R & ~CAN_RF0R_FMP0_Msk) | (fmp & 0x3U);

    if (fmp >= 3)
      SET_BIT_V(can->RF0R, CAN_RF0R_FULL0_Msk);
    else
      CLEAR_BIT_V(can->RF0R, CAN_RF0R_FULL0_Msk);
  }
  else
  {
    can->RF1R = (can->RF1R & ~CAN_RF1R_FMP1_Msk) | (fmp & 0x3U);

    if (fmp >= 3)
      SET_BIT_V(can->RF1R, CAN_RF1R_FULL1_Msk);
    else
      CLEAR_BIT_V(can->RF1R, CAN_RF1R_FULL1_Msk);
  }
}

void
simulate_rx(Frame frame, uint8_t fifo)
{
  if (fifo > 1)
    return;

  const bool was_empty = rx_fifo[fifo].empty();
  rx_fifo[fifo].push(frame);

  // If this is the first frame, load it immediately into the mailbox registers
  if (was_empty)
    load_fifo_mailbox(fifo);
  else
  {
    // Just update FMP count
    const uint32_t fmp = static_cast<uint32_t>(rx_fifo[fifo].size());

    if (fifo == 0)
    {
      can->RF0R = (can->RF0R & ~CAN_RF0R_FMP0_Msk) | (fmp & 0x3U);

      if (fmp >= 3)
        SET_BIT_V(can->RF0R, CAN_RF0R_FULL0_Msk);
    }
    else
    {
      can->RF1R = (can->RF1R & ~CAN_RF1R_FMP1_Msk) | (fmp & 0x3U);

      if (fmp >= 3)
        SET_BIT_V(can->RF1R, CAN_RF1R_FULL1_Msk);
    }
  }
}

/**
 * @brief Read a Frame out of a TX mailbox register set.
 * @param mb Reference to the mailbox.
 * @return The decoded Frame.
 */
static Frame
read_tx_mailbox(const CAN_TxMailBox_TypeDef &mb)
{
  Frame f = {};

  const bool ide = (mb.TIR & CAN_TI0R_IDE_Msk) != 0;

  f.ide = ide;
  f.rtr = (mb.TIR & CAN_TI0R_RTR_Msk) != 0;

  if (ide)
    f.id = (mb.TIR & CAN_TI0R_EXID_Msk) >> CAN_TI0R_EXID_Pos;
  else
    f.id = (mb.TIR & CAN_TI0R_STID_Msk) >> CAN_TI0R_STID_Pos;

  f.dlc = static_cast<uint8_t>(mb.TDTR & CAN_TDT0R_DLC_Msk);

  f.data[0] = static_cast<uint8_t>(mb.TDLR & 0xFFU);
  f.data[1] = static_cast<uint8_t>((mb.TDLR >> 8) & 0xFFU);
  f.data[2] = static_cast<uint8_t>((mb.TDLR >> 16) & 0xFFU);
  f.data[3] = static_cast<uint8_t>((mb.TDLR >> 24) & 0xFFU);
  f.data[4] = static_cast<uint8_t>(mb.TDHR & 0xFFU);
  f.data[5] = static_cast<uint8_t>((mb.TDHR >> 8) & 0xFFU);
  f.data[6] = static_cast<uint8_t>((mb.TDHR >> 16) & 0xFFU);
  f.data[7] = static_cast<uint8_t>((mb.TDHR >> 24) & 0xFFU);

  return f;
}

// Per-mailbox TX pending state: stores the cycle at which transmission should
// complete, or 0 if no transmission is pending.
static uint32_t tx_complete_cyc[3] = {};

// TXRQ bits per mailbox, indexed 0-2
static constexpr uint32_t TXRQ_BITS[3] = {
    CAN_TI0R_TXRQ_Msk,
    CAN_TI0R_TXRQ_Msk, // same bit position in every TIR
    CAN_TI0R_TXRQ_Msk,
};

// Per-mailbox TSR bits
struct MailboxTsrBits
{
  uint32_t tme;
  uint32_t rqcp;
  uint32_t txok;
};

static constexpr MailboxTsrBits TSR_BITS[3] = {
    {CAN_TSR_TME0_Msk, CAN_TSR_RQCP0_Msk, CAN_TSR_TXOK0_Msk},
    {CAN_TSR_TME1_Msk, CAN_TSR_RQCP1_Msk, CAN_TSR_TXOK1_Msk},
    {CAN_TSR_TME2_Msk, CAN_TSR_RQCP2_Msk, CAN_TSR_TXOK2_Msk},
};

/**
 * @brief Persistent hook called every cycle.
 * Handles init/sleep mode acknowledgement, TX TXRQ detection and completion,
 * and RFOM (Release FIFO Output Mailbox) handling.
 */
void
peripheral_hook(uint32_t cyc)
{
  // Init mode handshake: MCR_INRQ set → acknowledge with MSR_INAK
  if (can->MCR & CAN_MCR_INRQ_Msk)
    SET_BIT_V(can->MSR, CAN_MSR_INAK_Msk);
  else
    CLEAR_BIT_V(can->MSR, CAN_MSR_INAK_Msk);

  // Sleep mode handshake: MCR_SLEEP set → acknowledge with MSR_SLAK
  if (can->MCR & CAN_MCR_SLEEP_Msk)
    SET_BIT_V(can->MSR, CAN_MSR_SLAK_Msk);
  else
    CLEAR_BIT_V(can->MSR, CAN_MSR_SLAK_Msk);

  // TX: scan mailboxes for TXRQ
  for (uint8_t i = 0; i < 3; ++i)
  {
    CAN_TxMailBox_TypeDef &mb = can->sTxMailBox[i];

    if ((mb.TIR & TXRQ_BITS[i]) && tx_complete_cyc[i] == 0)
    {
      // Schedule completion after TX_DELAY cycles
      tx_complete_cyc[i] = cyc + TX_DELAY;

      // Mark mailbox as busy (TME cleared)
      CLEAR_BIT_V(can->TSR, TSR_BITS[i].tme);
    }

    if (tx_complete_cyc[i] != 0 && cyc >= tx_complete_cyc[i])
    {
      tx_complete_cyc[i] = 0;

      Frame f = read_tx_mailbox(mb);
      tx_frames.push_back(f);

      // Clear TXRQ, set RQCP and TXOK, mark mailbox empty
      CLEAR_BIT_V(mb.TIR, TXRQ_BITS[i]);
      SET_BIT_V(can->TSR,
                TSR_BITS[i].rqcp | TSR_BITS[i].txok | TSR_BITS[i].tme);

      on_tx(f);
    }
  }

  // RX FIFO 0: release output mailbox when RFOM0 is set by firmware
  if (can->RF0R & CAN_RF0R_RFOM0_Msk)
  {
    CLEAR_BIT_V(can->RF0R, CAN_RF0R_RFOM0_Msk);

    if (!rx_fifo[0].empty())
    {
      rx_fifo[0].pop();

      if (rx_fifo[0].empty())
      {
        can->RF0R = 0;
      }
      else
      {
        load_fifo_mailbox(0);
      }
    }
  }

  // RX FIFO 1: release output mailbox when RFOM1 is set by firmware
  if (can->RF1R & CAN_RF1R_RFOM1_Msk)
  {
    CLEAR_BIT_V(can->RF1R, CAN_RF1R_RFOM1_Msk);

    if (!rx_fifo[1].empty())
    {
      rx_fifo[1].pop();

      if (rx_fifo[1].empty())
      {
        can->RF1R = 0;
      }
      else
      {
        load_fifo_mailbox(1);
      }
    }
  }
}

void
reset()
{
  can = &can1_instance;

  tx_frames.clear();
  on_tx.clear();

  rx_fifo[0] = {};
  rx_fifo[1] = {};

  tx_complete_cyc[0] = 0;
  tx_complete_cyc[1] = 0;
  tx_complete_cyc[2] = 0;

  // All TX mailboxes start empty; CAN starts in sleep mode (after reset,
  // hardware sets SLEEP and SLAK by default).
  can->MCR = CAN_MCR_SLEEP_Msk;
  can->MSR = CAN_MSR_SLAK_Msk;
  can->TSR = CAN_TSR_TME0_Msk | CAN_TSR_TME1_Msk | CAN_TSR_TME2_Msk;
  can->RF0R = 0;
  can->RF1R = 0;

  Base::add_hook(peripheral_hook);
}

} // namespace Embys::Stm32::Sim::CAN
