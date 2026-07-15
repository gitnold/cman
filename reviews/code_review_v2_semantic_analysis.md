# cman Code Review v2 — Semantic Analysis & Dynamic Help

**Scope**: Deep audit of `SemanticAnalyzer::analyze()` and the semantic analysis pipeline. Evaluation of the "dynamic help messages (like git)" feature claimed in `CHECKLIST.md`. Specific focus on logic flow, missed cases, and how analysis feeds into execution.

Previous review (`code_review_2026-05-14.md`) covered the 11 critical bugs, most of which have been partially addressed. This review zeroes in on what remains and what was never wired up.

---

## 1. RUN action is never pushed — `--run` is a no-op
- [x] Done

**Location**: `semantic_analysis.cpp:81-96`

```cpp
if (parsed.build_project && build_ctx.lang_set) {
    this->build_config.build = parsed.build_type;
    this->action_list.push_back(ActionType::BUILD);
    // RUN is NEVER pushed
    if (parsed.bin_args.has_value() && parsed.run_bin) {
        build_config.arguments = utils::split_string(parsed.bin_args.value());
    }
}
```

`parsed.run_bin` is set to `true` by the parser when `--run` is passed, but the semantic analyzer only ever pushes `BUILD`. The `RUN` case in `execute()` is dead code.

**Impact**: `cman --run --lang cpp` builds the project but never runs the binary.

**Fix**:
```cpp
if (parsed.build_project && build_ctx.lang_set) {
    this->build_config.build = parsed.build_type;
    this->action_list.push_back(ActionType::BUILD);
    if (parsed.run_bin) {
        this->action_list.push_back(ActionType::RUN);
    }
    if (parsed.bin_args.has_value() && parsed.run_bin) {
        build_config.arguments = utils::split_string(parsed.bin_args.value());
    }
}
```

Also, `execute()`'s `RUN` handler should build-conditionally and then run, not just call `build()` again:

```cpp
case cman::ActionType::RUN:
    cman::run(this->build_config.project_name);
    break;
```

---

## 2. Silent hole: `--build` without `--lang` returns OK with no error
- [x] Done

**Location**: `semantic_analysis.cpp:81`

```cpp
if (parsed.build_project && build_ctx.lang_set) {
```

When `--build` is passed without `--lang`, `build_ctx.lang_set` is `false`, the entire block is skipped, `analyze()` returns `ResultType::OK`, and **nothing happens**. No error message, no action pushed, exit code 0.

This is the **single most user-hostile bug** in the analyzer. The user asked for a build and got silence.

**Fix** — add an explicit validation block before the action-building logic:

```cpp
if (parsed.build_project || parsed.run_bin) {
    if (!build_ctx.lang_set) {
        print_message("--build/--run requires --lang c or --lang cpp", ERROR);
        return ResultType::ILLEGAL_FORMAT;
    }
    if (!build_ctx.buildtype_set) {
        build_config.build = BuildType::SHELL_SCRIPT;  // safe default
    }
    this->action_list.push_back(ActionType::BUILD);
    // ... rest of build setup
}
```

---

## 3. `build_config.mode` is never set — uninitialized use
- [ ] Done

**Location**: `semantic_analysis.cpp:45-108`

`BuildConfig` has no default values for its POD members:

```cpp
struct BuildConfig {
    BuildType build;          // uninitialized
    std::string bin_path;
    BuildMode mode;           // uninitialized
    std::string project_name;
    std::string project_path;
    Lang language;            // uninitialized
    std::vector<std::string> arguments;
};
```

In `analyze()`, line 84 sets `build_config.build` from `parsed.build_type`, but **`build_config.mode` is never set from `parsed.mode`**. The parser captures `--mode` into `parse_result.mode`, but the analyzer ignores it.

Additionally, the `MODE` case in `parser.cpp:101-108` has an empty else:
```cpp
case OptionType::MODE:
    if (option.value.empty()) {
        this->parse_result.mode = BuildMode::DEFAULT;
    } else {
        // BUG: value is silently discarded
    }
    break;
```

So even a valid `--mode release` is lost.

**Fix** — add defaults to `BuildConfig`:
```cpp
struct BuildConfig {
    BuildType build = BuildType::SHELL_SCRIPT;
    std::string bin_path;
    BuildMode mode = BuildMode::DEFAULT;
    std::string project_name;
    std::string project_path;
    Lang language = Lang::CPP;     // or make it std::optional
    std::vector<std::string> arguments;
};
```

And read `parsed.mode` in `analyze()`:
```cpp
this->build_config.mode = parsed.mode;
```

Fix the parser's MODE handler:
```cpp
case OptionType::MODE:
    if (option.value == "release") {
        this->parse_result.mode = BuildMode::RELEASE;
    } else if (option.value == "debug") {
        this->parse_result.mode = BuildMode::DEBUG;
    } else {
        this->parse_result.mode = BuildMode::DEFAULT;
    }
    break;
```

---

## 4. `BuildCtx::validate()` is never called
- [ ] Done

**Location**: `semantic_analysis.h:33-41`, `semantic_analysis.cpp:31-33`

`BuildCtx::validate()` checks `lang_set`, `bin_path_set`, `buildtype_set`, and `project_folder_exists`. It is declared, defined, and **never called anywhere**.

Meanwhile:
- `buildtype_set` is never set to `true` anywhere in the codebase (dead field).
- `project_folder_exists` is never set to `true` anywhere.
- `buildmode_set` is declared but never used.

**Fix** — either remove these dead fields or wire validation into `analyze()`:
```cpp
if ((parsed.build_project || parsed.run_bin) && !build_ctx.validate()) {
    print_message("Build context incomplete: missing --lang, --type, or project path", ERROR);
    return ResultType::ILLEGAL_FORMAT;
}
```

---

## 5. `construct_hash()` and `evaluate()` called but never defined — linker error
- [ ] Done

**Location**: `parser.cpp:24-25`, `parser.h:56-57`

```cpp
// parser.h
ResultType construct_hash();
ResultType evaluate();

// parser.cpp constructor
if (status == ResultType::OK) {
    construct_hash();   // never defined → linker error
    evaluate();         // never defined → linker error
}
```

These functions are declared in the header and called in the constructor, but their definitions were apparently removed. **The project does not link.**

This also means the `Parser` constructor's error handling is broken — when called, it unconditionally attempts to call these symbols.

**Fix** — remove the calls and the declarations:
```cpp
// parser.h — delete lines 56-57
// parser.cpp — delete lines 24-25
```

---

## 6. `Language` switch is case-sensitive and rejects valid variants
- [ ] Done

**Location**: `semantic_analysis.cpp:66-76`

```cpp
if (parsed.language == "c") {
    build_config.language = Lang::C;
} else if (parsed.language == "cpp") {
    build_config.language = Lang::CPP;
} else {
    print_message("Unsupported language type! Try --lang c/cpp", ERROR);
    return ResultType::ILLEGAL;
}
```

`"C"`, `"CPP"`, `"C++"`, `"cxx"` all hit the error path. A user passing `--lang C` (capital) gets rejected with no case-insensitive fallback.

**Fix**:
```cpp
std::string lang = *parsed.language;
// normalize to lowercase
for (auto& ch : lang) ch = std::tolower(ch);
if (lang == "c") {
    build_config.language = Lang::C;
} else if (lang == "cpp" || lang == "c++" || lang == "cxx") {
    build_config.language = Lang::CPP;
} else {
    // ... error with suggestion
}
```

---

## 7. `NEW_PROJECT` and `INIT_PROJECT` can overlap — no mutual exclusion
- [ ] Done

**Location**: `semantic_analysis.cpp:47-106`

There is nothing preventing both `--new foo` and `--init` from being pushed to the action list. Running `cman --new foo --init` would:
1. Create `./foo/` with project structure
2. Then re-initialize the **current** directory with `initialize_current_dir()`

These operations are logically incompatible.

**Fix** — add exclusion:
```cpp
if (parsed.init_dir && parsed.init_project) {
    print_message("--new and --init are mutually exclusive", ERROR);
    return ResultType::ILLEGAL_FORMAT;
}
```

---

## 8. No dynamic/suggestive help messages — feature claimed but not built
- [ ] Done

**Location**: `CHECKLIST.md:7`, `utils/help_menu.h:15`, `utils/help_menu.cpp`

`CHECKLIST.md` marks "semantic analysis and dynamic help" as done. But `help_menu::print_help_tip()` is declared in the header and never defined. `help_menu::print_whole_help()` is declared *inside* the `.cpp` file body (line 15) with no definition. These are placeholders.

The promised "like git" behavior — suggesting the correct flag when the user makes a mistake — exists nowhere. The analyzer returns bare enums with no contextual suggestions. Compare what's needed:

| Mistake | Current behavior | Desired behavior |
|---|---|---|
| `cman --build` (no `--lang`) | Silent return OK | "error: --build requires --lang. Try --lang c or --lang cpp" |
| `cman --lang rust` | "Unsupported language type!" | "Unsupported language 'rust'. Supported: c, cpp" |
| `cman --new` (no name) | "cannot have an empty project name" | "--new requires a project name: cman --new myproject" |
| `cman --lan cpp` | "Unknown option passed!!" + full help | "Unknown option '--lan'. Did you mean '--lang'?" |

The last one (typo detection) requires edit-distance matching in `Config::check_arg()`:

```cpp
// cli.cpp check_arg — before returning ILLEGAL, try suggestions
else {
    std::string_view best_match;
    int best_dist = 3; // max edit distance for suggestion
    for (auto& [flag, _] : known_flags) {
        int d = edit_distance(option, flag);
        if (d < best_dist) {
            best_dist = d;
            best_match = flag;
        }
    }
    std::string msg = "Unknown option '";
    msg += option;
    msg += "'";
    if (!best_match.empty()) {
        msg += ". Did you mean '";
        msg += best_match;
        msg += "'?";
    }
    return make_option(OptionType::ILLEGAL, msg);
}
```

---

## 9. Parser side-effects in `parse()` break the analysis pipeline
- [ ] Done

**Location**: `parser.cpp:38-150`

`Parser::parse()` both populates `ParsedInput` **and** executes side effects:

```cpp
case OptionType::HELP:
    this->parse_result.has_help = true;
    cman::print_help();          // ← SIDE EFFECT
    return ResultType::OK;

case OptionType::VERSION:
    cman::help_menu::print_version_info();  // ← SIDE EFFECT
    return ResultType::OK;

case OptionType::UPDATE:
    cman::utils::self_update(...);          // ← SIDE EFFECT
    return ResultType::OK;
```

These short-circuit before `SemanticAnalyzer` is ever reached. The `HELP` and `VERSION` cases return early with `OK`, so `analyze()` sees `parsed.has_help == true` and does nothing useful (but also no harm). However, `UPDATE` calls `self_update` and returns — the semantic analyzer is skipped entirely.

Per the architectural intent in the previous review, `Parser::parse()` should only populate `ParsedInput`. Side effects belong in `execute()`. Add `ActionType::HELP`, `ActionType::VERSION`, `ActionType::UPDATE` to the action list and handle them in `execute()`.

---

## 10. `split_string` doesn't handle quoted arguments
- [ ] Done

**Location**: `semantic_analysis.cpp:18-28`

```cpp
std::vector<std::string> split_string(std::string str) {
    std::stringstream stream(str);
    std::string word;
    std::vector<std::string> words;
    while (stream >> word) {
        words.push_back(word);
    }
    return words;
}
```

`operator>>` splits on any whitespace and strips quotes. If the user passes:
```
cman --run -- -f "foo bar"
```
The resulting args are `["-f", "foo", "bar"]` instead of `["-f", "foo bar"]`.

Also: takes `std::string` by value (unnecessary copy) and is a bare function, not a `std::span`-aware utility.

**Fix** — use a proper quote-aware splitter or document the limitation. At minimum take `const std::string&`:

```cpp
std::vector<std::string> split_string(const std::string& str) {
    std::vector<std::string> words;
    std::string current;
    bool in_quotes = false;
    for (char ch : str) {
        if (ch == '"') {
            in_quotes = !in_quotes;
        } else if (std::isspace(ch) && !in_quotes) {
            if (!current.empty()) {
                words.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += ch;
        }
    }
    if (!current.empty()) words.push_back(std::move(current));
    return words;
}
```

---

## 11. `BUILD` and `RUN` in `execute()` are duplicates
- [ ] Done

**Location**: `semantic_analysis.cpp:113-137`

```cpp
case ActionType::BUILD:
    cman::build(this->build_config);
    break;
...
case cman::ActionType::RUN:
    cman::build(this->build_config);  // same as BUILD!
    break;
```

Both just call `build()`. The `RUN` case should execute the binary after building. And since BUILD is always pushed before RUN (once the above fix in issue #1 is applied), `RUN` can safely assume the binary exists:

```cpp
case cman::ActionType::RUN:
    cman::run(this->build_config.project_name);
    break;
```

---

## 12. `generate_build_sh` has inverted early-return logic
- [ ] Done

**Location**: `build.cpp:74-87`

```cpp
void generate_build_sh(std::string_view project_name) {
    if (!fs::exists("./build.sh")) return;  // ← returns when file DOESN'T exist
    if (fs::exists("./src/") || ...) {       // ← only runs when file DOES exist
```

The guard says "if build.sh doesn't exist, return early (do nothing)". But the function is supposed to *create* build.sh. This is the opposite of the intended logic — it only overwrites existing build.sh, never creates one from scratch.

The call site in `build()` (line 45) works around this:
```cpp
if (!fs::exists("build.sh")) generate_build_sh(config.project_name);
```
But the function itself is still buggy if called independently.

**Fix**:
```cpp
void generate_build_sh(std::string_view project_name) {
    if (fs::exists("./build.sh")) return;  // don't overwrite
    if (fs::exists("./src/") || fs::current_path().filename() == project_name) {
        // ...rest of generation
    }
}
```

---

## 13. `FileStates::update_access_time` uses `insert` instead of `insert_or_assign`
- [ ] Done

**Location**: `build.cpp:128-130`

```cpp
void FileStates::update_access_time(std::string filename) {
    this->access_times.insert({filename, fs::last_write_time(filename)});
}
```

Per the previous review (issue #10), `insert` does nothing if the key already exists. This means `was_modified` would always return `true` for files that have been checked before (since the stored time is never updated). But this was noted in v1 and remains unfixed.

---

## 14. CMake build path is broken
- [ ] Done

**Location**: `build.cpp:54-60`

```cpp
case cman::BuildType::CMAKE:
    try {
        fs::current_path(config.project_name + "build");  // missing '/' separator
        std::system("cmake");                              // should be "cmake .." or similar
    }
```

Two issues:
1. Path concatenation creates `project_namebuild` — missing `/` separator.
2. Running `cmake` with no arguments does nothing useful. Should be `cmake ..` or `cmake -S .. -B .`.

---

## 15. `generate_build_sh` hardcodes `g++` always
- [ ] Done

**Location**: `build.cpp:80`

```cpp
shell_script << "g++ ./src/*.cpp -o ./bin/" << project_name << " -Wall -Wextra\n";
```

Ignores `config.language`. A C project gets compiled with `g++`. Should check the language config and emit `gcc` for C:

```cpp
if (config.language == Lang::CPP) {
    shell_script << "g++";
} else {
    shell_script << "gcc";
}
shell_script << " ./src/*. -o ./bin/" << project_name << " -Wall -Wextra\n";
```

This means the function signature needs to also accept `Lang`, or `BuildConfig` should be passed instead of just `project_name`.

---

## SUMMARY TABLE

| # | Severity | Issue | Affects |
|---|---|---|---|
| 1 | **CRITICAL** | RUN action never pushed | `semantic_analysis.cpp:81-96` |
| 2 | **CRITICAL** | Silent hole: `--build` without `--lang` | `semantic_analysis.cpp:81` |
| 3 | **HIGH** | `build_config.mode` uninitialized/unused | `semantic_analysis.cpp:84`, `parser.cpp:101-108` |
| 4 | **HIGH** | `construct_hash()`/`evaluate()` cause linker error | `parser.cpp:24-25` |
| 5 | **HIGH** | `BuildCtx::validate()` never called | `semantic_analysis.cpp` |
| 6 | **MEDIUM** | Case-sensitive language matching | `semantic_analysis.cpp:66-72` |
| 7 | **MEDIUM** | `--new` and `--init` mutually exclusive not enforced | `semantic_analysis.cpp:47-106` |
| 8 | **MEDIUM** | Dynamic/suggestive help not implemented | `utils/help_menu.cpp`, `semantic_analysis.cpp` |
| 9 | **MEDIUM** | Parser side-effects bypass analyzer | `parser.cpp:48-51,120-128` |
| 10 | **LOW** | `split_string` ignores quoted args | `semantic_analysis.cpp:18-28` |
| 11 | **LOW** | BUILD and RUN in execute() are identical | `semantic_analysis.cpp:113-137` |
| 12 | **LOW** | `generate_build_sh` inverted guard | `build.cpp:76` |
| 13 | **LOW** | `FileStates::insert` vs `insert_or_assign` | `build.cpp:129` |
| 14 | **LOW** | CMake path missing separator | `build.cpp:55-57` |
| 15 | **LOW** | `generate_build_sh` ignores language | `build.cpp:80` |

---

## Dynamic Help: What It Would Take

The `CHECKLIST.md` claim that "semantic analysis and dynamic help" is done is false. Here is what it would actually take:

1. **Typo detection** — Edit-distance matching in `Config::check_arg()` for unknown flags → "Did you mean `--lang`?"
2. **Suggestive error messages** — Every `ResultType::ILLEGAL` return should carry a hint. Either enrich `ResultType` to carry a string payload, or add a `std::string error_suggestion` to `ParsedInput`.
3. **Help tip infrastructure** — Implement `help_menu::print_help_tip()` to give short, contextual one-liners instead of dumping the full help menu.
4. **Context-aware build errors** — When `--build` is missing `--lang`, the error should say: "Missing --lang. Usage: cman --build --lang c|--lang cpp --type script|make|cmake".
5. **Remove `Parser::parse()` side-effects** — Move HELP, VERSION, UPDATE into the action list so the analyzer can provide the full analysis pipeline before deciding output.
6. **Action ordering** — Ensure compatible action combinations (e.g., `--new foo --git` should init the git repo inside the new project, not the current directory) and reject incompatible ones with suggestions.

The existing `help_menu` module is a stub. `print_whole_help()` is declared in the `.cpp` body but never defined. `print_help_tip()` is declared in the header but never defined. Neither is called anywhere.

---

## Recommended Fix Order

1. Remove `construct_hash()`/`evaluate()` calls & decls — unblocks linking
2. Fix RUN action push — makes `--run` work
3. Add validation for `--build` without `--lang` — fixes the silent hole
4. Initialize `BuildConfig` defaults + wire `mode` — prevents UB
5. Remove parser side-effects — makes pipeline pure (HELP, VERSION, UPDATE)
6. Add edit-distance typo detection + `print_help_tip()` — delivers the "dynamic help" promise
7. Quote-aware argument splitting — correctness for `--run -- -f "foo bar"`
