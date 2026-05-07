#include "MesAnimDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "MesAnimDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

MesAnimDef MesAnimDef::parse(Buffer& buf) {
    MesAnimDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        throw unknownOpcodeError(opcode, buf.position());
    }

    return def;
}
