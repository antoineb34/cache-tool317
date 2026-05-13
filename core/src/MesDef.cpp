#include "MesDef.h"
#include "Buffer.h"
#include "Utils.h"

// TODO: MesDef parse is a stub — format not yet documented.
// Currently throws on any non-zero opcode.
MesDef MesDef::parse(Buffer& buf) {
    MesDef def;
    while (true) {
        int opcode = buf.readByte();
        if (opcode == 0) break;
        throw unknownOpcodeError("MesDef", opcode, buf.position());
    }
    return def;
}
