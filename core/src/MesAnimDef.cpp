#include "MesAnimDef.h"
#include "Buffer.h"
#include "Utils.h"

// TODO: MesAnimDef parse is a stub — format not yet documented.
// Currently throws on any non-zero opcode.
MesAnimDef MesAnimDef::parse(Buffer& buf) {
    MesAnimDef def;
    while (true) {
        int opcode = buf.readByte();
        if (opcode == 0) break;
        throw unknownOpcodeError("MesAnimDef", opcode, buf.position());
    }
    return def;
}
