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

add_library(libpublic SHARED ${SRCS})

set_property(TARGET libpublic PROPERTY CXX_STANDARD 17)
