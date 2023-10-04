cmake_minimum_required (VERSION 3.2)

project(e2sim_envman)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pg -g3" )

include_directories(src/model)
include_directories(src/api)
include_directories(src)

file(GLOB SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/*.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

file(GLOB ENVMAN_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/*.h
#    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
	

add_library(${PROJECT_NAME} STATIC ${SRCS} )

set_target_properties(${PROJECT_NAME}
	PROPERTIES PUBLIC_HEADER "${ENVMAN_PUBLIC_HEADERS}")

target_link_libraries(${PROJECT_NAME} pistache pthread)
install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
