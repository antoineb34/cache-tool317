#include "SpotAnimDef.h"
#include "Buffer.h"
#include "Utils.h"

SpotAnimDef SpotAnimDef::parse(Buffer& buf) {
    SpotAnimDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1:
                def.modelId = buf.readUShort();
                break;

            case 2:
                def.seqId = buf.readUShort();
                break;

            case 4:
                def.scaleXY = buf.readUShort();
                break;

            case 5:
                def.scaleZ = buf.readUShort();
                break;

            case 6:
                def.rotation = buf.readUShort();
                break;

            case 7:
                def.ambient = buf.readByte();
                break;

            case 8:
                def.contrast = buf.readByte();
                break;

            case 40: case 41: case 42: case 43: case 44: case 45:
                def.recolorSrc[opcode - 40] = buf.readUShort();
                break;

            case 50: case 51: case 52: case 53: case 54: case 55:
                def.recolorDst[opcode - 50] = buf.readUShort();
                break;

            default:
                throw unknownOpcodeError("SpotAnimDef", opcode, buf.position());
        }
    }

    return def;
}
