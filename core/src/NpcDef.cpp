#include "NpcDef.h"

#include <iostream>

NpcDef NpcDef::parse(Buffer& buf) {
    NpcDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1: { // model IDs
                int count = buf.readByte();
                def.modelIDs.resize(count);
                for (int i = 0; i < count; i++)
                    def.modelIDs[i] = buf.readUShort();
                break;
            }
            case 2:  def.name    = buf.readString(); break;
            case 3:  def.examine = buf.readString(); break;
            case 12: def.size    = buf.readByte();   break;
            case 13: def.seqStandID = buf.readUShort(); break;
            case 14: def.seqWalkID  = buf.readUShort(); break;
            case 17: // all walk animations at once
                def.seqWalkID       = buf.readUShort();
                def.seqTurnAroundID = buf.readUShort();
                def.seqTurnLeftID   = buf.readUShort();
                def.seqTurnRightID  = buf.readUShort();
                break;

            // right-click options (opcodes 30-34 → slots 0-4)
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

            case 60: { // head model IDs (used in dialogue)
                int count = buf.readByte();
                def.headModelIDs.resize(count);
                for (int i = 0; i < count; i++)
                    def.headModelIDs[i] = buf.readUShort();
                break;
            }

            case 90: buf.skip(2); break; // unknown uint16
            case 91: buf.skip(2); break; // unknown uint16
            case 92: buf.skip(2); break; // unknown uint16
            case 93: def.showOnMinimap  = false;           break;
            case 95: def.level          = buf.readUShort(); break;
            case 97: def.scaleXY        = buf.readUShort(); break;
            case 98: def.scaleZ         = buf.readUShort(); break;
            case 99: def.important      = true;             break;
            case 100: def.lightAmbient    = buf.readSByte(); break;
            case 101: def.lightAttenuation = buf.readSByte() * 5; break;
            case 102: def.headicon      = buf.readUShort(); break;
            case 103: def.turnSpeed     = buf.readUShort(); break;

            case 106: { // npc transform/override system
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

            case 107: def.interactable = false; break;

            default:
                std::cerr << "NpcDef: unknown opcode " << opcode
                          << " at buffer pos " << buf.position() << std::endl;
                return def;
        }
    }

    return def;
}
