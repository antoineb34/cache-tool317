#include "ParamDef.h"

#include <stdexcept>
#include <string>

static std::runtime_error unknownOpcodeError(int opcode, int position) {
    return std::runtime_error(
        "ParamDef: unknown opcode " + std::to_string(opcode) +
        " at buffer pos " + std::to_string(position)
    );
}

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
                throw unknownOpcodeError(opcode, buf.position());
        }
    }

    return def;
}
