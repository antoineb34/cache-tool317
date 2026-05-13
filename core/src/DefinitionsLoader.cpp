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
    const Buffer& dat      = archive.getFile(datHash);
    const Buffer& idxData  = archive.getFile(idxHash);

    if (dat.empty())
        throw std::runtime_error(std::string("Missing or empty definition data file: ") + datName);
    if (idxData.empty())
        throw std::runtime_error(std::string("Missing or empty definition index file: ") + idxName);
    if (idxData.size() < 2)
        throw std::runtime_error(std::string("Malformed definition index file: ") + idxName);
    
    // Create copies of the buffers for reading (we need to advance positions)
    Buffer datBuf(dat.slice(0, dat.size()));
    Buffer idxBuf(idxData.slice(0, idxData.size()));
    
    int count  = idxBuf.readUShort();
    int offset = 2; // skip the 2-byte count header at the start of .dat

    std::size_t expectedIdxSize = 2 + static_cast<std::size_t>(count) * 2;
    if (idxData.size() < expectedIdxSize)
        throw std::runtime_error(std::string("Truncated definition index file: ") + idxName);
    if (dat.size() < 2)
        throw std::runtime_error(std::string("Malformed definition data file: ") + datName);

    out.resize(count);

    for (int i =0; i < count; i++) {
        int size = idxBuf.readUShort();
        if (offset + size > static_cast<int>(dat.size())) {
            throw std::runtime_error(
                std::string("Definition entry exceeds data file bounds in ") + datName +
                " at id " + std::to_string(i)
            );
        }

        // Use slice() to get just the data we need for this entry
        Buffer buf(datBuf.slice(offset, offset + size));
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

void DefinitionsLoader::loadIdks(const Archive& archive) {
    loadDefs(archive, ArchiveNames::IDK_DAT, ArchiveNames::IDK_IDX, "idk.dat", "idk.idx", idks_);
}

void DefinitionsLoader::loadMesAnims(const Archive& archive) {
    loadDefs(archive, ArchiveNames::MESANIM_DAT, ArchiveNames::MESANIM_IDX, "mesanim.dat", "mesanim.idx", mesAnims_);
}

void DefinitionsLoader::loadMes(const Archive& archive) {
    loadDefs(archive, ArchiveNames::MES_DAT, ArchiveNames::MES_IDX, "mes.dat", "mes.idx", mes_);
}

void DefinitionsLoader::loadParams(const Archive& archive) {
    loadDefs(archive, ArchiveNames::PARAM_DAT, ArchiveNames::PARAM_IDX, "param.dat", "param.idx", params_);
}

void DefinitionsLoader::loadSpotAnims(const Archive& archive) {
    loadDefs(archive, ArchiveNames::SPOTANIM_DAT, ArchiveNames::SPOTANIM_IDX, "spotanim.dat", "spotanim.idx", spotAnims_);
}

void DefinitionsLoader::loadSeqs(const Archive& archive) {
    loadDefs(archive, ArchiveNames::SEQ_DAT, ArchiveNames::SEQ_IDX, "seq.dat", "seq.idx", seqs_);
}

void DefinitionsLoader::loadVarbits(const Archive& archive) {
    loadDefs(archive, ArchiveNames::VARBIT_DAT, ArchiveNames::VARBIT_IDX, "varbit.dat", "varbit.idx", varbits_);
}

void DefinitionsLoader::loadVarps(const Archive& archive) {
    loadDefs(archive, ArchiveNames::VARP_DAT, ArchiveNames::VARP_IDX, "varp.dat", "varp.idx", varps_);
}

// --- accessors ---

const ItemDef& DefinitionsLoader::getItem(int id) const {
    if (id <0 || id >= (int)items_.size())
        throw std::out_of_range("ItemDef id out of range: " + std::to_string(id));
    return items_[id];
}

int DefinitionsLoader::itemCount() const {
    return static_cast<int>(items_.size());
}

const NpcDef& DefinitionsLoader::getNpc(int id) const {
    if (id <0 || id >= (int)npcs_.size())
        throw std::out_of_range("NpcDef id out of range: " + std::to_string(id));
    return npcs_[id];
}

int DefinitionsLoader::npcCount() const {
    return static_cast<int>(npcs_.size());
}

const LocDef& DefinitionsLoader::getLoc(int id) const {
    if (id <0 || id >= (int)locs_.size())
        throw std::out_of_range("LocDef id out of range: " + std::to_string(id));
    return locs_[id];
}

int DefinitionsLoader::locCount() const {
    return static_cast<int>(locs_.size());
}

const FloDef& DefinitionsLoader::getFlo(int id) const {
    if (id <0 || id >= (int)flos_.size())
        throw std::out_of_range("FloDef id out of range: " + std::to_string(id));
    return flos_[id];
}

int DefinitionsLoader::floCount() const {
    return static_cast<int>(flos_.size());
}

const IdkDef& DefinitionsLoader::getIdk(int id) const {
    if (id <0 || id >= (int)idks_.size())
        throw std::out_of_range("IdkDef id out of range: " + std::to_string(id));
    return idks_[id];
}

int DefinitionsLoader::idkCount() const {
    return static_cast<int>(idks_.size());
}

const MesAnimDef& DefinitionsLoader::getMesAnim(int id) const {
    if (id <0 || id >= (int)mesAnims_.size())
        throw std::out_of_range("MesAnimDef id out of range: " + std::to_string(id));
    return mesAnims_[id];
}

int DefinitionsLoader::mesAnimCount() const {
    return static_cast<int>(mesAnims_.size());
}

const MesDef& DefinitionsLoader::getMes(int id) const {
    if (id <0 || id >= (int)mes_.size())
        throw std::out_of_range("MesDef id out of range: " + std::to_string(id));
    return mes_[id];
}

int DefinitionsLoader::mesCount() const {
    return static_cast<int>(mes_.size());
}

const ParamDef& DefinitionsLoader::getParam(int id) const {
    if (id <0 || id >= (int)params_.size())
        throw std::out_of_range("ParamDef id out of range: " + std::to_string(id));
    return params_[id];
}

int DefinitionsLoader::paramCount() const {
    return static_cast<int>(params_.size());
}

const SpotAnimDef& DefinitionsLoader::getSpotAnim(int id) const {
    if (id <0 || id >= (int)spotAnims_.size())
        throw std::out_of_range("SpotAnimDef id out of range: " + std::to_string(id));
    return spotAnims_[id];
}

int DefinitionsLoader::spotAnimCount() const {
    return static_cast<int>(spotAnims_.size());
}

const SeqDef& DefinitionsLoader::getSeq(int id) const {
    if (id <0 || id >= (int)seqs_.size())
        throw std::out_of_range("SeqDef id out of range: " + std::to_string(id));
    return seqs_[id];
}

int DefinitionsLoader::seqCount() const {
    return static_cast<int>(seqs_.size());
}

const VarbitDef& DefinitionsLoader::getVarbit(int id) const {
    if (id <0 || id >= (int)varbits_.size())
        throw std::out_of_range("VarbitDef id out of range: " + std::to_string(id));
    return varbits_[id];
}

int DefinitionsLoader::varbitCount() const {
    return static_cast<int>(varbits_.size());
}

const VarpDef& DefinitionsLoader::getVarp(int id) const {
    if (id <0 || id >= (int)varps_.size())
        throw std::out_of_range("VarpDef id out of range: " + std::to_string(id));
    return varps_[id];
}

int DefinitionsLoader::varpCount() const {
    return static_cast<int>(varps_.size());
}
