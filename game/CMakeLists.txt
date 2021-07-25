set(game_SOURCES
    main.cpp
    oggplayer.cpp
    oggplayer.h
    track.cpp
    track.h
    mesh.cpp
    mesh.h
    camera.cpp
    camera.h
    geometryutils.cpp
    geometryutils.h
    frustum.cpp
    frustum.h
    shadermanager.cpp
    shadermanager.h
    material.cpp
    material.h
    renderer.cpp
    renderer.h
)

add_executable(game ${game_SOURCES})

target_link_libraries(
    game
    gx
    rapidjson
    OpenAL
)
