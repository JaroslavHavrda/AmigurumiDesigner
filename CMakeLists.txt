cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_TOOLCHAIN_FILE /opt/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(Amigurumi_designer VERSION 0.1.0 LANGUAGES C CXX)

set( PRJ_LIBRARIES imgui::imgui)
find_package(imgui CONFIG REQUIRED)

find_package(OpenGL REQUIRED)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#include_directories("${imgui_INCLUDE_DIR}")

include(CTest)
enable_testing()

set(CMAKE_CXX_CPPCHECK "cppcheck")

add_executable(Amigurumi_designer main.cpp)

target_link_libraries(Amigurumi_designer PRIVATE imgui::imgui)
target_link_libraries(Amigurumi_designer PRIVATE OpenGL)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION}) 
include(CPack)
