cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ProjectKTVXL)
# Add SPIFFS library
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${COMPONENT_DIR}/spiffs/libspiffs.a
)