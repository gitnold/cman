# "Did you mean?" — Edit-Distance Typo Detection for CLI Flags

## Goal

Replace `"Unknown option passed!!"` with `"Unknown option '--lan'. Did you mean '--lang'?"` — the git-style suggestive error that tells the user what they typed, what they probably meant, and how to fix it.

---

## Current flow (dead end)

`cli.cpp:47-107` — `Config::check_arg()` is a linear `if/else if` chain over `std::string_view::compare()`. When nothing matches:

```cpp
} else {
    return make_option(OptionType::ILLEGAL, "Unknown option passed!!");
}
```

The `Option` returned has `.type = ILLEGAL` and `.value = "Unknown option passed!!"`. This `Option` is pushed into `Config::options`, read by `Parser::get_optiontypes()`:

```cpp
// parser.cpp:160-168
if (option.type != cman::OptionType::ILLEGAL) {
    this->tokens.push_back(option.type);
} else {
    return ResultType::ILLEGAL;   // aborts parsing, discards option.value
}
```

Then `Parser::parse()` hits the default case:

```cpp
// parser.cpp:146-148
default:
    cman::print_message("Illegal token found!!", ERROR);
    cman::print_help();
    return ResultType::ILLEGAL;
```

**The `.value` string `"Unknown option passed!!"` is thrown away.** The user sees `"Illegal token found!!"` plus the full help menu dump. No suggestion, no context.

---

## Step 1 — Known-flags table (replaces the if/else chain)

Add to `cli.h`:

```cpp
struct FlagEntry {
    std::string_view flag;
    OptionType type;
};

inline constexpr FlagEntry known_flags[] = {
    {"-h",        OptionType::HELP},
    {"--help",    OptionType::HELP},
    {"-v",        OptionType::VERSION},
    {"--version", OptionType::VERSION},
    {"--new",     OptionType::NEW},
    {"--init",    OptionType::INIT},
    {"--git",     OptionType::GIT},
    {"--build",   OptionType::BUILD},
    {"--run",     OptionType::RUN},
    {"--lang",    OptionType::LANGUAGE},
    {"--mode",    OptionType::MODE},
    {"--type",    OptionType::BUILD_TYPE},
    {"--update",  OptionType::UPDATE},
    {"--",        OptionType::CLI_ARGS},
};
```

Two benefits:
1. Exact-match lookup can become O(1) with a hash set later.
2. The array **is the candidate set** for fuzzy matching.

---

## Step 2 — Levenshtein distance utility

New file `utils/string_utils.h`:

```cpp
namespace cman::utils {
    // Levenshtein distance: minimum insertions/deletions/substitutions
    // to turn 's' into 't'. O(|s|*|t|) time, O(min(|s|,|t|)) memory.
    constexpr std::size_t edit_distance(std::string_view s, std::string_view t) {
        if (s.empty()) return t.size();
        if (t.empty()) return s.size();

        auto [small, large] = s.size() < t.size()
            ? std::pair{s, t} : std::pair{t, s};

        std::vector<std::size_t> prev(small.size() + 1);
        std::vector<std::size_t> curr(small.size() + 1);

        for (std::size_t j = 0; j <= small.size(); ++j) prev[j] = j;

        for (std::size_t i = 1; i <= large.size(); ++i) {
            curr[0] = i;
            for (std::size_t j = 1; j <= small.size(); ++j) {
                if (large[i-1] == small[j-1]) {
                    curr[j] = prev[j-1];
                } else {
                    curr[j] = 1 + std::min({
                        prev[j],      // delete
                        curr[j-1],    // insert
                        prev[j-1]     // substitute
                    });
                }
            }
            std::swap(prev, curr);
        }
        return prev[small.size()];
    }
}
```

### Why Levenshtein over alternatives?

| Algorithm | Transpositions | Cost | Use case |
|---|---|---|---|
| **Levenshtein** | No | delete + insert + substitute | CLI flags rarely transpose chars; cheap enough |
| **Damerau-Levenshtein** | Yes (1 edit) | same + adjacent swap | `--lag` → `--lang` (swap) is 1 edit vs 2; pricier but better |
| **Hamming** | No | equal-length only | Useless for `--lan` vs `--lang` (different lengths) |
| **Jaro-Winkler** | Yes | 0–1, prefix boost | Overkill for short strings; weird results on `--` prefixes |

For CLI flags (3–20 chars), vanilla Levenshtein with threshold ≤ 3 has essentially zero false positives. Damerau-Levenshtein is a nice upgrade but not critical.

---

## Step 3 — Rewrite the else block in `check_arg()`

Replace `cli.cpp:103-104`:

```cpp
} else {
    return make_option(OptionType::ILLEGAL, "Unknown option passed!!");
}
```

With:

```cpp
} else {
    std::string_view best;
    std::size_t best_dist = 3;  // threshold — ignore suggestions > 3 edits

    for (const auto& entry : known_flags) {
        std::size_t d = utils::edit_distance(option, entry.flag);

        // Penalize dash-count mismatch: "build" should not fuzzy-match
        // to "--build" with distance 2 (just missing "--").
        bool input_has_double = option.starts_with("--");
        bool flag_has_double  = entry.flag.starts_with("--");
        if (input_has_double != flag_has_double) {
            d += 1;
        }

        if (d < best_dist) {
            best_dist = d;
            best = entry.flag;
        }
    }

    std::string msg = "Unknown option '";
    msg += option;
    msg += "'";
    if (!best.empty()) {
        msg += ". Did you mean '";
        msg += best;
        msg += "'?";
    }
    return make_option(OptionType::ILLEGAL, msg);
}
```

### Why the dash-mismatch penalty?

Without it, `cman build` (no dashes) fuzzy-matches to `"--build"` at distance 2 (insert `--`) and beats `--new` (distance 5+). The +1 penalty pushes dash-mismatched candidates far enough that a flag matching both dashes and characters always wins.

---

## Step 4 — Wire the suggestion through to the user

The `Option.value` now carries: `"Unknown option '--lan'. Did you mean '--lang'?"`.

Two places in `parser.cpp` need fixing to **not discard it**.

### 4a. `parser.cpp:165-168` — preserve the error message

Add to `ParsedInput` in `parser.h`:

```cpp
struct ParsedInput {
    // ...existing fields...
    std::string last_error;       // ← new
};
```

Capture it in `get_optiontypes()`:

```cpp
} else {
    this->parse_result.last_error = option.value;  // ← preserve suggestion
    return ResultType::ILLEGAL;
}
```

### 4b. `parser.cpp:146-148` — print it in the default case

```cpp
default:
    if (!parse_result.last_error.empty()) {
        cman::print_message(parse_result.last_error.c_str(), ERROR);
    } else {
        cman::print_message("Illegal token found!!", ERROR);
    }
    cman::print_help();
    return ResultType::ILLEGAL;
```

Now the user sees:

```
[ERROR]  Unknown option '--lan'. Did you mean '--lang'?
```

followed by the help menu — exactly the git-style "here's what you typed, here's what I think you meant, here's how to use me" pattern.

---

## Edge cases the penalty handles

| User types | Closest flag | Raw distance | With penalty | Correct suggestion? |
|---|---|---|---|---|
| `--lan` | `--lang` | 1 | 1 | **Yes** — missing `g` |
| `--lag` | `--lang` | 1 | 1 | **Yes** — `g` ↔ `n` |
| `--lang cpp` | `--lang` | 5 | 5 | No (≥ 3) — good, the value was swallowed by the flag |
| `build` | `--build` | 2 | **3** | No — good, missing `--` is significant |
| `--hepl` | `--help` | 2 | 2 | **Yes** — transposed `lp` → `pl` |
| `--verson` | `--version` | 2 | 2 | **Yes** — missing `i`, `o`↔`n` |
| `--initt` | `--init` | 1 | 1 | **Yes** — extra `t` |
| `--newb` | `--new` | 1 | 1 | **Yes** — extra `b` |
| `--updte` | `--update` | 1 | 1 | **Yes** — missing `a` |
| `--git` | `--git` | 0 | 0 | Exact match, never reaches this path |

---

## Performance

For each unknown flag: ≤ 14 entries × Levenshtein on strings ≤ 10 chars ≈ 140 character operations. Imperceptible in a CLI tool.

If the known-flag list grows to 100+, add an exact-match hash set as the first check (O(1)), and only fall back to fuzzy search on miss. The fuzzy search itself can be accelerated with a BK-tree if it ever becomes a bottleneck.

---

## Files to touch

| File | Change |
|---|---|
| `cli.h` | Add `FlagEntry` struct, `known_flags` array; include string_utils.h |
| `cli.cpp:103-104` | Replace bare string with fuzzy suggestion loop |
| `utils/string_utils.h` (new) | `edit_distance()` function |
| `parser.h` | Add `std::string last_error` to `ParsedInput` |
| `parser.cpp:160-168` | Capture `option.value` into `parse_result.last_error` on ILLEGAL |
| `parser.cpp:146-148` | Print `last_error` instead of generic `"Illegal token found!!"` |
