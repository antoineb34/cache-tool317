#include "LocDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(const char* parserName, int opcode, int position) {
    return std::runtime_error(
        std::string(parserName) + ": unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

LocDef LocDef::parse(Buffer& buf) {
    LocDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1: {
                int count = buf.readByte();
                def.modelIDs.resize(count);
                def.modelTypes.resize(count);
                for (int i = 0; i < count; i++) {
                    def.modelIDs[i] = buf.readUShort();
                    def.modelTypes[i] = buf.readByte();
                }
                break;
            }

            case 2: def.name = buf.readString(); break;
            case 3: def.description = buf.readString(); break;

            case 5: {
                int count = buf.readByte();
                def.modelIDs.resize(count);
                def.modelTypes.clear();
                for (int i = 0; i < count; i++)
                    def.modelIDs[i] = buf.readUShort();
                break;
            }

            case 14: def.width = buf.readByte(); break;
            case 15: def.length = buf.readByte(); break;
            case 17: def.blocksWalk = false; break;
            case 18: def.blocksProjectiles = false; break;
            case 19: def.hasActions = buf.readByte() == 1; break;
            case 21: def.contouredGround = true; break;
            case 22: def.nonFlatShading = true; break;
            case 23: def.modelClipped = true; break;
            case 24:
                def.animationID = buf.readUShort();
                if (def.animationID == 65535) def.animationID = -1;
                break;
            case 28: def.decorDisplacement = buf.readByte(); break;
            case 29: def.ambient = buf.readSByte(); break;
            case 39: def.contrast = buf.readSByte() * 25; break;

            // right-click options (opcodes 30-34 -> slots 0-4)
            case 30: case 31: case 32: case 33: case 34: {
                std::string opt = buf.readString();
                if (opt != "hidden") def.options[opcode - 30] = opt;
                break;
            }

            // colour recoloring: 1 byte count then N src+dst uint16 pairs
            case 40: {
                int count = buf.readByte();
                def.colorSrc.resize(count);
                def.colorDst.resize(count);
                for (int i = 0; i < count; i++) {
                    def.colorSrc[i] = buf.readUShort();
                    def.colorDst[i] = buf.readUShort();
                }
                break;
            }

            case 60:
                def.mapSceneID = buf.readUShort();
                if (def.mapSceneID == 65535) def.mapSceneID = -1;
                break;
            case 62: def.rotated = true; break;
            case 64: def.castsShadow = false; break;
            case 65: def.scaleX = buf.readUShort(); break;
            case 66: def.scaleY = buf.readUShort(); break;
            case 67: def.scaleZ = buf.readUShort(); break;
            case 68:
                def.mapFunctionID = buf.readUShort();
                if (def.mapFunctionID == 65535) def.mapFunctionID = -1;
                break;
            case 69: def.surroundings = buf.readByte(); break;
            case 70: def.offsetX = buf.readShort(); break;
            case 71: def.offsetY = buf.readShort(); break;
            case 72: def.offsetZ = buf.readShort(); break;
            case 73: def.obstructsGround = true; break;
            case 74: def.hollow = true; break;
            case 75: def.supportItems = buf.readByte(); break;

            case 77: {
                def.varbitID = buf.readUShort();
                if (def.varbitID == 65535) def.varbitID = -1;
                def.varpID = buf.readUShort();
                if (def.varpID == 65535) def.varpID = -1;
                int count = buf.readByte();
                def.overrides.resize(count + 1);
                for (int i = 0; i <= count; i++) {
                    def.overrides[i] = buf.readUShort();
                    if (def.overrides[i] == 65535) def.overrides[i] = -1;
                }
                break;
            }

            default:
                throw unknownOpcodeError("LocDef", opcode, buf.position());
        }
    }

    if (def.hollow) {
        def.blocksWalk = false;
        def.blocksProjectiles = false;
    }

    if (def.supportItems == -1)
        def.supportItems = def.blocksWalk ? 1 : 0;

    return def;
}
