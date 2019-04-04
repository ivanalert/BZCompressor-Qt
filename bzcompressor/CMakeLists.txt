set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_library(bzcompressor
    bzcompressor.h
    bzcompressor.cpp
)

target_link_libraries(bzcompressor
    Qt5::Core
    bz2
)

target_include_directories(bzcompressor INTERFACE .)
