


# shared lib

add_library(${SHARED_LIB} SHARED
            ${sls_sdk_srcs}
            ${PROTO_SRCS})

set_target_properties(${SHARED_LIB}
    PROPERTIES
    LINKER_LANGUAGE CXX
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_include_directories(${SHARED_LIB}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CURL_INCLUDE_DIRS}
    PUBLIC ${PROTOBUF_INCLUDE_DIRS}
    PUBLIC ${PROTO_BINARY_DIR})

target_link_libraries(${SHARED_LIB} 
    PRIVATE CURL::libcurl
    PUBLIC lz4::lz4
    PUBLIC protobuf::libprotobuf)


# install shared lib

install(TARGETS ${SHARED_LIB}
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# if (WIN32 AND PUBLISH)
#     # shared dlls
#     install(FILES
#             $<TARGET_RUNTIME_DLLS:${SHARED_LIB}>
#             DESTINATION lib)
# endif()
