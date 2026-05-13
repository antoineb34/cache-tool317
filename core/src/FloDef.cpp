#include "FloDef.h"
#include "Buffer.h"
#include "Utils.h"

FloDef FloDef::parse(Buffer& buf) {
    FloDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1:
                def.rgb = static_cast<int>(buf.readTribyte());
                break;

            case 2:
                def.textureId = buf.readByte();
                break;

            case 3:
                // Present in this cache, but unused by the 317 client format.
                break;

            case 5:
                def.occlude = false;
                break;

            case 6:
                def.name = buf.readString();
                break;

            case 7:
                def.secondaryRgb = static_cast<int>(buf.readTribyte());
                break;

            default:
                throw unknownOpcodeError("FloDef", opcode, buf.position());
        }
    }

    return def;
}
