#==================================================================================
#	Copyright (c) 2020 AT&T Intellectual Property.
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

add_executable( e2sim-rc rc_callbacks.cpp encode_rc.cpp )

target_link_libraries( e2sim-rc PRIVATE e2ap_asn1_objects
                                        base_objects
                                        logger_objects
                                        encoding_objects
                                        def_objects
                                        sctp_objects
                                        messagerouting_objects )
target_link_libraries( e2sim-rc PRIVATE e2sm_rc_asn1_objects )
target_link_libraries( e2sim-rc PRIVATE pthread )

# Prometheus
target_include_directories( e2sim-rc PRIVATE /usr/include/prometheus )
if(PROMETHEUS_CPP_ENABLE_PUSH)
    target_link_libraries(e2sim-rc PRIVATE prometheus-cpp::push)
endif()
if(PROMETHEUS_CPP_ENABLE_PULL)
    target_link_libraries(e2sim-rc PRIVATE prometheus-cpp::pull)
endif()

install(
    TARGETS e2sim-rc
    DESTINATION ${install_bin}
)

