
set(public_SOURCES
    crtlib.cpp
    crclib.cpp
    appframework.cpp
    threadtools.cpp
    keyvalues.cpp
    containers/string.cpp
    xprof.cpp
    platform.cpp
    reflection.cpp
    mem.cpp
    logger.cpp
    debug.cpp
    cmdline.cpp)

set(public_INCLUDES
    /home/jeremy/Desktop/Projects/uSource/common
    /home/jeremy/Desktop/Projects/uSource/public
    /home/jeremy/Desktop/Projects/uSource/engine
    /home/jeremy/Desktop/Projects/uSource
    /home/jeremy/Desktop/Projects/uSource/common
    /home/jeremy/Desktop/Projects/uSource/public
    /home/jeremy/Desktop/Projects/uSource/engine
    /home/jeremy/Desktop/Projects/uSource)

include_directories(${public_INCLUDES})

add_library(public SHARED ${public_SOURCES})

target_link_libraries(public RT)
target_link_libraries(public DL)
target_link_libraries(public PTHREAD)

