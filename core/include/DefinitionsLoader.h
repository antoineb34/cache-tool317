#pragma once

#include <vector>

#include "Archive.h"
#include "ItemDef.h"
#include "LocDef.h"
#include "NpcDef.h"

class DefinitionsLoader {
public:
    // load item definitions from obj.dat + obj.idx
    void loadItems(const Archive& archive);

    // load npc definitions from npc.dat + npc.idx
    void loadNpcs(const Archive& archive);

    // load location/object definitions from loc.dat + loc.idx
    void loadLocs(const Archive& archive);

    // --- accessors ---
    const ItemDef& getItem(int id) const;
    int            itemCount()     const;

    const NpcDef&  getNpc(int id)  const;
    int            npcCount()      const;

    const LocDef&  getLoc(int id)  const;
    int            locCount()      const;

private:
    std::vector<ItemDef> items_;
    std::vector<NpcDef>  npcs_;
    std::vector<LocDef>  locs_;
};
