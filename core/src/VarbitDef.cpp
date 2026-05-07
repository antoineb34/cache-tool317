#include "VarbitDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "VarbitDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

VarbitDef VarbitDef::parse(Buffer& buf) {
    VarbitDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1:
                def.varpId = buf.readUShort();
                def.leastSignificantBit = buf.readByte();
                def.mostSignificantBit = buf.readByte();
                break;

            default:
                throw unknownOpcodeError(opcode, buf.position());
        }
    }

    return def;
}
