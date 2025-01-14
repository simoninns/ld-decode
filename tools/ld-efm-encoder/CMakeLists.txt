add_executable(ld-efm-encoder
    main.cpp
    efmencoder.cpp
    frame.cpp
    audio.cpp
    delay_lines.cpp
    converter.cpp
)

target_include_directories(ld-efm-encoder PRIVATE .)
target_link_libraries(ld-efm-encoder PRIVATE Qt::Core lddecode-library)
install(TARGETS ld-efm-encoder)