cmake_minimum_required(VERSION 3.16)
project(gboy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)

file(GLOB CPPS "src/*.cpp")

add_executable(${PROJECT_NAME} WIN32 ${CPPS})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/bin")

target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/lib/SDL3")

target_link_libraries(${PROJECT_NAME} PRIVATE
SDL3 
opengl32       # OpenGL
user32 gdi32   # стандартные Win32
shell32
)