# libs and includes of third party libraries
# private: lz4 curl
# public: protobuf
# find package ptotobuf / curl 
find_package(Protobuf REQUIRED)
find_package(CURL REQUIRED)
include(${SDK_CMAKE_DIR}/Findlz4.cmake)
if (NOT lz4_FOUND)
    message(FATAL_ERROR "library lz4 not found}")
endif()

# protobuf generate
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTOS})
get_filename_component(PROTO_BINARY_DIR "${PROTO_HDRS}" PATH)

# static lib
add_library(${STATIC_LIB} STATIC
    ${sls_sdk_srcs}
    ${PROTO_SRCS})

set_target_properties(${STATIC_LIB}
    PROPERTIES
    LINKER_LANGUAGE CXX
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_include_directories(${STATIC_LIB}
        PUBLIC ${SDK_ROOT}
        PUBLIC ${SDK_ROOT}/include
        PUBLIC ${SDK_ROOT}/${sls_sdk_lz4_dir}
        PUBLIC ${PROTOBUF_INCLUDE_DIRS}
        PRIVATE ${CURL_INCLUDE_DIRS}
        PUBLIC ${PROTO_BINARY_DIR})
        
target_link_libraries(${STATIC_LIB} 
                PRIVATE CURL::libcurl
                PRIVATE lz4::lz4
                PRIVATE protobuf::libprotobuf)

target_compile_options(${STATIC_LIB} 
                PRIVATE "${SDK_COMPILER_FLAGS}")

target_link_options(${STATIC_LIB} 
                PRIVATE "${SDK_LINK_FLAGS}")

# sample
if(BUILD_SAMPLE)
	add_subdirectory(${SDK_ROOT}/example)
endif()

# test
if(BUILD_TESTS)
	add_subdirectory(${SDK_ROOT}/test)
endif()


# ==== install ====
# install static lib
install(TARGETS ${STATIC_LIB}
        ARCHIVE DESTINATION lib)

# install headers
install(FILES 
        ${sls_sdk_headers} 
        ${PROTO_HDRS}
        DESTINATION include)
install(DIRECTORY 
        ${sls_sdk_rapidjson_dir} 
        ${sls_sdk_lz4_dir}
        DESTINATION include)


if (INSTALL_SAMPLE)
    install(DIRECTORY ${SDK_ROOT}/example
        DESTINATION .)
endif()

if (INSTALL_PROTOBUF_HDRS)
    install(DIRECTORY ${PROTOBUF_INCLUDE_DIRS}/google
        DESTINATION include)
endif()

if (INSTALL_CURL_HDRS)
    install(DIRECTORY ${CURL_INCLUDE_DIRS}/curl
        DESTINATION include)
endif()

