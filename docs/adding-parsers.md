# Adding a New Definition Parser

## Pattern

Every definition parser follows the same pattern. Look at `ItemDef.cpp` or `NpcDef.cpp` as reference.

## Step 1: Create the Definition Struct

```cpp
// core/include/MyDef.h
#pragma once

#include <string>
#include <vector>
#include "Buffer.h"

struct MyDef {
    int id = -1;
    std::string name;
    // ... other fields

    static MyDef parse(int id, Buffer& buf);
};
```

## Step 2: Implement the Parser

```cpp
// core/src/MyDef.cpp
#include "MyDef.h"
#include <stdexcept>

MyDef MyDef::parse(int id, Buffer& buf) {
    MyDef def;
    def.id = id;

    while (true) {
        int opcode = buf.readByte();
        if (opcode == 0) break;  // end of definition

        switch (opcode) {
            case 1:
                def.name = buf.readString();
                break;
            case 2:
                def.someValue = buf.readUShort();
                break;
            // ... more opcodes
            default:
                throw std::runtime_error(
                    "MyDef: unknown opcode " + std::to_string(opcode) +
                    " at id " + std::to_string(id));
        }
    }

    return def;
}
```

## Step 3: Register in DefinitionsLoader

```cpp
// core/include/DefinitionsLoader.h

// Add to public section:
void loadMyDefs(const Archive& archive);
const MyDef& getMyDef(int id) const;
int myDefCount() const;

// Add to private section:
std::vector<MyDef> myDefs_;
```

```cpp
// core/src/DefinitionsLoader.cpp

#include "MyDef.h"

void DefinitionsLoader::loadMyDefs(const Archive& archive) {
    auto dat = archive.getFile("my.dat");
    auto idx = archive.getFile("my.idx");
    if (dat.empty() || idx.empty())
        throw std::runtime_error("my.dat or my.idx missing");

    Buffer idxBuf(idx);
    int count = idxBuf.readUShort();
    myDefs_.reserve(count);

    for (int i = 0; i < count; i++) {
        int size = idxBuf.readUShort();
        int offset = (i == 0) ? 2 : myDefs_.back().offset + myDefs_.back().size;
        // ... or however the index works for this format
    }
}

const MyDef& DefinitionsLoader::getMyDef(int id) const {
    if (id < 0 || id >= (int)myDefs_.size())
        throw std::runtime_error("MyDef out of range: " + std::to_string(id));
    return myDefs_[id];
}

int DefinitionsLoader::myDefCount() const {
    return (int)myDefs_.size();
}
```

## Step 4: Add to CMake

```cmake
# core/CMakeLists.txt
target_sources(core PRIVATE
    # ... existing files ...
    src/MyDef.cpp
)
```

## Step 5: Build and Verify

```bash
cmake --build build
./build/bin/tool ./cache
```

## Index Format Notes

Different definition files use different index formats:

- **obj, npc, loc, idk, seq, spotanim**: `.dat` has all entries concatenated, `.idx` has offsets
- **flo**: single `.dat`, no `.idx`, count is hardcoded or determined by file size
- **varp, varbit**: single `.dat`, entries are fixed-size or opcode-terminated

Check existing parsers to see which pattern applies.
