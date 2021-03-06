include_directories(${CMAKE_CURRENT_LIST_DIR})

add_definitions(-DHAVE_ZLIB)
add_definitions(-DMAT73)
add_definitions(-DHAVE_HDF5)

list(APPEND SHARED_THIRDPARTY_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/endian.c
    ${CMAKE_CURRENT_LIST_DIR}/inflate.c
    ${CMAKE_CURRENT_LIST_DIR}/io.c
    ${CMAKE_CURRENT_LIST_DIR}/mat.c
    ${CMAKE_CURRENT_LIST_DIR}/mat4.c
    ${CMAKE_CURRENT_LIST_DIR}/mat5.c
    ${CMAKE_CURRENT_LIST_DIR}/mat73.c
    ${CMAKE_CURRENT_LIST_DIR}/matvar_cell.c
    ${CMAKE_CURRENT_LIST_DIR}/matvar_struct.c
    ${CMAKE_CURRENT_LIST_DIR}/read_data.c
    ${CMAKE_CURRENT_LIST_DIR}/snprintf.c
)

list(APPEND SHARED_THIRDPARTY_HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/mat4.h
    ${CMAKE_CURRENT_LIST_DIR}/mat5.h
    ${CMAKE_CURRENT_LIST_DIR}/mat73.h
    ${CMAKE_CURRENT_LIST_DIR}/matio.h
    ${CMAKE_CURRENT_LIST_DIR}/matio_private.h
    ${CMAKE_CURRENT_LIST_DIR}/matio_pubconf.h
    ${CMAKE_CURRENT_LIST_DIR}/matioConfig.h
)
