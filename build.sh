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

if [ $1 = "build" ]
then
    compileCman    
elif [ $1 = "run" ]
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

#g++ ./src/main.cpp ./src/filesystem.cpp ./src/cli.cpp ./src/parser.cpp -o ./bin/cman --std=c++23 -Wall -Wextra
#./bin/cman

# compileDebug() {
#
# }

# store all branches in an array.
# mapfile -t branches < <(git branch --format='%(refname:short)')

