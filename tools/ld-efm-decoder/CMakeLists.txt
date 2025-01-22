add_executable(ld-efm-decoder
    main.cpp
    efm_processor.cpp
    delay_lines.cpp
    decoders.cpp
    frame.cpp
    delay_lines.cpp
    reedsolomon.cpp
)

# Find the Qt library
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Add the include directories for Qt
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

# Link the Qt libraries to your target
target_include_directories(ld-efm-decoder PRIVATE .)
target_link_libraries(ld-efm-decoder PRIVATE Qt::Core lddecode-library)
install(TARGETS ld-efm-decoder)