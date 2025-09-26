# cmake_minimum_required(VERSION 4.1.1)

# set(PROJECT_NAME gboy)
# project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE Release)

# file(GLOB CPPS "./src/*.cpp")

# add_executable(${PROJECT_NAME} WIN32 ${CPPS})

# target_include_directories(${PROJECT_NAME} PUBLIC "include/SDL/include")
# target_link_libraries(${PROJECT_NAME} PUBLIC "include/SDL/lib/SDL.lib")

cmake_minimum_required(VERSION 3.16)
project(gboy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)

file(GLOB CPPS "src/*.cpp")

add_executable(${PROJECT_NAME} WIN32 ${CPPS})

# Пути к SDL
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include/SDL/include")
target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include/SDL/lib")

# Линкуем библиотеку SDL2
target_link_libraries(${PROJECT_NAME} PRIVATE SDL SDLmain)