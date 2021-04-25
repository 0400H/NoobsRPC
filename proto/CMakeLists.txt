PROTOBUF_GENERATE_CPP(PROTO_RPC_SERVICE_PTL_SRC PROTO_RPC_SERVICE_PTL_HDRS rpc_service_ptl.proto)

add_library(proto STATIC ${PROTO_RPC_SERVICE_PTL_SRC} ${PROTO_RPC_SERVICE_PTL_HDRS})
set_target_properties(proto PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(proto protobuf::libprotobuf)
target_include_directories(proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
