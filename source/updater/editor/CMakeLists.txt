include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/thirdparty_headers.cmake)

add_definitions(-DPRODUCT_NAME="${PROJECT_NAME}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND HEADERS
)

list(APPEND SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

list(APPEND RESOURCES
    ${CMAKE_CURRENT_LIST_DIR}/resources.qrc
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(MSVC)
    add_executable(UpdateEditor WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(UpdateEditor ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

find_package(Qt5 COMPONENTS Qml Quick Xml Widgets REQUIRED)
target_link_libraries(UpdateEditor
    Qt5::Qml
    Qt5::Quick
    Qt5::Xml
    Qt5::Widgets
)

target_link_libraries(UpdateEditor shared)
