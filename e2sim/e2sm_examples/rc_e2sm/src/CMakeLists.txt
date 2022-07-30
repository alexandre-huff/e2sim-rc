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

add_executable( e2sim-rc rc_callbacks.cpp encode_rc.cpp )

add_dependencies( e2sim-rc asn1_objects )

target_include_directories(e2sim-rc PRIVATE ../ASN1c /usr/local/include/e2sim)

target_link_libraries( e2sim-rc e2sim )
target_link_libraries( e2sim-rc asn1_objects )
target_link_libraries( e2sim-rc pthread)

install(
    TARGETS e2sim-rc
    DESTINATION ${install_bin}
)

