cmake_minimum_required (VERSION 3.8)

project("grpc-limits-mvr")

add_executable(server)

target_sources(server PRIVATE
	main.cpp
)

# option defaults
set(PROTO_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/protogen)
set(PROTOBUF_SUBMODULE_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/protobuf-install)


target_include_directories(server PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${PROTO_GENERATED_DIR}
)

# start gRPC
add_subdirectory(thirdparty/grpc)
file(MAKE_DIRECTORY ${PROTOBUF_SUBMODULE_INSTALL_DIR})
install(
    FILES thirdparty/grpc/third_party/protobuf/cmake_install.cmake
    DESTINATION ${PROTOBUF_SUBMODULE_INSTALL_DIR}
)
set(GRPC_PROTOC_PLUGIN $<TARGET_FILE:grpc_cpp_plugin>)

target_link_libraries(server
	grpc
	grpc++
	grpc++_reflection
	protobuf::libprotoc
	protobuf::libprotobuf
)
# end gRPC


# dir to hold protobuf generated code
file(MAKE_DIRECTORY ${PROTO_GENERATED_DIR})

# run protoc compiler to generate *.pb.cc + h
add_custom_command(
    OUTPUT ${PROTO_GENERATED_DIR}/service.pb.cc
           ${PROTO_GENERATED_DIR}/service.pb.h
    COMMAND protoc
    ARGS --cpp_out=${PROTO_GENERATED_DIR}
	     service.proto
    DEPENDS service.proto protoc
    WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running C++ protocol buffer compiler on service.proto"
    VERBATIM
)

# run protoc compiler to generate *.grpc.pb.cc + h
add_custom_command(
    OUTPUT ${PROTO_GENERATED_DIR}/service.grpc.pb.cc
           ${PROTO_GENERATED_DIR}/service.grpc.pb.h
    COMMAND protoc
    ARGS --grpc_out=${PROTO_GENERATED_DIR}
	     --plugin=protoc-gen-grpc=${GRPC_PROTOC_PLUGIN}
	     service.proto
    DEPENDS service.proto protoc grpc_protoc_plugin
    WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running gRPC C++ protocol buffer compiler on service.proto with ${GRPC_PROTOC_PLUGIN}"
    VERBATIM
)

target_sources(server PUBLIC
	${PROTO_GENERATED_DIR}/service.pb.cc
	${PROTO_GENERATED_DIR}/service.grpc.pb.cc
)

