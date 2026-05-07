#include "DefinitionsLoader.h"

#include <stdexcept>

#include "ArchiveNames.h"
#include "Buffer.h"

// generic loader — works for any def type that has a static parse(Buffer&) and an int id field
template<typename T>
static void loadDefs(const Archive& archive, uint32_t datHash, uint32_t idxHash, std::vector<T>& out) {
    auto dat      = archive.getFile(datHash);
    auto idxData  = archive.getFile(idxHash);

    Buffer idx(idxData);
    int count  = idx.readUShort();
    int offset = 2; // skip the 2-byte count header at the start of .dat

    out.resize(count);

    for (int i = 0; i < count; i++) {
        int size = idx.readUShort();
        Buffer buf(dat.data() + offset, size);
        out[i]    = T::parse(buf);
        out[i].id = i;
        offset   += size;
    }
}

// --- load methods ---

void DefinitionsLoader::loadItems(const Archive& archive) {
    loadDefs(archive, ArchiveNames::OBJ_DAT, ArchiveNames::OBJ_IDX, items_);
}

void DefinitionsLoader::loadNpcs(const Archive& archive) {
    loadDefs(archive, ArchiveNames::NPC_DAT, ArchiveNames::NPC_IDX, npcs_);
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
