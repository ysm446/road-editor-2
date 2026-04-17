cmake_minimum_required(VERSION 3.20)
project(RoadEditor2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets)
find_package(OpenGL REQUIRED)

include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

add_executable(RoadEditor2 ${SOURCES})

if(MSVC)
    target_compile_options(RoadEditor2 PRIVATE /W4 /utf-8)
endif()

target_compile_definitions(RoadEditor2 PRIVATE
    SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(RoadEditor2 PRIVATE
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    glm::glm
    nlohmann_json::nlohmann_json
)

# Copy shaders to output directory after build
add_custom_command(TARGET RoadEditor2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:RoadEditor2>/shaders"
    COMMENT "Copying shaders to output directory"
)

# Deploy Qt DLLs on Windows
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET RoadEditor2 POST_BUILD
            COMMAND ${WINDEPLOYQT}
                --no-translations
                "$<TARGET_FILE:RoadEditor2>"
            COMMENT "Running windeployqt"
        )
    endif()
endif()
