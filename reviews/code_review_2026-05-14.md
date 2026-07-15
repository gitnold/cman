# cman Code Review — May 2026

## Scope

Full audit of `include/` and `src/`. Focus: architectural soundness, correctness bugs, build.c/build.cpp design path, and the "multipass" CLI parser.

---

## CRITICAL BUGS (must fix)

### 1. Inverted `std::string_view::compare()` in `cli.cpp:61-65` — CLI is completely broken

```cpp
// cli.cpp:61 — WRONG
if (option.compare("-h") || option.compare("--help")) {
```

`compare()` returns **0 on match**, non-zero on mismatch. Every option that **isn't** `"-h"` makes the first clause truthy and short-circuits into `HELP`. This means `--git`, `--build`, `--new` etc. all get classified as `HELP`. All branches from `--git` onward are **dead code**.

**Fix:**

```cpp
if (option.compare("-h") == 0 || option.compare("--help") == 0) {
```

Same bug on line 64 (`-v` / `--version`) — spell it out:

```cpp
if (option.compare("-v") == 0 || option.compare("--version") == 0) {
```

And line 85 (`--update`) is missing `== 0` entirely.

---

### 2. Null dereference of `std::optional` in `semantic_analysis.cpp:64`

```cpp
if (!parsed.language->empty()) {    // UB if language has no value
```

`language` is `std::optional<std::string>`. The `->` operator is unchecked UB when the optional is disengaged. `--build` without `--lang` reaches this line with `nullopt`.

**Fix:**

```cpp
if (parsed.language.has_value() && !parsed.language->empty()) {
```

---

### 3. Inverted empty-check in `parser.cpp:55`

```cpp
if (option.value.empty() == 0) {     // WRONG — enters error when value is *non-empty*
```

`empty()` returns `true` (1) when empty, `false` (0) when non-empty. `false == 0` is `true`, so this enters the error branch for **valid** inputs like `--new myproject`.

**Fix:**

```cpp
if (option.value.empty()) {         // or:  option.value.empty() != 0
```

---

### 4. Language constants swapped in `semantic_analysis.cpp:66` and `filesystem.cpp:78-81`

```cpp
// semantic_analysis.cpp:66
if (parsed.language == "c") {
    build_config.language = Lang::CPP;    // BUG: should be Lang::C
} else if (parsed.language == "cpp") {
    build_config.language = Lang::CPP;    // correct
```

```cpp
// filesystem.cpp:77-80
case Lang::CPP:
    mainfile = "./" + project_name + "/src/main.c";   // cpp → .c, swapped
    break;
case Lang::C:
    mainfile = "./" + project_name + "/src/main.cpp";  // c → .cpp, swapped
    break;
```

---

### 5. `SemanticAnalyzer` is dead code — never instantiated

`main.cpp` creates `Config` and `Parser`, calls `parser.parse()`, and exits. The `SemanticAnalyzer` class in `semantic_analysis.cpp` is never constructed. All its logic (action list building, `execute()`) is orphaned. The `Parser::parse()` switch (parser.cpp:46-150) partially duplicates the work. Two overlapping parsing systems with no clear boundary.

**Fix:** Decide which system owns which concern, wire `SemanticAnalyzer` into `main.cpp`:

```cpp
auto parser = cman::Parser(cli.options);
if (parser.parse() != cman::ResultType::OK) return EXIT_FAILURE;

auto analyzer = cman::SemanticAnalyzer(parser.parse_result);
analyzer.analyze();
analyzer.execute();           // <-- actually runs the actions
return EXIT_SUCCESS;
```

---

### 6. Global `Config.language` reference in `filesystem.cpp:76` — does not compile

```cpp
switch (Config.language) {
```

There is no global instance `Config` of type `BuildConfig`. The inline global in `build.h` was commented out (`// inline BuildConfig Config;`). `Config` refers to the class type `cman::Config` which has no `language` member.

**Fix:** Pass `BuildConfig` (or the language) as a function parameter instead of relying on a removed global.

---

### 7. Function signature mismatches between `build.h` and `build.cpp`

| Declaration in `build.h` | Definition in `build.cpp` | Call site |
|---|---|---|
| `void generate_build_sh();` | `void generate_build_sh(std::string_view)` | `build.cpp:45` calls with no args |
| `bool compile_bash();` | `bool compile_bash(std::string_view)` | `build.cpp:46` calls with no args |

These are **link-time errors** (or ODR violations).

**Fix:** Align signatures. Either drop the parameter or add it everywhere.

---

### 8. `ast_better.cpp:176` — reference member not initialized + assigned to

```cpp
// ast_better.h:58
const std::vector<D>& options;   // reference member

// ast_better.cpp:170
ActionList<D>::ActionList(const std::vector<D>& options)
    : arena(options.size() + 5) {
    this->options = options;      // UB: binding uninitialized reference
}
```

A reference member **must** be initialized in the constructor initializer list. This is undefined behavior.

**Fix:**

```cpp
ActionList(const std::vector<D>& options)
    : options(options), arena(options.size() + 5) {}
```

(But see architecture notes below — this whole class may be unnecessary.)

---

### 9. `ast.cpp:68` — template parameter `D` ignored, hard-coded to `Option`

```cpp
template<typename D>
ActionList<D>::ActionList(std::vector<Option> options) { ... }
```

Should use `D` throughout, not `Option`. As-is the template is useless.

---

### 10. `build.cpp:128` — `insert` doesn't update existing entries

```cpp
this->access_times.insert({filename, fs::last_write_time(filename)});
```

`std::unordered_map::insert` does **nothing** if the key already exists. The intent is clearly to update.

**Fix:**

```cpp
this->access_times[filename] = fs::last_write_time(filename);
// or: this->access_times.insert_or_assign(filename, fs::last_write_time(filename));
```

---

### 11. `parser.cpp:155-168` — `get_optiontypes()` checks `ILLEGAL` but doesn't propagate the error correctly

```cpp
if (option.type != cman::OptionType::ILLEGAL) {
    this->tokens.push_back(option.type);
    ...
} else {
    return ResultType::ILLEGAL;   // returns error, but...
}
```

The caller `Parser()` constructor ignores this return value: `get_optiontypes()` is called but its result isn't stored.

---

## ARCHITECTURAL ISSUES

### A. Multi-pass pipeline is only partially plumbed

The intended flow is:

```
Config::parse() → vector<Option>   // pass 1: tokenize
Parser::parse()  → ParsedInput     // pass 2: parse
SemanticAnalyzer → ActionList      // pass 3: semantic analysis
execute()        → run actions     // pass 4: execution
```

What actually happens:

```
Config::parse() → vector<Option>
Parser::parse() → direct action (print help, init, etc.) + ParsedInput partially populated
```

The `SemanticAnalyzer` is never wired in. `Parser::parse()` conflates parsing and execution. The old `evaluate()` hash-based system lingers as deprecated dead code.

**Recommendation:**

1. Strip `evaluate()` / `construct_hash()` entirely — they're `[[deprecated]]`.
2. `Parser::parse()` should only populate `ParsedInput`, never execute side effects.
3. `main.cpp` should construct and run `SemanticAnalyzer`.
4. `SemanticAnalyzer::execute()` should be the sole executor.

### B. Hash-based evaluation is fragile and non-deterministic

```cpp
// parser.cpp:196-233
if (this->hash.value.compare("03") == 0) { ... }
else if (this->hash.value.compare("04") == 0) { ... }
```

Magic string hashes based on `std::to_string(static_cast<int>(enum_val))`. The order of tokens in the vector depends on `Config::parse()` iteration, which has bugs (see below). Any change to `OptionType` enum values silently breaks all hash comparisons. This should be removed (already marked `[[deprecated]]`).

### C. `Config::parse()` has dead code from an earlier iteration

Lines 29-31 push a `check_arg` result and then lines 37-42 push another one — the first push is always a duplicate/out-of-bounds access:

```cpp
// cli.cpp:28-44
for (int i = 1; i < this->num_of_args; i++) {
    this->options.push_back(check_arg(this->args[i], this->args[i+1]));  // LINE 29: OOB on last iter
    const char* current = this->args[i];
    char* next = (i + 1 < this->num_of_args) ? this->args[i + 1] : nullptr;
    // ... then pushes again on lines 37/42
```

Line 29 accesses `this->args[i+1]` which is out-of-bounds when `i == num_of_args - 1`. It pushes a bogus Option. Then the proper logic pushes the real one. Remove line 29.

### D. Template churn without payoff

- `ast.h` / `ast.cpp` — first Arena/ActionList attempt (raw pointers, manual memory, `uint`).
- `ast_better.h` / `ast_better.cpp` — second attempt (Rule of Five, const-correct, auto-resize).

Neither is actually used anywhere in the project. The `LinkedList` header in `utils/linkedlist.h` is also orphan boilerplate.

**Recommendation:** Remove all three until an actual consumer exists. A `std::vector<ActionType>` (already used in `SemanticAnalyzer`) is sufficient and simpler.

### E. Singletons and global state

- `FileStates` — global singleton (`instance()`).
- `UpdateConfig` — global `SelfUpdate` struct with hardcoded paths.
- `help_menu` — `std::unordered_map` as a global.

These make testing and state reasoning harder. Prefer dependency injection: construct state in `main()` and pass references down.

---

## BUILD.C/BUILD.CPP DESIGN RECOMMENDATIONS

Per `CHECKLIST.md`: the goal is a `build.c`/`build.cpp` similar to `build.zig` — users write native C/C++ code that acts as the build script.

### Proposed Design

1. **Discovery**: On `--build`, look for `build.c` or `build.cpp` in the project root.
2. **Compilation**: Compile the build file into a temporary binary (linked against a small `cman_build_api` library).
3. **Execution**: Run the temp binary. It calls `cman_build_api` functions to declare targets, sources, flags, etc.
4. **API surface** (in a shared header `cman/build_api.h`):

```cpp
namespace cman::build {
    struct Target {
        std::string name;
        std::vector<std::string> sources;
        std::vector<std::string> include_dirs;
        std::vector<std::string> libs;
        std::string standard;       // "c17", "c++23", etc.
        Lang language;
        BuildMode mode;
    };

    struct BuildScript {
        std::string project_name;
        std::vector<Target> targets;
        void add_target(Target t);
        void set_default_target(std::string_view name);
    };

    // Called by the compiled build binary:
    void declare_build(BuildScript& script);
}
```

5. **User-facing `build.cpp` example**:

```cpp
#include <cman/build_api.h>

void cman::build::declare_build(BuildScript& script) {
    script.project_name = "myapp";
    auto& tgt = script.add_target({.name = "myapp"});
    tgt.sources = {"src/main.cpp", "src/foo.cpp"};
    tgt.include_dirs = {"include"};
    tgt.language = Lang::CPP;
    tgt.standard = "c++23";
}
```

6. **Schema compilation command** that produces a `compile_commands.json` (as noted in CHECKLIST.md). `bear` is one route; a custom `--dry-run` mode on the build script is better.

7. **Why this beats shell scripts**: Type safety, error messages with source locations (`build.cpp:12:5`), IDE support, and composability. Same reason Zig chose `build.zig` over Make/CMake.

---

## MINOR ISSUES & CODE QUALITY

### Naming / Typo fixes

| File | Issue |
|---|---|
| `cli.h:13,16` | `consinder` → `consider` |
| `cli.h:32` | `try using unions for non-values params` — dangling doc |
| `cli.h:49` | `make_option` takes value by copy, should be `std::string_view` |
| `parser.h:40` | `TODO: repetitive logic below.` |
| `build.h:60` | `remove inline def below, opt for references` — already done |
| `build.h:65` | `consinder` |
| `style.h:1` | No include guards should match `CMAN_STYLE_H` |
| `ast_better.h:9` | `next_node` is `unsigned int` vs `uint` in `ast.h` — inconsistent |
| `ast_better.cpp:199` | `assignemt` → `assignment` |

### Missing `constexpr` / `noexcept`

- `OptionType`, `ActionType`, `BuildMode`, `Lang` — good candidates for enum class (already are), but `make_option` should be `constexpr`.
- `get_node` const overload returns `const U&` — good, but missing from `ast.h`.

### Unused includes

Every .cpp file includes headers it doesn't use. E.g. `parser.cpp` includes `filesystem.h`, `self_update.h`, `help_menu.h` but only uses them indirectly. Aim for minimal includes per file; use forward declarations in headers where possible.

### `print_message` takes C-string, not `std::string_view`

```cpp
void print_message(const char* message, MessageType type);
```

Callers construct `std::string` temporaries via `std::string::c_str()`. Accept `std::string_view` instead for zero-copy.

---

## SUMMARY TABLE

| Severity | Count | Examples |
|---|---|---|
| **CRITICAL** | 11 | Inverted compare, optional deref, dead code path, compile errors, swapped lang |
| **ARCHITECTURE** | 5 | Unused SemanticAnalyzer, hash fragility, OOB in Config::parse, template churn |
| **MINOR** | ~15 | Typos, missing constexpr, unused includes, style inconsistencies |

## NEXT STEPS

1. Fix the `compare()` bug in `cli.cpp` — everything else is downstream of this.
2. Wire `SemanticAnalyzer` into `main.cpp` and delete `Parser::evaluate()`.
3. Remove `ast.h`, `ast_better.h`, `linkedlist.h` until consumers exist.
4. Write integration tests (per CHECKLIST.md "do an integration test for all implemented features").
5. Prototype `build.cpp` API design in a separate branch before integrating.
