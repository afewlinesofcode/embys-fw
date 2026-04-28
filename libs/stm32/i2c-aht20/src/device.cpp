#include "device.hpp"

namespace Embys::Stm32::I2c::Dev::Aht20
{

static constexpr uint8_t AHT20_ADDR = 0x38u;

Device::Device(Base::Loop *loop, I2c::Bus *bus)
  : timeout(loop), write(bus), read(bus)
{
}

void
Device::enable(Cb cb)
{
  enable_cb = cb;
  power_up();
}

void
Device::query(QueryCb cb)
{
  this->cb = cb;

  if (!initialized)
  {
    response(NOT_INITIALIZED);
    return;
  }

  request_query();
}

void
Device::power_up()
{
  stage = PowerUp;
  timeout.exec(40000u, {command_callback, this});
}

void
Device::read_calibration_status()
{
  stage = ReadCalibration;
  read.exec(AHT20_ADDR, 0x71u, buffer, 1, {command_callback, this});
}

void
Device::handle_calibration_status()
{
  if (buffer[0] & 0x08u)
  {
    initialized = true;
    enable_cb(0);
    return;
  }

  send_initialization();
}

void
Device::send_initialization()
{
  stage = SendInitialization;
  static const uint8_t INIT_CMD[] = {0xE1u, 0x08u, 0x00u};
  write.exec(AHT20_ADDR, INIT_CMD, sizeof(INIT_CMD), {command_callback, this});
}

void
Device::wait_initialization()
{
  stage = WaitInitialization;
  timeout.exec(10000u, {command_callback, this});
}

void
Device::finish_initialization()
{
  initialized = true;
  enable_cb(0);
}

void
Device::request_query()
{
  stage = RequestQuery;
  static const uint8_t QUERY_CMD[] = {0xACu, 0x33u, 0x00u};
  write.exec(AHT20_ADDR, QUERY_CMD, sizeof(QUERY_CMD),
             {command_callback, this});
}

void
Device::wait_request_query()
{
  stage = WaitQuery;
  timeout.exec(80000u, {command_callback, this});
}

void
Device::read_query()
{
  stage = ReadQuery;
  read.exec(AHT20_ADDR, buffer, 7u, {command_callback, this});
}

void
Device::parse_query()
{
  if (buffer[0] & 0x80u)
  {
    response(NOT_READY);
    return;
  }

  if (!check_crc(buffer))
  {
    response(CRC_ERROR);
    return;
  }

  uint32_t raw_humidity = (static_cast<uint32_t>(buffer[1]) << 12) |
                          (static_cast<uint32_t>(buffer[2]) << 4) |
                          ((buffer[3] >> 4) & 0x0Fu);
  uint32_t raw_temperature = (static_cast<uint32_t>(buffer[3] & 0x0Fu) << 16) |
                             (static_cast<uint32_t>(buffer[4]) << 8) |
                             buffer[5];

  values.humidity = static_cast<float>(raw_humidity) * 100.0f / 1048576.0f;
  values.temperature =
      static_cast<float>(raw_temperature) * 200.0f / 1048576.0f - 50.0f;

  response(0);
}

void
Device::response(int rc)
{
  cb(rc, &values);
  cb.clear();
}

bool
Device::check_crc(const uint8_t *buf)
{
  uint8_t expected_crc = buf[6];
  uint8_t crc = 0xFFu;
  static constexpr uint8_t POLY = 0x31u;

  for (uint8_t i = 0; i < 6; i++)
  {
    crc ^= buf[i];
    for (int b = 0; b < 8; b++)
    {
      if (crc & 0x80u)
        crc = static_cast<uint8_t>((crc << 1) ^ POLY);
      else
        crc = static_cast<uint8_t>(crc << 1);
    }
  }

  return crc == expected_crc;
}

void
Device::command_callback(void *ctx, int result)
{
  auto aht20 = static_cast<Device *>(ctx);

  if (result != 0)
  {
    aht20->response(result);
    return;
  }

  switch (aht20->stage)
  {
    case PowerUp:
      aht20->read_calibration_status();
      break;
    case ReadCalibration:
      aht20->handle_calibration_status();
      break;
    case SendInitialization:
      aht20->wait_initialization();
      break;
    case WaitInitialization:
      aht20->finish_initialization();
      break;
    case RequestQuery:
      aht20->wait_request_query();
      break;
    case WaitQuery:
      aht20->read_query();
      break;
    case ReadQuery:
      aht20->parse_query();
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Aht20
