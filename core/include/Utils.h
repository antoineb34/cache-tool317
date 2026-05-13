#pragma once

#include <stdexcept>
#include <string>

inline std::runtime_error unknownOpcodeError(const char* parserName, int opcode, int position)
{
    return std::runtime_error(
        std::string(parserName) + ": unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}