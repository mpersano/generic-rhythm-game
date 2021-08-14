find_package(OpenGL REQUIRED)

set(gx_SOURCES
    fontcache.cpp
    glwindow.cpp
    ioutil.cpp
    lazytexture.cpp
    pixmap.cpp
    shaderprogram.cpp
    spritebatcher.cpp
    textureatlas.cpp
    textureatlaspage.cpp
    texture.cpp
    fontcache.h
    glwindow.h
    ioutil.h
    lazytexture.h
    pixmap.h
    shaderprogram.h
    spritebatcher.h
    textureatlas.h
    textureatlaspage.h
    texture.h
)

add_library(gx
    ${gx_SOURCES}
)

target_include_directories(gx
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
PRIVATE
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(gx
PUBLIC
    ${OPENGL_LIBRARIES}
    glfw
    glm
    stb
    spdlog
    glew
)
