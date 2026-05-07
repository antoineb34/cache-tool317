#include "MesDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "MesDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

MesDef MesDef::parse(Buffer& buf) {
    MesDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        throw unknownOpcodeError(opcode, buf.position());
    }

    return def;
}
