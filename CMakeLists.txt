cmake_minimum_required(VERSION 3.16)
project(gboy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release) 

file(GLOB CPPS "src/*.cpp")
set(SOURCES
        main.cpp
        ${CPPS})

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/bin")

target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/lib/SDL3")

target_link_libraries(${PROJECT_NAME} PRIVATE
SDL3 
opengl32       
user32 gdi32   
shell32
)

set(SDL3_DLL "${CMAKE_SOURCE_DIR}/bin/SDL3.dll")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_DLL}"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)