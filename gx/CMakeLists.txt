find_package(OpenGL REQUIRED)

set(gx_SOURCES
    src/fontcache.cpp
    src/glwindow.cpp
    src/ioutil.cpp
    src/lazytexture.cpp
    src/pixmap.cpp
    src/shaderprogram.cpp
    src/spritebatcher.cpp
    src/textureatlas.cpp
    src/textureatlaspage.cpp
    src/texture.cpp
    include/gx/fontcache.h
    include/gx/glwindow.h
    include/gx/ioutil.h
    include/gx/lazytexture.h
    include/gx/mesh.h
    include/gx/pixmap.h
    include/gx/shaderprogram.h
    include/gx/spritebatcher.h
    include/gx/textureatlas.h
    include/gx/textureatlaspage.h
    include/gx/texture.h
)

add_library(gx
    ${gx_SOURCES}
)

target_include_directories(gx
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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

add_subdirectory(tests)
