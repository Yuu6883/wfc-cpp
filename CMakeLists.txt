cmake_minimum_required(VERSION 3.9)
project(examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(DEFAULT_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
endif()

if (WIN32)
else()
    set(CMAKE_CXX_FLAGS_RELEASE  "-march=native -Ofast")
endif()

add_executable(${PROJECT_NAME} example/main.cpp)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
      $<$<CONFIG:RelWithDebInfo>:NDEBUG>
      $<$<CONFIG:Release>:NDEBUG>
      $<$<CONFIG:MinSizeRel>:NDEBUG>
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ./src/
    ./example/include)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/example/samples.xml
            ${CMAKE_BINARY_DIR})

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/example/samples
            ${CMAKE_BINARY_DIR}/samples)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            ${CMAKE_BINARY_DIR}/results
)