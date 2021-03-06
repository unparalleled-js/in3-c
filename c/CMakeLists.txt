###############################################################################
# This file is part of the Incubed project.
# Sources: https://github.com/slockit/in3-c
# 
# Copyright (C) 2018-2020 slock.it GmbH, Blockchains LLC
# 
# 
# COMMERCIAL LICENSE USAGE
# 
# Licensees holding a valid commercial license may use this file in accordance 
# with the commercial license agreement provided with the Software or, alternatively, 
# in accordance with the terms contained in a written agreement between you and 
# slock.it GmbH/Blockchains LLC. For licensing terms and conditions or further 
# information please contact slock.it at in3@slock.it.
# 	
# Alternatively, this file may be used under the AGPL license as follows:
#    
# AGPL LICENSE USAGE
# 
# This program is free software: you can redistribute it and/or modify it under the
# terms of the GNU Affero General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later version.
#  
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
# PARTICULAR PURPOSE. See the GNU Affero General Public License for more details.
# [Permissions of this strong copyleft license are conditioned on making available 
# complete source code of licensed works and modifications, which include larger 
# works using a licensed work, under the same license. Copyright and license notices 
# must be preserved. Contributors provide an express grant of patent rights.]
# You should have received a copy of the GNU Affero General Public License along 
# with this program. If not, see <https://www.gnu.org/licenses/>.
###############################################################################

include("macro.cmake")
include("compiler.cmake")

#  build modules
add_subdirectory(src/third-party)


if (GCC_ANALYZER) 
  ADD_DEFINITIONS(-DGCC_ANALYZER=1)
  add_compile_options(-fanalyzer -Werror -Wno-nonnull-compare)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fanalyzer -Werror")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fanalyzer -Werror")
endif()

IF (TRANSPORTS)
    ADD_DEFINITIONS(-DTRANSPORTS)
    if (USE_CURL)
        ADD_DEFINITIONS(-DUSE_CURL)
        set(IN3_TRANSPORT ${IN3_TRANSPORT} transport_curl)
        if (CURL_BLOCKING)
            ADD_DEFINITIONS(-DCURL_BLOCKING)
        endif (CURL_BLOCKING)
    elseif(USE_WINHTTP)
       ADD_DEFINITIONS(-DUSE_WINHTTP)
       set(IN3_TRANSPORT ${IN3_TRANSPORT} transport_winhttp)
    else ()
        set(IN3_TRANSPORT ${IN3_TRANSPORT} transport_http)
    endif (USE_CURL)
    add_subdirectory(src/transport)
ENDIF (TRANSPORTS)

add_subdirectory(src/core)
add_subdirectory(src/verifier)
add_subdirectory(src/signer)
add_subdirectory(src/pay)
add_subdirectory(src/api)
add_subdirectory(src/cmd)
add_subdirectory(docs)

link_directories(${CMAKE_BINARY_DIR}/lib/)

# create the library
if (IN3_LIB)
   get_property(IN3_LIBS GLOBAL PROPERTY IN3_OBJECTS)

    # create the libraries
    add_library(in3_bundle STATIC ${IN3_LIBS} )
    add_library(in3_lib    SHARED ${IN3_LIBS} )
    set_target_properties(in3_bundle PROPERTIES OUTPUT_NAME "in3")
    set_target_properties(in3_lib PROPERTIES OUTPUT_NAME "in3")
    if( LEDGER_NANO)
        target_link_libraries(in3_lib ${IN3_TRANSPORT} ledger_signer) 
    else()
        target_link_libraries(in3_lib ${IN3_TRANSPORT} ) 
    endif()

    # install
    INSTALL(TARGETS in3_bundle
      DESTINATION "lib"
    )
    INSTALL(TARGETS in3_lib
        DESTINATION lib
        PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE)
    INSTALL (
        DIRECTORY ${CMAKE_SOURCE_DIR}/c/include/in3
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*")

endif()

