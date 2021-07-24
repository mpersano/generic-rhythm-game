set(game_SOURCES
    main.cpp
    oggplayer.cpp
    oggplayer.h
    track.cpp
    track.h
)

add_executable(game ${game_SOURCES})

target_link_libraries(
    game
    gx
    rapidjson
    OpenAL
)
