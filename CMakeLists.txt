cmake_minimum_required(VERSION 3.5)
project(libpublic C CXX)

set(SRCS 
        appframework.cpp 
        keyvalues.cpp
        cmdline.cpp
        crclib.cpp
        crtlib.cpp
        debug.cpp
        logger.cpp
        mem.cpp
        platform.cpp
        reflection.cpp
        threadtools.cpp
        xprof.cpp
        containers/string.cpp 
        )

add_definitions(-DPUBLIC_STANDALONE -DPUBLIC_EXPORT)
if(WIN32)
        add_definitions(-D_WIN32 -DWIN32)
elseif(UNIX)
        add_definitions(-DPOSIX -D_POSIX)
endif() 

add_library(libpublic SHARED ${SRCS})

set_property(TARGET libpublic PROPERTY CXX_STANDARD 17)
