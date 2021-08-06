find_package(Boost REQUIRED)

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
    world.cpp
    world.h
    meshutils.cpp
    meshutils.h
    loadprogram.cpp
    loadprogram.h
    hudpainter.cpp
    hudpainter.h
    logo.cpp
    logo.h
    particlesystem.cpp
    particlesystem.h
)

add_executable(game ${game_SOURCES})

target_link_libraries(
    game
    gx
    rapidjson
    OpenAL
    Boost::headers
)

if (NOT WIN32)
    add_custom_command(TARGET game
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/assets" "${CMAKE_CURRENT_BINARY_DIR}/assets"
    )
endif()
