cmake_minimum_required(VERSION 4.3)
project(SDLCuckoo)

set(CMAKE_CXX_STANDARD 26)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
        ${PROJECT_NAME}
        src/App.cpp
        src/App.h
)
target_sources(${PROJECT_NAME} PRIVATE src/main.cpp)

# copy the resources folder in src
add_custom_target(resources
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
        COMMENT "Copying resources"
)
add_dependencies(${PROJECT_NAME} resources)

### Libraries
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

find_package(SDL3_image CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)

find_package(SDL3_ttf CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_ttf::SDL3_ttf)
