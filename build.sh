#TODO: write better logic below.

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

$CXX $CXXFLAGS $INCLUDES ./src/*.cpp -o "$BUILD_DIR/cman"

echo "Built for branch: $BRANCH -> $BUILD_DIR/cman"
#g++ ./src/main.cpp ./src/filesystem.cpp ./src/cli.cpp ./src/parser.cpp -o ./bin/cman --std=c++23 -Wall -Wextra
#./bin/cman
