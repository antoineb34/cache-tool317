#include "DefinitionsLoader.h"

#include <cstdint>
#include <stdexcept>
#include <string>

#include "ArchiveNames.h"
#include "Buffer.h"

// generic loader — works for any def type that has a static parse(Buffer&) and an int id field
template<typename T>
static void loadDefs(const Archive& archive,
                     uint32_t datHash,
                     uint32_t idxHash,
                     const char* datName,
                     const char* idxName,
                     std::vector<T>& out) {
    auto dat      = archive.getFile(datHash);
    auto idxData  = archive.getFile(idxHash);

    if (dat.empty())
        throw std::runtime_error(std::string("Missing or empty definition data file: ") + datName);
    if (idxData.empty())
        throw std::runtime_error(std::string("Missing or empty definition index file: ") + idxName);
    if (idxData.size() < 2)
        throw std::runtime_error(std::string("Malformed definition index file: ") + idxName);

    Buffer idx(idxData);
    int count  = idx.readUShort();
    int offset = 2; // skip the 2-byte count header at the start of .dat

    std::size_t expectedIdxSize = 2 + static_cast<std::size_t>(count) * 2;
    if (idxData.size() < expectedIdxSize)
        throw std::runtime_error(std::string("Truncated definition index file: ") + idxName);
    if (dat.size() < 2)
        throw std::runtime_error(std::string("Malformed definition data file: ") + datName);

    out.resize(count);

    for (int i = 0; i < count; i++) {
        int size = idx.readUShort();
        if (offset + size > static_cast<int>(dat.size())) {
            throw std::runtime_error(
                std::string("Definition entry exceeds data file bounds in ") + datName +
                " at id " + std::to_string(i)
            );
        }

        Buffer buf(dat.data() + offset, size);
        out[i]    = T::parse(buf);
        out[i].id = i;
        if (!buf.isEmpty()) {
            throw std::runtime_error(
                std::string("Definition parser left unread bytes in ") + datName +
                " at id " + std::to_string(i) +
                " (" + std::to_string(buf.remaining()) + " bytes remaining)"
            );
        }
        offset   += size;
    }
}

// --- load methods ---

void DefinitionsLoader::loadItems(const Archive& archive) {
    loadDefs(archive, ArchiveNames::OBJ_DAT, ArchiveNames::OBJ_IDX, "obj.dat", "obj.idx", items_);
}

void DefinitionsLoader::loadNpcs(const Archive& archive) {
    loadDefs(archive, ArchiveNames::NPC_DAT, ArchiveNames::NPC_IDX, "npc.dat", "npc.idx", npcs_);
}

void DefinitionsLoader::loadLocs(const Archive& archive) {
    loadDefs(archive, ArchiveNames::LOC_DAT, ArchiveNames::LOC_IDX, "loc.dat", "loc.idx", locs_);
}

void DefinitionsLoader::loadFlos(const Archive& archive) {
    loadDefs(archive, ArchiveNames::FLO_DAT, ArchiveNames::FLO_IDX, "flo.dat", "flo.idx", flos_);
}

void DefinitionsLoader::loadVarbits(const Archive& archive) {
    loadDefs(archive, ArchiveNames::VARBIT_DAT, ArchiveNames::VARBIT_IDX, "varbit.dat", "varbit.idx", varbits_);
}

void DefinitionsLoader::loadVarps(const Archive& archive) {
    loadDefs(archive, ArchiveNames::VARP_DAT, ArchiveNames::VARP_IDX, "varp.dat", "varp.idx", varps_);
}

// --- accessors ---

const ItemDef& DefinitionsLoader::getItem(int id) const {
    if (id < 0 || id >= (int)items_.size())
        throw std::out_of_range("ItemDef id out of range: " + std::to_string(id));
    return items_[id];
}

int DefinitionsLoader::itemCount() const {
    return static_cast<int>(items_.size());
}

const NpcDef& DefinitionsLoader::getNpc(int id) const {
    if (id < 0 || id >= (int)npcs_.size())
        throw std::out_of_range("NpcDef id out of range: " + std::to_string(id));
    return npcs_[id];
}

int DefinitionsLoader::npcCount() const {
    return static_cast<int>(npcs_.size());
}

const LocDef& DefinitionsLoader::getLoc(int id) const {
    if (id < 0 || id >= (int)locs_.size())
        throw std::out_of_range("LocDef id out of range: " + std::to_string(id));
    return locs_[id];
}

int DefinitionsLoader::locCount() const {
    return static_cast<int>(locs_.size());
}

const FloDef& DefinitionsLoader::getFlo(int id) const {
    if (id < 0 || id >= (int)flos_.size())
        throw std::out_of_range("FloDef id out of range: " + std::to_string(id));
    return flos_[id];
}

int DefinitionsLoader::floCount() const {
    return static_cast<int>(flos_.size());
}

const VarbitDef& DefinitionsLoader::getVarbit(int id) const {
    if (id < 0 || id >= (int)varbits_.size())
        throw std::out_of_range("VarbitDef id out of range: " + std::to_string(id));
    return varbits_[id];
}

int DefinitionsLoader::varbitCount() const {
    return static_cast<int>(varbits_.size());
}

const VarpDef& DefinitionsLoader::getVarp(int id) const {
    if (id < 0 || id >= (int)varps_.size())
        throw std::out_of_range("VarpDef id out of range: " + std::to_string(id));
    return varps_[id];
}

int DefinitionsLoader::varpCount() const {
    return static_cast<int>(varps_.size());
}
