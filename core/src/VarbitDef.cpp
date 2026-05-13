#include "VarbitDef.h"
#include "Buffer.h"
#include "Utils.h"

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
                throw unknownOpcodeError("VarbitDef", opcode, buf.position());
        }
    }

    return def;
}
