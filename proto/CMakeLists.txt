PROTOBUF_GENERATE_CPP(PROTO_RPC_SERVICE_PTL_SRC PROTO_RPC_SERVICE_PTL_HDRS rpc_service_ptl.proto)
set_property(SOURCE ${PROTO_RPC_SERVICE_PTL_SRC} ${PROTO_RPC_SERVICE_PTL_HDRS} PROPERTY SKIP_AUTOGEN ON)

add_library(tinyrpc STATIC ${PROTO_RPC_SERVICE_PTL_SRC} ${PROTO_RPC_SERVICE_PTL_HDRS})
set_target_properties(tinyrpc PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(tinyrpc protobuf::libprotobuf)
target_include_directories(tinyrpc PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
