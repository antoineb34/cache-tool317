#pragma once

#include <cstdint>

// ============================================================
// 317 Cache Archive Name Hashes
// Hash algorithm: hash = (hash * 61 + toupper(c)) - 32
//
// Archive 0 (idx0) contains packed JAG sub-archives:
//   File 1 = title       (login screen assets)
//   File 2 = definitions (item, npc, object, animation defs etc.)
//   File 3 = interface
//   File 4 = media       (2D graphics/sprites)
//   File 5 = versionlist
//   File 6 = textures
//   File 7 = wordenc     (chat filter)
//   File 8 = sounds
// ============================================================

namespace ArchiveNames {

    // --------------------------------------------------------
    // Archive 0, File 1 — title
    // --------------------------------------------------------
    constexpr uint32_t P11_FULL_DAT     = 0x62a3f043; // p11_full.dat  (plain 11pt font)
    constexpr uint32_t P12_FULL_DAT     = 0xf2748da0; // p12_full.dat  (plain 12pt font)
    constexpr uint32_t B12_FULL_DAT     = 0xbcfe5ada; // b12_full.dat  (bold 12pt font)
    constexpr uint32_t Q8_FULL_DAT      = 0x0c29bdfe; // q8_full.dat   (quill 8pt font)
    constexpr uint32_t LOGO_DAT         = 0x9788a968; // logo.dat      (RS logo sprite)
    constexpr uint32_t TITLE_DAT        = 0xde3bdc91; // title.dat     (login background JPEG)
    constexpr uint32_t TITLEBOX_DAT     = 0x8f41ded6; // titlebox.dat  (login box sprite)
    constexpr uint32_t TITLEBUTTON_DAT  = 0x74916959; // titlebutton.dat (login buttons sprite)
    constexpr uint32_t RUNES_DAT        = 0x9c888208; // runes.dat     (rune icon sprites)
    constexpr uint32_t INDEX_DAT        = 0x8d00a607; // index.dat     (shared font glyph index)

    // --------------------------------------------------------
    // Archive 0, File 2 — definitions
    // --------------------------------------------------------
    constexpr uint32_t FLO_DAT          = 0xa276f8ac; // flo.dat       (floor/tile definitions)
    constexpr uint32_t IDK_DAT          = 0x08fd540b; // idk.dat       (identity kit definitions)
    constexpr uint32_t IDK_IDX          = 0x08fd9d73; // idk.idx
    constexpr uint32_t LOC_DAT          = 0x28b56bdd; // loc.dat       (object/location definitions)
    constexpr uint32_t LOC_IDX          = 0x28b5b545; // loc.idx
    constexpr uint32_t NPC_DAT          = 0x58c1fcdc; // npc.dat       (NPC definitions)
    constexpr uint32_t NPC_IDX          = 0x58c24644; // npc.idx
    constexpr uint32_t OBJ_DAT          = 0x9c9a2c36; // obj.dat       (item definitions)
    constexpr uint32_t OBJ_IDX          = 0x9c9a759e; // obj.idx
    constexpr uint32_t SEQ_DAT          = 0x34d1b7b8; // seq.dat       (animation sequence definitions)
    constexpr uint32_t SEQ_IDX          = 0x34d20120; // seq.idx
    constexpr uint32_t SPOTANIM_DAT     = 0xc7114176; // spotanim.dat  (spot animation definitions)
    constexpr uint32_t SPOTANIM_IDX     = 0xc7118ade; // spotanim.idx
    constexpr uint32_t VARP_DAT         = 0x16df653c; // varp.dat      (variable player definitions)
    constexpr uint32_t VARP_IDX         = 0x16dfaea4; // varp.idx
    constexpr uint32_t VARBIT_DAT       = 0xe14fb6af; // varbit.dat    (variable bit definitions)
    constexpr uint32_t VARBIT_IDX       = 0xe1500017; // varbit.idx

    // --------------------------------------------------------
    // Archive 0, File 2 — definitions (cracked)
    // --------------------------------------------------------
    constexpr uint32_t FLO_IDX          = 0xa2774214; // flo.idx      (floor tile index, 246 bytes)
    constexpr uint32_t MESANIM_DAT      = 0x0ae38f79; // mesanim.dat  (message animation data, 22 bytes)
    constexpr uint32_t MESANIM_IDX      = 0x0ae3d8e1; // mesanim.idx  (message animation index, 42 bytes)

    constexpr uint32_t MES_DAT          = 0x3d591c44; // mes.dat      (message definitions, 46 bytes)
    constexpr uint32_t MES_IDX          = 0x3d5965ac; // mes.idx      (message definition index, 90 bytes)
    constexpr uint32_t PARAM_DAT        = 0x93a322ec; // param.dat    (parameter definitions, 108 bytes)
    constexpr uint32_t PARAM_IDX        = 0x93a36c54; // param.idx    (parameter definition index, 214 bytes)

} // namespace ArchiveNames
