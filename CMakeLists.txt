cmake_minimum_required(VERSION 3.16)
project(AlchitryCPP VERSION 0.1 LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add GLFW submodule
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)   # Builds libglfw and exports target glfw

# Add ImGui submodule (core + GLFW backend)
add_library(imgui
imgui/imgui.cpp
imgui/imgui_draw.cpp
imgui/imgui_widgets.cpp
imgui/imgui_tables.cpp
imgui/imgui_demo.cpp
imgui/backends/imgui_impl_glfw.cpp
imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
imgui
imgui/backends
)

target_link_libraries(imgui PUBLIC glfw)



# Create executable
add_executable(${PROJECT_NAME} src/main.cpp
        src/USB/UsbUtils.cpp
        src/USB/UsbUtils.h
        src/USB/UartDevice.cpp
        src/USB/UartDevice.h
        src/App.cpp
        src/App.h
        src/HediosHandler.cpp
        src/HediosHandler.h
        src/utils.h)

add_executable(Listener src/listener.cpp
        src/USB/UsbUtils.cpp
        src/USB/UsbUtils.h
)

# Link against the .so file (shared library)
find_package(OpenGL REQUIRED)
find_library(FTDI_LIB ftd2xx PATHS FTDI NO_DEFAULT_PATH)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        imgui
        glfw
        ${CMAKE_DL_LIBS}
        OpenGL::GL
        ${FTDI_LIB}
)

target_link_libraries(Listener
        PRIVATE
        ${FTDI_LIB}
        ${CMAKE_DL_LIBS})



# 5. Installation (optional)
# install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)