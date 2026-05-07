#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct Archive {
    // sub-files indexed by name hash -> decompressed bytes
    std::unordered_map<uint32_t, std::vector<uint8_t>> files;

    // get a sub-file by name hash
    std::vector<uint8_t> getFile(uint32_t nameHash) const;

    // get a sub-file by name (computes hash internally)
    std::vector<uint8_t> getFile(const std::string& name) const;

    // check if a sub-file exists
    bool hasFile(uint32_t nameHash) const;
    bool hasFile(const std::string& name) const;

    // number of sub-files in this archive
    std::size_t size() const;

    // 317 JAG name hash: hash = (hash * 61 + toupper(c)) - 32
    static uint32_t hashName(const std::string& name);
};
