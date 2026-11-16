cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
        physfs
        # Upstream migrated from hg.icculus.org (sunset) to GitHub.
        GIT_REPOSITORY "https://github.com/icculus/physfs.git"
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(physfs)
