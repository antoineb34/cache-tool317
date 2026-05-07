#include "DefinitionsLoader.h"

#include <stdexcept>

#include "ArchiveNames.h"
#include "Buffer.h"

void DefinitionsLoader::loadItems(const Archive& archive) {
    auto objDat = archive.getFile(ArchiveNames::OBJ_DAT);
    auto objIdx = archive.getFile(ArchiveNames::OBJ_IDX);

    Buffer idx(objIdx);
    int count = idx.readUShort();

    // obj.dat also starts with a 2-byte count header, so item data starts at offset 2
    int offset = 2;
    items_.resize(count);

    for (int i = 0; i < count; i++) {
        int size = idx.readUShort();
        Buffer buf(objDat.data() + offset, size);
        items_[i] = ItemDef::parse(buf);
        items_[i].id = i;
        offset += size;
    }
}

const ItemDef& DefinitionsLoader::getItem(int id) const {
    if (id < 0 || id >= (int)items_.size())
        throw std::out_of_range("ItemDef id out of range: " + std::to_string(id));
    return items_[id];
}

int DefinitionsLoader::itemCount() const {
    return static_cast<int>(items_.size());
}
