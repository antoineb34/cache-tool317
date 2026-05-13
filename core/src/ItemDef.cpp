#include "ItemDef.h"
#include "Buffer.h"
#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(const char* parserName, int opcode, int position) {
    return std::runtime_error(
        std::string(parserName) + ": unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

ItemDef ItemDef::parse(Buffer& buf) {
    ItemDef def;

    while (true) {
        // Read opcode once
        int opcode = buf.readByte();
        
        if (opcode == 0) break;
        
        switch (opcode) {
            case 1:  def.modelId     = buf.readUShort(); break;
            case 2:  def.name        = buf.readString(); break;
            case 3:  def.description = buf.readString(); break;
            case 10: def.description = buf.readString(); break; // alternate description field
            case 4:  def.zoom2d      = buf.readUShort(); break;
            case 5:  def.xan2d       = buf.readUShort(); break;
            case 6:  def.yan2d       = buf.readUShort(); break;
            case 7:  def.xof2d       = buf.readShort();  break;
            case 8:  def.yof2d       = buf.readShort();  break;
            case 11: def.stackable   = true;             break;
            case 12: def.value       = buf.readInt();    break;
            case 16: def.members     = true;             break;

            // ground options (opcodes 30-34 → slots 0-4)
            case 30: case 31: case 32: case 33: case 34:
                def.groundOptions[opcode - 30] = buf.readString();
                break;

            // inventory options (opcodes 35-39 → slots 0-4)
            case 35: case 36: case 37: case 38: case 39:
                def.inventoryOptions[opcode - 35] = buf.readString();
                break;

            // colour recoloring: 1 byte count, then N pairs of uint16
            case 40: {
                int count = buf.readByte();
                def.originalColors.resize(count);
                def.replacementColors.resize(count);
                for (int i = 0; i < count; i++) {
                    def.originalColors[i] = buf.readUShort();
                    def.replacementColors[i] = buf.readUShort();
                }
                break;
            }

            // noted item links
            case 97: def.notedId       = buf.readUShort(); break;
            case 98: def.notedTemplate = buf.readUShort(); break;

            // stack variants (opcodes 100-109)
            case 100: case 101: case 102: case 103: case 104:
            case 105: case 106: case 107: case 108: case 109:
                buf.skip(4); // uint16 item id + uint16 amount — not stored yet
                break;

            // --- opcodes below are consumed but not yet stored ---
            // equipped wear models and offsets
            case 23: buf.skip(3); break; // uint16 + int8
            case 24: buf.skip(2); break; // uint16
            case 25: buf.skip(3); break; // uint16 + int8
            case 26: buf.skip(2); break; // uint16
            case 27: buf.skip(2); break; // uint16
            case 28: buf.skip(2); break; // uint16
            case 29: buf.skip(1); break; // int8
            case 78: buf.skip(2); break; // uint16
            case 79: buf.skip(2); break; // uint16
            case 90: buf.skip(2); break; // uint16
            case 91: buf.skip(2); break; // uint16
            case 92: buf.skip(2); break; // uint16
            case 93: buf.skip(2); break; // uint16
            case 95: buf.skip(2); break; // zan2d uint16
            case 96: buf.skip(2); break; // uint16
            case 110: buf.skip(2); break;
            case 111: buf.skip(2); break;
            case 112: buf.skip(2); break;
            case 113: buf.skip(1); break;
            case 114: buf.skip(1); break;
            case 115: buf.skip(1); break; // team uint8

            default:
                throw unknownOpcodeError("ItemDef", opcode, buf.position());
        }
    }

    return def;
}
