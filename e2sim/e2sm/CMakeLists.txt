#==================================================================================
#	Copyright 2024 Alexandre Huff
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

# each binary is built from a subset
add_subdirectory( e2model )
add_subdirectory( rc )

add_library( e2sm_objects OBJECT e2sm.cpp submgr.cpp )

target_link_libraries( e2sm_objects PRIVATE logger_objects
    procedure_objects
    def_objects
    e2ap_asn1_objects
    e2model_objects
    common_objects
    )

target_include_directories (e2sm_objects PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
    # PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../e2ap/src/procedures>
    )

if( DEV_PKG )
    install( FILES
        e2sm.hpp
        subaction.hpp
        submgr.hpp
        DESTINATION ${install_inc}
    )
endif()
