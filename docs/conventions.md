# Coding Conventions

## Language & Build

- **C++20** minimum
- **CMake 3.20+**
- **GCC** (primary compiler)
- Compile with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` for IDE support

## File Organization

- One major type per `.h` + `.cpp` pair
- Headers in `include/`, implementation in `src/`
- No subdirectories within `include/` or `src/` (flat structure)

## Naming

- **Structs/Classes**: `PascalCase` — `Buffer`, `CacheReader`, `ItemDef`
- **Functions**: `camelCase` — `readByte()`, `loadItems()`
- **Variables**: `camelCase` — `vertexCount`, `triangleIndex`
- **Private members**: trailing underscore — `items_`, `pos_`
- **Constants**: `UPPER_SNAKE` or `constexpr` — `lightX = -30`
- **Type aliases**: `PascalCase` — `ModelVertex`, `TriangleLitColors`

## Headers

- Use `#pragma once` (not include guards)
- Include what you use — don't rely on transitive includes
- Public API headers go in `include/`
- Implementation-only includes go in `.cpp`

## Error Handling

- **Parser errors**: throw `std::runtime_error` with descriptive message
- **Invalid data**: throw, don't return partial results
- **Missing files**: return empty vector / optional, don't crash
- **Bounds checks**: use `require()` helper in parsers

```cpp
static void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error("Parser error: " + message);
}
```

## Buffer Usage

ALL sequential byte reading goes through `Buffer`. Never do raw pointer arithmetic inline.

```cpp
// Good
Buffer buf(data);
int count = buf.readUShort();

// Bad
int count = (data[0] << 8) | data[1];  // Don't do this
```

## Structs vs Classes

- Use `struct` for plain data containers (POD-ish)
- Use `class` when there is invariant enforcement or complex behavior
- Most of this project uses `struct` with public fields

## Const Correctness

- Accessor methods should be `const`
- Parameters that aren't modified should be `const&`
- Return values that are copies: don't mark `const`

```cpp
const ItemDef& getItem(int id) const;  // good
int itemCount() const;                  // good
```

## No Namespaces

The project uses flat global scope. All types are top-level.

## Comments

- Header files: document public API with brief comments
- Implementation: explain WHY, not WHAT (the code shows what)
- Mark temporary/debug code with `// TODO: remove`
