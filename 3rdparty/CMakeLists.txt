add_subdirectory(glew)
add_subdirectory(glfw)
add_subdirectory(glm)
add_subdirectory(stb)
add_subdirectory(spdlog)

set(ALSOFT_EXAMPLES OFF)
set(ALSOFT_UTILS OFF)
add_subdirectory(openal-soft)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE rapidjson/include)

add_subdirectory(fmt)
