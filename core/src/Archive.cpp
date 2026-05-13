#include "Archive.h"

const Buffer& Archive::getFile(uint32_t nameHash) const {
    static Buffer emptyBuffer(std::vector<uint8_t>{});
    auto it = files.find(nameHash);
    if (it != files.end()) return it->second;
    return emptyBuffer;
}

const Buffer& Archive::getFile(const std::string& name) const {
    return getFile(hashName(name));
}

bool Archive::hasFile(uint32_t nameHash) const {
    return files.count(nameHash) > 0;
}

bool Archive::hasFile(const std::string& name) const {
    return hasFile(hashName(name));
}

std::size_t Archive::size() const {
    return files.size();
}

uint32_t Archive::hashName(const std::string& name) {
    uint32_t hash = 0;
    for (char c : name)
        hash = hash * 61 + std::toupper((uint8_t)c) - 32;
    return hash;
}
