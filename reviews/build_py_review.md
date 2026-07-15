# `build.py` Code Review — Comparison with `build.sh`

## Scope

Full audit of `build.py` (129 lines), intended as a flexible Python replacement for the existing `build.sh`. Compares behavior, identifies bugs, and suggests fixes.

---

## CRITICAL BUGS (will crash or silently produce wrong output)

### 1. No source files in compile command — compiler has nothing to link

`build.sh:21`:
```bash
$CXX $CXXFLAGS $INCLUDES ./src/*.cpp -o "$BUILD_DIR/cman"
```

`build.py:101`:
```python
command = [config['cxx'], config['cxx_flags'], config['includes'], "-o", f"{config['build_dir']}/cman-default"]
```

**No source files (`./src/*.cpp`) are included.** The compiler receives flags and an output path but no input files. It will either error ("no input files") or produce an empty/stale binary.

**Fix:** Add `"./src/*.cpp"` (or better, glob `src/` explicitly) to every command list.

---

### 2. `parse_cli_args()` missing argument guard — crashes with no args

`build.py:24`:
```python
args = sys.argv
if args[1] == "build":
```

If invoked as `python build.py` (no arguments), `args[1]` raises `IndexError`. The `else` on line 34 only handles an invalid *present* argument, not a missing one.

**Fix:** Add early guard:
```python
if len(args) < 2:
    print("Usage: python build.py [build|run|build-release|test|debug]")
    sys.exit(1)
```

---

### 3. `"-03"` is a literal flag, not optimization — release build is unoptimized

`build.py:105`:
```python
command = [config['cxx'], config['cxx_flags'], "-03", config['includes'], ...]
```

`-03` (zero-three) is not a recognized compiler flag. Should be **`-O3`** (capital O). The release build produces unoptimized code silently.

**Fix:** Replace `"-03"` with `"-O3"`.

---

### 4. Includes config missing leading dash — `"Iinclude"` is not a valid flag

`build.py:18`:
```python
'includes': "Iinclude -Ilib",
```

Should be `"-Iinclude -Ilib"`. The leading `-` is missing on the first include path. The compiler will treat `Iinclude` as a source file name or unknown flag.

**Fix:**
```python
'includes': "-Iinclude -Ilib",
```

---

### 5. `glob(".cpp")` matches files literally named `.cpp`, not `*.cpp` files

`build.py:63`:
```python
for entry in tests_folder.glob(".cpp"):
```

`Path.glob(".cpp")` matches a file literally named `.cpp`. Should be `"*.cpp"`. Result: **no test files are ever discovered** — the test list stays empty.

**Fix:**
```python
for entry in tests_folder.glob("*.cpp"):
```

---

### 6. `subprocess.run(["pwd"])` returns a `CompletedProcess`, not a string

`build.py:59-60`:
```python
cwd = subprocess.run(["pwd"], capture_output=True, text=True)
tests_folder = Path(f"{cwd}/tests/")
```

`cwd` is a `CompletedProcess` object, so `f"{cwd}/tests/"` produces something like:
```
<CompletedProcess returncode=0 stdout='/home/...\n' stderr=''>/tests/
```

This will fail. Already have `Path.cwd()` on line 41 — use it.

**Fix:**
```python
tests_folder = Path.cwd() / "tests"
```

---

### 7. `tests_bin` is set to the same path as `tests_folder`, then `mkdir()` fails

`build.py:71-72`:
```python
tests_bin = Path(tests_folder)    # same as tests_folder
tests_bin.mkdir()                 # FileExistsError — tests/ already exists
```

The intent is clearly to create a separate `bin/` subdirectory.

**Fix:**
```python
tests_bin = tests_folder / "bin"
tests_bin.mkdir(parents=True, exist_ok=True)
```

---

### 8. `runIntegrationTests()` is never called — dead code for TEST mode

`build.py:127-128`:
```python
if __name__ == "__main__":
    cman_build()
```

`cman_build()` (line 38) calls `parse_cli_args()` then always compiles. It never branches on `config['mode']` to call `runIntegrationTests()`. The TEST mode code path exists but is unreachable.

**Fix:** Branch in `cman_build()`:
```python
if config['mode'] == BuildMode.TEST:
    runIntegrationTests()
else:
    compileCman(config['mode'])
```

---

### 9. `"run"` mode sets `BuildMode.BUILD` and never passes `run=True`

`build.py:26-28`:
```python
elif args[1] == "run":
    config['mode'] = BuildMode.BUILD    # should be its own mode?
    config['run'] = True
```

Then `cman_build()` calls `compileCman(config['mode'])` — the `run` flag is set in config but **never passed to `compileCman()`**. Even if it were, `compileCman()` checks `if status == 0 and run:` to print a message, but does not actually run the binary.

**Fix:** Either remove the `run` concept entirely, or actually execute the binary after build:
```python
def cman_build():
    parse_cli_args()
    ...
    compileCman(config['mode'], run=config['run'])
    if config['run'] and status == 0:
        subprocess.run([f"{build_dir}/cman-default"])
```

---

### 10. `.name.split(".").pop()[0]` produces `"c"` for every binary name

`build.py:77`:
```python
f"{tests_bin}/{file.name.split('.').pop()[0]}"
```

For `test.cpp`: `.split(".")` → `["test", "cpp"]`, `.pop()` → `"cpp"`, `[0]` → `"c"`. Every compiled test binary would be named `c`, overwriting each other.

**Fix:** Use `file.stem`:
```python
f"{tests_bin}/{file.stem}"
```

(The existing `# FIX: possible parsing bug below` comment confirms this is a known issue — but it was never fixed.)

---

### 11. `current_branch.stdout` includes trailing newline — corrupts path

`build.py:42-43`:
```python
current_branch = subprocess.run(["git", "branch", "--show-current"], capture_output=True, text=True)
build_dir = f"bin/{current_branch}"
```

`current_branch.stdout` ends with `\n`. The resulting path is `bin/main\n/`. This will create a directory with a literal newline in its name or fail.

**Fix:**
```python
branch = current_branch.stdout.strip()
build_dir = f"bin/{branch}"
config['build_dir'] = build_dir
```

---

### 12. `exit()` should be `sys.exit(1)`

`build.py:80`:
```python
exit()
```

In Python, `exit()` is a shell-friendly helper that can be intercepted (e.g., in IDLE it prints a message instead of exiting). `sys.exit(1)` is the correct programmatic exit with a non-zero status.

**Fix:**
```python
sys.exit(1)
```

---

### 13. Enum values are tuples instead of integers due to trailing commas

`build.py:8-11`:
```python
class BuildMode(Enum):
    BUILD=auto(),
    DEBUG=auto(),
    RELEASE=auto(),
    TEST=auto()
```

In Python, `BUILD=auto(),` assigns `(auto(),)` — a 1-tuple. `BuildMode.BUILD.value` is `(1,)` instead of `1`. Currently `match/case` uses member identity so it still works, but any `.value` comparison would break.

**Fix:** Remove trailing commas:
```python
class BuildMode(Enum):
    BUILD = auto()
    DEBUG = auto()
    RELEASE = auto()
    TEST = auto()
```

---

### 14. `"debug"` CLI mode is defined in the enum but has no parser handler

`BuildMode.DEBUG` exists (line 9) and `compileCman` handles it (line 111-113), but `parse_cli_args()` has no `elif args[1] == "debug"` branch. Running `python build.py debug` falls through to the `else` and prints "Script requires at least one positional argument!!".

**Fix:** Add handler:
```python
elif args[1] == "debug":
    config['mode'] = BuildMode.DEBUG
```

---

## MODERATE ISSUES

### 15. `config['build_dir']` is `None` until `cman_build()` runs

Line 19 sets `'build_dir': None`. If `compileCman()` is called before `cman_build()` sets it (or called directly), it will crash. Should use a lazy-computed property or set a default.

---

### 16. `runIntegrationTests()` uses `gcc` hardcoded instead of `config['cxx']`

`build.py:77`:
```python
status = subprocess.run(["gcc", f"{file}", ...])
```

Hardcodes `gcc` instead of using `config['cxx']` (`g++`). For C++ test files, this would miss C++ linking.

**Fix:**
```python
status = subprocess.run([config['cxx'], str(file), "-o", str(tests_bin / file.stem)])
```

---

### 17. `runIntegrationTests()` uses `for bin in tests_bin.iterdir()` — masks built-in `bin`

`build.py:83`: `for bin in ...` shadows the `bin` directory. Works but is confusing and a linter warning.

**Fix:** Use `for test_bin in tests_bin.iterdir():` or similar.

---

### 18. Compiler output is not captured — user sees raw compiler stderr on success

`build.py:118`:
```python
status = subprocess.run(command).returncode
```

The comment says "do not capture output as compiler could generate some info" — but without `capture_output=True`, compiler warnings and errors are interleaved with the script's own output. At minimum capture stderr and print it only on failure.

---

### 19. Empty `if` block with no-op comment

`build.py:47-49`:
```python
if Path(f"{current_working_dir}/{build_dir}").exists():
    # do sth.
    compileCman(config['mode'])
```

The `if` and `else` branches are identical except for `mkdir()`. The conditional can be simplified:
```python
Path(f"{current_working_dir}/{build_dir}").mkdir(parents=True, exist_ok=True)
compileCman(config['mode'])
```

---

## COMPARISON WITH `build.sh` (feature gaps)

| Feature | `build.sh` | `build.py` | Notes |
|---|---|---|---|
| `compileCman` | ✓ | ✓ | py missing source files (bug #1) |
| `runBinary` | ✓ (basic) | ✗ | py sets `run` flag but never runs |
| `runEmbeddedTests` | ✓ | ✗ | py has no equivalent |
| `runIntegrationTests` | stub (no compile) | broken (bug #5-10) | Both incomplete |
| `detached` branch fallback | `BRANCH=${BRANCH:-detatched}` | ✗ | py assumes git branch always works |
| `build-release` | ✗ | ✓ | py adds this |
| `debug` mode | commented out | partial (no CLI handler) | Both incomplete |
| Error handling | `set -xe` (fail early) | no explicit error handling | py swallows errors |

---

## SUMMARY TABLE

| Severity | Count | Issues |
|---|---|---|
| **CRITICAL** | 14 | No source files, missing arg guard, `-03` typo, `Iinclude` missing dash, wrong glob, CompletedProcess misuse, `mkdir` on existing dir, dead code path, `run` never passed, broken binary name, newline in path, bare `exit()`, enum tuples, missing `debug` handler |
| **MODERATE** | 5 | `build_dir` is None, hardcoded `gcc`, `bin` name shadow, uncaptured compiler output, empty conditional branch |
| **FEATURE GAP** | 5 | No `runBinary`, no embedded tests, no detached fallback, no error propagation |

## RECOMMENDATIONS

1. **Fix critical bugs #1–14** before using this script — especially the missing source files and broken CLI argument parsing.
2. **Add `runBinary`** if the `run` command is needed (or remove the dead `run` flag).
3. **Wire `runIntegrationTests` into the dispatch** in `cman_build()` so `test` mode actually does something.
4. **Use `pathlib` consistently** — it's already imported; replace `os.path.exists`, `subprocess.run(["pwd"])`, and manual path joining.
5. **Add `sys.exit(1)` on compile failures** so CI pipelines can detect failures.
