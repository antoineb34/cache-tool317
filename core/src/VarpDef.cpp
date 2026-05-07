#include "VarpDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "VarpDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

VarpDef VarpDef::parse(Buffer& buf) {
    VarpDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 5:
                def.configType = buf.readUShort();
                break;

            default:
                throw unknownOpcodeError(opcode, buf.position());
        }
    }

    return def;
}
