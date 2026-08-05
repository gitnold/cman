import subprocess
import os
from pathlib import Path
from enum import Enum, auto
import sys

class BuildMode(Enum):
    BUILD = auto()
    DEBUG = auto()
    RELEASE = auto()
    TEST = auto()

config = {
    'mode' : BuildMode.BUILD,
    'run' : False,
    'cxx' : "g++",
    'cxx_flags' : "-std=c++23 -Wall -Wextra",
    'includes' : "-Iinclude -Ilib",
    'build_dir' : None
}

def parse_cli_args():
    help_menu = """A python script for building cman.\nUsage:
        help            :: print this help menu and quit.
        build           :: build the project using default settings.
        run             :: build and run the resulting binary.
        build-release   :: build an optimized version of cman.
        test            :: build cman in test mode and run integration tests.
        debug           :: build a debug build of cman.
    """
    args = sys.argv

    if len(args) < 2:
        print(help_menu)
        sys.exit(1)

    if args[1] == "help":
        print(help_menu)
        sys.exit(0)

    elif args[1] == "build":
        config['mode'] = BuildMode.BUILD

    elif args[1] == "run":
        config['mode'] = BuildMode.BUILD
        config['run'] = True

    elif args[1] == "build-release":
        config['mode'] = BuildMode.RELEASE

    elif args[1] == "test":
        config['mode'] = BuildMode.TEST

    elif args[1] == "debug":
        config['mode'] = BuildMode.DEBUG

    else:
        print(help_menu)
        sys.exit(1)


def cman_build():
    parse_cli_args()

    current_working_dir = Path.cwd()
    try:
        res = subprocess.run(["git", "branch", "--show-current"], capture_output=True, text=True)
        current_branch = res.stdout.strip() if res.returncode == 0 else "detatched"
    except Exception:
        current_branch = "detatched"
    if not current_branch:
        current_branch = "detatched"

    build_dir = f"bin/{current_branch}"
    config['build_dir'] = build_dir

    # create the build directory first if it doesnt exist.
    build_dir_path = current_working_dir / build_dir
    build_dir_path.mkdir(parents=True, exist_ok=True)

    if config['mode'] == BuildMode.TEST:
        # First build cman-test (embedded tests)
        compileCman(BuildMode.TEST)

        # Run the compiled cman-test
        cman_test_path = build_dir_path / "cman-test"
        print(f"Running embedded tests ({cman_test_path})...")
        test_result = subprocess.run([str(cman_test_path)])
        if test_result.returncode != 0:
            print("Embedded tests failed!")
            sys.exit(1)

        # Run integration tests
        runIntegrationTests()
    else:
        compileCman(config['mode'])


def runIntegrationTests():
    # for every file in the tests folder compile it as a standalone binary and run it.
    test_files = list()
    tests_folder = Path.cwd() / "tests"
    if os.path.exists(tests_folder):
        # make sure its not empty, and skip 0-byte placeholder files.
        print("Looking for test files.....")
        for entry in tests_folder.glob("*.cpp"):
            if entry.is_file() and entry.stat().st_size > 0:
                test_files.append(entry)

        #create a test bin folder for separation.
        tests_bin = tests_folder / "bin"
        tests_bin.mkdir(parents=True, exist_ok=True)

        for file in test_files:
            print(f"Compiling test file : {file}")

            cxx_flags = config['cxx_flags'].split()
            includes = config['includes'].split()
            status = subprocess.run([config['cxx']] + cxx_flags + includes + [str(file), "-o", str(tests_bin / file.stem)]).returncode
            if status != 0:
                print(f"File {file} failed to compile, fix errors!!!")
                sys.exit(1)

        # run the compiled tests.
        any_failed = False
        for file in test_files:
            test_bin = tests_bin / file.stem
            print(f"Running test.... {test_bin}")

            output = subprocess.run([str(test_bin)])
            if output.returncode != 0:
                print(f"Test {test_bin} failed!\n")
                any_failed = True
            else:
                print(f"Test {test_bin} passed!!\n")

        if any_failed:
            sys.exit(1)
    else:
        print("Integration tests folder does not exist!! Create it or move to the project root and retry!!")


def compileCman(mode: BuildMode):
    command = []

    # Expand wildcard and split flags
    cxx_flags = config['cxx_flags'].split()
    includes = config['includes'].split()
    source_files = [str(p) for p in Path("src").rglob("*.cpp")]

    bin_to_run = ""
    match mode:
        case BuildMode.BUILD:
            bin_to_run = f"{config['build_dir']}/cman-default"
            command = [config['cxx']] + cxx_flags + includes + source_files + ["-o", bin_to_run]

        case BuildMode.RELEASE:
            bin_to_run = f"{config['build_dir']}/cman"
            command = [config['cxx']] + cxx_flags + ["-O3"] + includes + source_files + ["-o", bin_to_run]

        case BuildMode.TEST:
            bin_to_run = f"{config['build_dir']}/cman-test"
            command = [config['cxx']] + cxx_flags + ["-DCMAN_TESTS"] + includes + source_files + ["-o", bin_to_run]

        case BuildMode.DEBUG:
            bin_to_run = f"{config['build_dir']}/cman-debug"
            command = [config['cxx']] + cxx_flags + ["-g"] + includes + source_files + ["-o", bin_to_run]

    print("Compiling cman.....")
    ## do not capture output as compiler could generate some info.
    result = subprocess.run(command)
    status = result.returncode

    if status == 0:
        if config['run']:
            print(f"Cman built! Running `{bin_to_run}`...")
            subprocess.run([bin_to_run] + sys.argv[2:])
    else:
        sys.exit(1)


if __name__ == "__main__":
    cman_build()
