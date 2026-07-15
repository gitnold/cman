#TODO: write better logic below.
#TODO: could python scripts be a better choice???
set -xe

CXX=g++
CXXFLAGS="-std=c++23 -Wall -Wextra"
INCLUDES="-Iinclude -Ilib"

# get current branch.
BRANCH=$(git branch --show-current)
BRANCH=${BRANCH:-detatched}

# build directory.
BUILD_DIR="bin/$BRANCH"

mkdir -p "$BUILD_DIR"

#$CXX $CXXFLAGS $INCLUDES ./src/*.cpp -o "$BUILD_DIR/cman"
compileCman(){
    # compile comand.
    $CXX $CXXFLAGS $INCLUDES ./src/*.cpp -o "$BUILD_DIR/cman"
    echo "Built for branch: $BRANCH -> $BUILD_DIR/cman"
}

# have one function but do variable based path resolution
runBinary() {
    "$BUILD_DIR/cman"
}

if [ "$1" = "build" ]
then
    compileCman    
elif [ "$1" = "run" ]
then
    # check the second argument.
    # have a list of all available branches and itereate through them.
    if [[ -z "$2" ]]
    then 
        runBinary
    else
        # loop through the branches array find matching argument.
        echo "Looping through the branches array"
    fi
fi

runEmbeddedTests() {
    local CXXFLAGS_TESTS="-std=c++23 -Wall -Wextra -DCMAN_TESTS"
    $CXX $CXXFLAGS_TESTS $INCLUDES ./src/*.cpp -o "$BUILD_DIR/cman_test"
    echo "Built tests for $BRANCH -> $BUILD_DIR/cman_test"

    # run tests here.
    echo "Running binary with test cases active........."
    "$BUILD_DIR/cman_test"
}

runIntegrationTests() {
    local TESTS_FOLDER="tests/"
    local TESTS_BIN_FOLDER="$TESTS_FOLDER/bin/"
    # 1. Create an empty array
    cpp_files=()

    # 2. Populate the array safely
    while IFS= read -r -d '' file; do
        cpp_files+=("$file")
    done < <(find . -maxdepth 1 -name "*.cpp" -print0)

    # 3. Print the total count
    echo "Found ${#cpp_files[@]} files."

    # 4. Loop through and print each file
    for file in "${cpp_files[@]}"; do
        echo "Compiling........ :: $file"
        # compile the tests.
        # TODO: add compilationlogic
    done

}

#g++ ./src/main.cpp ./src/filesystem.cpp ./src/cli.cpp ./src/parser.cpp -o ./bin/cman --std=c++23 -Wall -Wextra
#./bin/cman

# compileDebug() {
#
# }

# store all branches in an array.
# mapfile -t branches < <(git branch --format='%(refname:short)')

