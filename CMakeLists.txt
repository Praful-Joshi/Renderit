cmake_minimum_required(VERSION 3.20)
project(OpenGLRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm    REQUIRED)
find_package(assimp REQUIRED)

# GLAD
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Collect all .cpp files under src/ automatically
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    external/glad/include
    external/stb
)

target_link_libraries(${PROJECT_NAME}
    glad
    glfw
    glm::glm
    assimp::assimp
    OpenGL::GL
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
)