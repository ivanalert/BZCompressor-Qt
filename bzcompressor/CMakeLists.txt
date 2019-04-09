set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(bzcompressor SHARED
    bzcompressor.h
    bzcompressor.cpp
)

add_library(bzcompressor_static STATIC
    bzcompressor.h
    bzcompressor.cpp
)

if(WIN32)
    # For windows (custom bzip2)
    if(DEFINED BZIP2_DIR)
        find_library(BZIP2_LIB libbz2 ${BZIP2_DIR})
        if(BZIP2_LIB STREQUAL BZIP2_LIB-NOTFOUND)
            message(FATAL_ERROR "bzip2 not found in ${BZIP2_DIR}")
        else()
            message(STATUS "bzip2 found in ${BZIP2_LIB}")
            target_link_libraries(bzcompressor
                Qt5::Core
                ${BZIP2_LIB}
            )
            target_link_libraries(bzcompressor_static
                Qt5::Core
                ${BZIP2_LIB}
            )
        endif()
    endif()
else()
    target_link_libraries(bzcompressor
        Qt5::Core
        bz2
    )
    target_link_libraries(bzcompressor_static
        Qt5::Core
        bz2
    )
endif()

configure_file(bzcompressor.h "${BINARY_DIR}/lib/bzcompressor.h" COPYONLY)

target_include_directories(bzcompressor_static INTERFACE .)
