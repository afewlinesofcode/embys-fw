#pragma once

#include <stdint.h>

namespace ModbusRtuServer
{

/**
 * Write the Modbus operation line into buf (must be at least 21 bytes).
 * Produces: "<op> <type> 0x<ADDR>          \0"
 *   op   — 'R' or 'W'
 *   type — "CO", "DI", "HR", or "IR"
 *   addr — 16-bit address in hex, zero-padded to 4 digits
 *   The line is padded with spaces to exactly 20 printable characters.
 */
void
write_operation(char *buf, char op, const char *type, uint16_t addr);

}; // namespace ModbusRtuServer
