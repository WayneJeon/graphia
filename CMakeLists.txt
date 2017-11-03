cmake_minimum_required(VERSION 3.5)

if (NOT "$ENV{PRODUCT_NAME}" STREQUAL "")
    project($ENV{PRODUCT_NAME} LANGUAGES C CXX)
else()
    project("GraphTool" LANGUAGES C CXX)
endif()

include_directories(source)

add_subdirectory(source/thirdparty)
add_subdirectory(source/app)