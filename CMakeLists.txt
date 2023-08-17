cmake_minimum_required(VERSION 3.3)
project(sls-sdk-cpp)

# Options
option(BUILD_SHARED_LIBS  "Enable shared library" OFF)
option(BUILD_SAMPLE "Build sample" OFF)
option(BUILD_TESTS "Build unit and perfermence tests" OFF)
option(VERBOSE "Verbose output" OFF)
option(INSTALL_SAMPLE "install sample" OFF)
option(INSTALL_PROTOBUF_HDRS "" OFF)
option(INSTALL_CURL_HDRS "" OFF)

set(VERBOSE ON)
set(BUILD_TESTS ON)
set(BUILD_SAMPLE ON)


# Variables
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(sls_sdk_lz4_dir "include/lz4")
set(sls_sdk_rapidjson_dir "include/rapidjson")
file(GLOB sls_sdk_srcs "*.cpp")
file(GLOB sls_sdk_headers "*.h")
set(PB_LIB "${PROJECT_NAME}-pb")
set(STATIC_LIB "${PROJECT_NAME}-static")
set(SDK_ROOT ${CMAKE_SOURCE_DIR})
set(SDK_CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
set(PROTOS sls_logs.proto)

# delete os platform: apple/linux/windows
if(CMAKE_HOST_APPLE)
    set(PLATFORM_APPLE 1)
    set(TARGET_OS "APPLE")
elseif(CMAKE_HOST_UNIX)
    set(PLATFORM_LINUX 1)
    set(TARGET_OS "LINUX")
elseif(CMAKE_HOST_WIN32)
    set(PLATFORM_WINDOWS 1)
    set(TARGET_OS "WINDOWS")
else()
    message(FATAL_ERROR "Do not support unknown host OS")
endif()

# detect 32-bit / 64-bit
set(WLIB_TARGET "unknown")
if (${TARGET_OS} STREQUAL "WINDOWS")
	set(WLIB_TARGET "x86")
	if (CMAKE_CL_64)
	    set(WLIB_TARGET "x64")
	endif()
endif()


# compiler flags
set(CMAKE_CXX_STANDARD 11)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    get_filename_component(MSVC_CL_PATH ${CMAKE_CXX_COMPILER} PATH)
	list(APPEND SDK_COMPILER_FLAGS "/MP")
    # compiler path and triplet
    if (${WLIB_TARGET} STREQUAL "x86")
        set(CMAKE_CXX_COMPILER "${MSVC_CL_PATH}/../x86/cl.exe")
        set(CMAKE_C_COMPILER "${MSVC_CL_PATH}/../x86/cl.exe")
        set(VCPKG_TARGET_TRIPLET "x86-windows")
        set(VCPKG_HOST_TRIPLET "x86-windows")
    else()
        set(CMAKE_CXX_COMPILER "${MSVC_CL_PATH}/../x64/cl.exe")
        set(CMAKE_C_COMPILER "${MSVC_CL_PATH}/../x64/cl.exe")
        set(VCPKG_TARGET_TRIPLET "x64-windows")
        set(VCPKG_HOST_TRIPLET "x64-windows")
    endif()
    # vcpkg
    set(VCPKG_DIR "C:\\Users\\Administrator\\Desktop\\vcpkg")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_DIR}\\scripts\\buidlsystems\\vcpkg.cmake")
    message("CXX_COMPILER: ${CMAKE_CXX_COMPILER}")  
else()
	list(APPEND SDK_COMPILER_FLAGS "-fPIC") # "-fno-exceptions"
	list(APPEND SDK_COMPILER_FLAGS "-Wall" "-pedantic" "-Wextra") # "-Werror"
endif()



# if(WIN32)
    
#     set(CMAKE_C_FLAGS "/m32")
#     set(CMAKE_CXX_FLAGS "/m32")
# else()
#     set(CMAKE_C_FLAGS "-m32")
#     set(CMAKE_CXX_FLAGS "-m32")
# endif()

# lib 
set(proto_lib_name Protobuf)
set(lz4_lib_name lz4)
set(curl_lib_name curl)

set(proto_lib protobuf::libprotobuf)
set(lz4_lib_name lz4::lz4)
set(curl_lib_name CURL::libcurl)

if (WIN32 AND (${WLIB_TARGET} STREQUAL "x86"))
    set(proto_lib_name protobuf)
endif()


include(ExternalProject)
include(GNUInstallDirs)
include(cmake/build.cmake)


# print debug info
if(VERBOSE)
    # platform
    message("TARGET_OS: ${TARGET_OS}")
    message("WLIB_TARGET: ${WLIB_TARGET}")
    # option
    message("BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
    message("BUILD_SAMPLE: ${BUILD_SAMPLE}")
    message("BUILD_TESTS: ${BUILD_TESTS}")
    # proto
    message("PROTOS: ${PROTOS}")
    # target name
    message("STATIC_LIB: ${STATIC_LIB}")
    message("SHARED_LIB: ${SHARED_LIB}")
    message("PB_LIB: ${PB_LIB}")
    # srcs
    message("sls_sdk_srcs: ${sls_sdk_srcs}")
    # compiler flags
    message("SDK_COMPILER_FLAGS: ${SDK_COMPILER_FLAGS}")
endif()