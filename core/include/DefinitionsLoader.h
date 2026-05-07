#pragma once

#include <vector>

#include "Archive.h"
#include "FloDef.h"
#include "IdkDef.h"
#include "ItemDef.h"
#include "LocDef.h"
#include "NpcDef.h"
#include "SeqDef.h"
#include "SpotAnimDef.h"
#include "VarbitDef.h"
#include "VarpDef.h"

class DefinitionsLoader {
public:
    // load item definitions from obj.dat + obj.idx
    void loadItems(const Archive& archive);

    // load npc definitions from npc.dat + npc.idx
    void loadNpcs(const Archive& archive);

    // load location/object definitions from loc.dat + loc.idx
    void loadLocs(const Archive& archive);

    // load floor definitions from flo.dat + flo.idx
    void loadFlos(const Archive& archive);

    // load identity kit definitions from idk.dat + idk.idx
    void loadIdks(const Archive& archive);

    // load spot animation definitions from spotanim.dat + spotanim.idx
    void loadSpotAnims(const Archive& archive);

    // load animation sequence definitions from seq.dat + seq.idx
    void loadSeqs(const Archive& archive);

    // load variable-bit definitions from varbit.dat + varbit.idx
    void loadVarbits(const Archive& archive);

    // load variable-player definitions from varp.dat + varp.idx
    void loadVarps(const Archive& archive);

    // --- accessors ---
    const ItemDef& getItem(int id) const;
    int            itemCount()     const;

    const NpcDef&  getNpc(int id)  const;
    int            npcCount()      const;

    const LocDef&  getLoc(int id)  const;
    int            locCount()      const;

    const FloDef& getFlo(int id) const;
    int           floCount()     const;

    const IdkDef& getIdk(int id) const;
    int           idkCount()     const;

    const SpotAnimDef& getSpotAnim(int id) const;
    int                spotAnimCount()      const;

    const SeqDef& getSeq(int id) const;
    int           seqCount()     const;

    const VarbitDef& getVarbit(int id) const;
    int              varbitCount()     const;

    const VarpDef& getVarp(int id) const;
    int            varpCount()     const;

private:
    std::vector<ItemDef> items_;
    std::vector<NpcDef>  npcs_;
    std::vector<LocDef>  locs_;
    std::vector<FloDef>  flos_;
    std::vector<IdkDef>  idks_;
    std::vector<SeqDef>  seqs_;
    std::vector<SpotAnimDef> spotAnims_;
    std::vector<VarbitDef> varbits_;
    std::vector<VarpDef>   varps_;
};
