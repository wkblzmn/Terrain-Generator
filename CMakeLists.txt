cmake_minimum_required(VERSION 3.14)
project(ProceduralTerrain C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----- Dependencies -----
find_package(OpenGL REQUIRED)
find_package(glfw3  3.3 REQUIRED)
find_package(glm        REQUIRED)

# GLAD — place generated sources in vendor/glad/
# Download from https://glad.dav1d.de/
#   Language: C, Specification: OpenGL, Profile: Core, Version: 3.3
add_library(glad STATIC vendor/glad/src/glad.c)
target_include_directories(glad PUBLIC vendor/glad/include)

# ----- Executable -----
add_executable(terrain
    src/main.cpp
    src/terrain.cpp
)

target_include_directories(terrain PRIVATE
    src/
    vendor/glad/include
)

target_link_libraries(terrain PRIVATE
    glad
    glfw
    glm::glm
    OpenGL::GL
)

# ----- Copy shaders to build dir so relative path "shaders/..." works -----
add_custom_command(TARGET terrain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:terrain>/shaders
    COMMENT "Copying shaders to build directory"
)

# Optional: faster builds in debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(terrain PRIVATE -O0 -g)
else()
    target_compile_options(terrain PRIVATE -O2)
endif()
