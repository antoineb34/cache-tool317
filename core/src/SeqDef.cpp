#include "SeqDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "SeqDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

SeqDef SeqDef::parse(Buffer& buf) {
    SeqDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1: {
                int count = buf.readByte();
                def.frameIds.resize(count);
                def.frameIdHigh.resize(count);
                def.frameLengths.resize(count);
                for (int i = 0; i < count; i++) {
                    def.frameIds[i] = buf.readUShort();
                    def.frameIdHigh[i] = buf.readUShort();
                    if (def.frameIdHigh[i] == 65535)
                        def.frameIdHigh[i] = -1;
                    def.frameLengths[i] = buf.readUShort();
                }
                break;
            }

            case 2:
                def.replayOffset = buf.readUShort();
                break;

            case 3: {
                int count = buf.readByte();
                def.interleaveOrder.resize(count + 1);
                for (int i = 0; i < count; i++)
                    def.interleaveOrder[i] = buf.readByte();
                def.interleaveOrder[count] = 9999999;
                break;
            }

            case 4:
                def.stretches = true;
                break;

            case 5:
                def.priority = buf.readByte();
                break;

            case 6:
                def.mainHandItem = buf.readUShort();
                break;

            case 7:
                def.offHandItem = buf.readUShort();
                break;

            case 8:
                def.maxLoops = buf.readByte();
                break;

            case 9:
                def.animatingPrecedence = buf.readByte();
                break;

            case 10:
                def.walkingPrecedence = buf.readByte();
                break;

            case 11:
                def.replayMode = buf.readByte();
                break;

            default:
                throw unknownOpcodeError(opcode, buf.position());
        }
    }

    if (def.animatingPrecedence == -1)
        def.animatingPrecedence = def.interleaveOrder.empty() ? 0 : 2;
    if (def.walkingPrecedence == -1)
        def.walkingPrecedence = def.interleaveOrder.empty() ? 0 : 2;

    return def;
}
