#==================================================================================
#      Copyright (c) 2020 AT&T Intellectual Property.
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

find_package(prometheus-cpp CONFIG REQUIRED)
find_package(cpprestsdk REQUIRED)

add_executable( e2sim e2node.cpp )

target_link_libraries( e2sim PRIVATE
    rc_helpers
    o1_objects
    base_objects
    rc_objects
    envman
    pistache
    ue_client
    common_objects
    ranfunction_objects
    messagerouting_objects
    sctp_objects
    encoding_objects
    encode_rc_objects
    e2model_objects
    procedure_objects
    )

target_link_libraries( e2sim PRIVATE cpprestsdk::cpprest OpenSSL::Crypto )

target_include_directories (e2sim PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
    # PRIVATE src
    )

# Prometheus
target_include_directories( e2sim PRIVATE /usr/include/prometheus )
target_include_directories( e2sim PRIVATE /usr/local/include/envman )
target_include_directories( e2sim PRIVATE /usr/local/include/envman/api )
target_include_directories( e2sim PRIVATE /usr/local/include/envman/model )
if(PROMETHEUS_CPP_ENABLE_PUSH)
    target_link_libraries(e2sim PRIVATE prometheus-cpp::push)
endif()
if(PROMETHEUS_CPP_ENABLE_PULL)
    target_link_libraries(e2sim PRIVATE prometheus-cpp::pull)
endif()

install(
    TARGETS e2sim
    DESTINATION ${install_bin}
)
