/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus protocol constants: function codes, exception codes, frame size
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

namespace Embys::Stm32::Modbus
{

struct FunctionCode
{
  static constexpr uint8_t ReadCoils = 0x01;
  static constexpr uint8_t ReadDiscreteInputs = 0x02;
  static constexpr uint8_t ReadHoldingRegisters = 0x03;
  static constexpr uint8_t ReadInputRegisters = 0x04;
  static constexpr uint8_t WriteSingleCoil = 0x05;
  static constexpr uint8_t WriteSingleRegister = 0x06;
  static constexpr uint8_t Diagnostics = 0x08;
  static constexpr uint8_t ReportServerId = 0x11;
  static constexpr uint8_t WriteMultipleCoils = 0x0F;
  static constexpr uint8_t WriteMultipleRegisters = 0x10;
};

struct DiagnosticsSubCode
{
  static constexpr uint16_t ReturnQueryData = 0x0000;
  static constexpr uint16_t ClearCounters = 0x000A;
  static constexpr uint16_t ReturnBusMessageCount = 0x000B;
  static constexpr uint16_t ReturnBusCommErrorCount = 0x000C;
  static constexpr uint16_t ReturnBusExceptionErrorCount = 0x000D;
  static constexpr uint16_t ReturnSlaveMessageCount = 0x000E;
  static constexpr uint16_t ReturnSlaveNoResponseCount = 0x000F;
  static constexpr uint16_t ReturnSlaveBusyCount = 0x0011;
};

struct ExceptionCode
{
  static constexpr uint8_t IllegalFunction = 0x01;
  static constexpr uint8_t IllegalDataAddress = 0x02;
  static constexpr uint8_t IllegalDataValue = 0x03;
  static constexpr uint8_t ServerDeviceFailure = 0x04;
  static constexpr uint8_t Acknowledge = 0x05;
  static constexpr uint8_t ServerDeviceBusy = 0x06;
  static constexpr uint8_t MemoryParityError = 0x08;
  static constexpr uint8_t GatewayPathUnavailable = 0x0A;
  static constexpr uint8_t GatewayTargetDeviceFailedToRespond = 0x0B;
};

// Bytes in a Modbus frame header (device ID + function code)
static constexpr uint16_t kFrameHeaderSize = 2;

// Maximum Modbus frame size (per spec: 256 bytes)
static constexpr uint16_t kFrameSize = 256;

}; // namespace Embys::Stm32::Modbus
