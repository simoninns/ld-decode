add_executable(ld-efm-encoder
    main.cpp
    efm_processor.cpp
    audio.cpp
    subcode.cpp
    encoders.cpp
    ../ld-efm-decoder/library/frame.cpp
    ../ld-efm-decoder/library/reedsolomon.cpp
    ../ld-efm-decoder/library/delay_lines.cpp
    ../ld-efm-decoder/library/interleave.cpp
    ../ld-efm-decoder/library/inverter.cpp
)

# Find the Qt library
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Add the include directories for Qt
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

# Add the include directory for the ld-efm-decoder library
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../ld-efm-decoder/library)

# Link the Qt libraries to your target
target_include_directories(ld-efm-encoder PRIVATE .)
target_link_libraries(ld-efm-encoder PRIVATE Qt::Core lddecode-library)
install(TARGETS ld-efm-encoder)