project(CastServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


file(GLOB_RECURSE CAST_SOURCES CONFIGURE_DEPENDS 
	${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_executable(CastServer
    ${CAST_SOURCES}
)


target_include_directories(CastServer PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/../MainServer/include
)

find_package(directxmath CONFIG REQUIRED)

target_link_libraries(CastServer
    PUBLIC
        Common
        Boost::system
)


if (WIN32)
    target_link_libraries(CastServer PUBLIC wsock32 ws2_32)
endif()

set_target_properties(CastServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

