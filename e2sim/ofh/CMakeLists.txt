#==================================================================================
#	Copyright 2025 Alexandre Huff
#
#   Licensed under the Apache License, Version 2.0 (the "License"),
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#==================================================================================
#

find_package(Protobuf CONFIG REQUIRED)

message(STATUS "Protobuf version ${Protobuf_VERSION}")

# Proto file
get_filename_component(signaling_proto "protos/signaling.proto" ABSOLUTE)
get_filename_component(signaling_proto_path "${signaling_proto}" PATH)

# Generated sources
set(signaling_proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/signaling.pb.cc")
set(signaling_proto_hdrs "${CMAKE_CURRENT_SOURCE_DIR}/signaling.pb.h")

add_custom_command(
      OUTPUT "${signaling_proto_srcs}" "${signaling_proto_hdrs}"
      COMMAND protobuf::protoc
      ARGS --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}"
        -I "${signaling_proto_path}"
        "${signaling_proto}"
      DEPENDS "${signaling_proto}")

add_library( ofh_objects OBJECT ofh_du.cpp ${signaling_proto_srcs} )

target_include_directories( ofh_objects PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>
  )

target_link_libraries( ofh_objects PRIVATE
  common_headers
  common_objects
  logger_objects
  )
