#include "IdkDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "IdkDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

IdkDef IdkDef::parse(Buffer& buf) {
    IdkDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1:
                def.bodyPartId = buf.readByte();
                break;

            case 2: {
                int count = buf.readByte();
                def.modelIds.resize(count);
                for (int i = 0; i < count; i++)
                    def.modelIds[i] = buf.readUShort();
                break;
            }

            case 3:
                def.selectable = false;
                break;

            case 40: case 41: case 42: case 43: case 44:
            case 45: case 46: case 47: case 48: case 49:
                def.recolorSrc[opcode - 40] = buf.readUShort();
                break;

            case 50: case 51: case 52: case 53: case 54:
            case 55: case 56: case 57: case 58: case 59:
                def.recolorDst[opcode - 50] = buf.readUShort();
                break;

            case 60: case 61: case 62: case 63: case 64:
                def.headModelIds[opcode - 60] = buf.readUShort();
                break;

            default:
                throw unknownOpcodeError(opcode, buf.position());
        }
    }

    return def;
}
