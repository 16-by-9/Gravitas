cmake_minimum_required(VERSION 3.10)
project(Gravitas LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define output dir for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/Release/x64")

# Define paths
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib/Release/x64")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(IMGUI_DIR "${INCLUDE_DIR}/imgui")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Source files
file(GLOB SRC_FILES "${SRC_DIR}/*.cpp")

# ImGui source files
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create executable
add_executable(Gravitas ${SRC_FILES} ${IMGUI_SOURCES})

# Include directories
target_include_directories(Gravitas PRIVATE
    ${INCLUDE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Link directories
link_directories(${LIB_DIR})

# Link libraries
target_link_libraries(Gravitas
    PRIVATE
        "${LIB_DIR}/glew32s.lib"
        "${LIB_DIR}/libglfw3.a"
        opengl32
        glu32
)

# For static GLEW build
target_compile_definitions(Gravitas PRIVATE GLEW_STATIC)

# Copy DLLs after build (so app runs without manual copying)
add_custom_command(TARGET Gravitas POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIB_DIR}/glfw3.dll"
        $<TARGET_FILE_DIR:Gravitas>
)