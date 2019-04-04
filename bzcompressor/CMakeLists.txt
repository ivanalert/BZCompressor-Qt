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

target_link_libraries(bzcompressor
    Qt5::Core
    bz2
)

target_link_libraries(bzcompressor_static
    Qt5::Core
    bz2
)

configure_file(bzcompressor.h "${BINARY_DIR}/lib/bzcompressor.h" COPYONLY)

target_include_directories(bzcompressor_static INTERFACE .)
