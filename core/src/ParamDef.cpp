#include "ParamDef.h"
#include "Buffer.h"
#include "Utils.h"

ParamDef ParamDef::parse(Buffer& buf) {
    ParamDef def;

    while (true) {
        int opcode = buf.readByte();

        if (opcode == 0) break;

        switch (opcode) {
            case 1:
                def.type = static_cast<char>(buf.readByte());
                break;

            case 2:
                def.defaultInt = buf.readInt();
                break;

            case 4:
                def.autoDisable = false;
                break;

            case 5:
                def.defaultString = buf.readString();
                break;

            default:
                throw unknownOpcodeError("ParamDef", opcode, buf.position());
        }
    }

    return def;
}
