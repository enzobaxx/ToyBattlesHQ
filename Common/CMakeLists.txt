project(Common LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(boost_json CONFIG REQUIRED)

add_subdirectory(${CMAKE_SOURCE_DIR}/ExternalLibraries/libbcrypt
                 ${CMAKE_BINARY_DIR}/libbcrypt_build)

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

file(GLOB_RECURSE COMMON_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.h
)

add_library(Common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS} 
)

set_target_properties(Common PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
)

target_include_directories(Common
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_SOURCE_DIR}/ExternalLibraries/libbcrypt/include
        ${CMAKE_SOURCE_DIR}/ExternalLibraries
        ${CMAKE_SOURCE_DIR}/Common/include/Utils
)

target_link_libraries(Common
    PUBLIC
        cryptopp::cryptopp
        unofficial::mariadb-connector-cpp::mariadbcpp
        bcrypt          # libbcrypt
        Boost::json
)

if(WIN32)
    target_link_libraries(Common PUBLIC "bcrypt.lib" ws2_32 wsock32)
endif()

if (MSVC)
    target_compile_options(Common PUBLIC /MP)
endif()
