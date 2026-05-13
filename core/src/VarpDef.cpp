#include "VarpDef.h"
#include "Buffer.h"
#include "Utils.h"

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
                throw unknownOpcodeError("VarpDef", opcode, buf.position());
        }
    }

    return def;
}
