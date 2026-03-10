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

# ── Main renderer ─────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    external/glad/include
    external/stb
)

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        glad glfw glm::glm assimp::assimp OpenGL::GL
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
else()
    target_link_libraries(${PROJECT_NAME}
        glad glfw glm::glm assimp::assimp OpenGL::GL
    )
endif()

# ── Sandbox ───────────────────────────────────────────────────────────────────
# Standalone single-file target for quick prototyping and demos.
# Has the same external dependencies but zero coupling to src/.
#
# Build only sandbox:  cmake --build build --target Sandbox
# Run it:              ./build/Sandbox
#
# The if(EXISTS) guard means CMake won't error if the file doesn't exist yet —
# just create sandbox/sandbox.cpp and re-run cmake to activate it.
if(EXISTS "${CMAKE_SOURCE_DIR}/sandbox/sandbox.cpp")
    add_executable(Sandbox sandbox/sandbox.cpp)
    target_include_directories(Sandbox PRIVATE
        external/glad/include
        external/stb
    )
    if(APPLE)
        target_link_libraries(Sandbox
            glad glfw glm::glm OpenGL::GL
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
        )
    else()
        target_link_libraries(Sandbox
            glad glfw glm::glm OpenGL::GL
        )
    endif()
endif()