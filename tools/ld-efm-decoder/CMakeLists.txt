add_executable(ld-efm-decoder
    main.cpp
    efmdecoder.cpp
    decoders.cpp
    frame.cpp
)

target_include_directories(ld-efm-decoder PRIVATE .)
target_link_libraries(ld-efm-decoder PRIVATE Qt::Core lddecode-library)
install(TARGETS ld-efm-decoder)