cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_TOOLCHAIN_FILE /opt/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(Amigurumi_designer VERSION 0.1.0 LANGUAGES C CXX)

set( PRJ_LIBRARIES imgui::imgui)
find_package(imgui CONFIG REQUIRED)

#include_directories("${imgui_INCLUDE_DIR}")

include(CTest)
enable_testing()

add_executable(Amigurumi_designer main.cpp)

target_link_libraries(Amigurumi_designer PRIVATE imgui::imgui)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION}) 
include(CPack)
