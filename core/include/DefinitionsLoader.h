#pragma once

#include <vector>

#include "Archive.h"
#include "ItemDef.h"

class DefinitionsLoader {
public:
    // load item definitions from obj.dat + obj.idx
    void loadItems(const Archive& archive);

    // --- accessors ---
    const ItemDef& getItem(int id) const;
    int            itemCount()     const;

private:
    std::vector<ItemDef> items_;
};
