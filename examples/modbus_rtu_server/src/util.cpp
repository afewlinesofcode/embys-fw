#include "util.hpp"

namespace ModbusRtuServer
{

// ── helpers ────────────────────────────────────────────────────────────────

static uint8_t
write_str(char *buf, const char *str)
{
  uint8_t i = 0;
  while (str[i])
  {
    buf[i] = str[i];
    ++i;
  }
  return i;
}

static uint8_t
write_hex16(char *buf, uint16_t val)
{
  static const char hex[] = "0123456789ABCDEF";
  buf[0] = hex[(val >> 12) & 0xFU];
  buf[1] = hex[(val >> 8) & 0xFU];
  buf[2] = hex[(val >> 4) & 0xFU];
  buf[3] = hex[val & 0xFU];
  return 4;
}

// ── public ─────────────────────────────────────────────────────────────────

void
write_operation(char *buf, char op, const char *type, uint16_t addr)
{
  // Layout: "<op> <type> 0x<ADDR>   "  — padded to 20 chars + NUL
  uint8_t i = 0;
  buf[i++] = op;
  buf[i++] = ' ';
  i += write_str(buf + i, type);   // 2 chars
  i += write_str(buf + i, " 0x");  // 3 chars
  i += write_hex16(buf + i, addr); // 4 chars — total 11
  while (i < 20)
    buf[i++] = ' ';
  buf[i] = '\0';
}

}; // namespace ModbusRtuServer
