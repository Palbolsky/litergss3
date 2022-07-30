cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
        physfs
        #URL "https://icculus.org/physfs/downloads/physfs-3.0.2.tar.bz2"

        # BE CAREFUL IT'S ALWAYS THE LATEST VERSION HERE !!!
        URL "https://hg.icculus.org/icculus/physfs/archive/default.zip"
        #HG_REPOSITORY "https://hg.icculus.org/icculus/"
        #HG_TAG "3c9714955659"
)

FetchContent_MakeAvailable(physfs)
