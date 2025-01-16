add_executable(ld-efm-encoder
    main.cpp
    efmencoder.cpp
    frame.cpp
    audio.cpp
    delay_lines.cpp
    encoders.cpp
    reedsolomon.cpp
)

# Find the Qt library
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

# Add the include directories for Qt
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})

# Link the Qt libraries to your target
target_include_directories(ld-efm-encoder PRIVATE .)
target_link_libraries(ld-efm-encoder PRIVATE Qt::Core lddecode-library)
install(TARGETS ld-efm-encoder)