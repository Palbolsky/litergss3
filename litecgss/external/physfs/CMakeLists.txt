cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
        physfs
        # Upstream migrated from hg.icculus.org (sunset) to GitHub.
        GIT_REPOSITORY "https://github.com/icculus/physfs.git"
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
)

# physfs 3.2.0 declares cmake_minimum_required(VERSION 2.8.x), which CMake 4
# rejects; scoped to this directory, so only physfs gets the floor.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(physfs)
