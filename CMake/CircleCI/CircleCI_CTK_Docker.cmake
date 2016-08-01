##############################################################################
#
# Library:   CTK
#
# Copyright 2010 Kitware Inc. 28 Corporate Drive,
# Clifton Park, NY, 12065, USA.
#
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
##############################################################################


set( CTK_SOURCE_DIR "/usr/src/CTK" )
set( CTK_BINARY_DIR "/usr/src/CTK-build" )

set( CTEST_SOURCE_DIRECTORY "${CTK_SOURCE_DIR}" )
set( CTEST_BINARY_DIRECTORY "${CTK_BINARY_DIR}/CTK-build" )

#############

set( SITE_CTEST_MODE "Experimental" ) # Experimental, Continuous, or Nightly

set( CTEST_CMAKE_GENERATOR "Unix Makefiles" ) # Ninja or Unix Makefiles

set( CTK_GIT_REPOSITORY "https://github.com/commontk/CTK.git" )

# Follow format for caps and components as given on CTK dashboard
set( CTEST_SITE "CircleCI_CTK" )

# Follow format for caps and components as given on CTK dashboard
set( SITE_PLATFORM "Ubuntu-64" )

# Use SITE_BUILD_TYPE specified by circle.yml
set( SITE_BUILD_TYPE "$ENV{SITE_BUILD_TYPE}" )
if( NOT( (SITE_BUILD_TYPE MATCHES "Debug") OR (SITE_BUILD_TYPE MATCHES "Release") ) )
  set( SITE_BUILD_TYPE "Debug" ) # Release, Debug
endif( NOT( (SITE_BUILD_TYPE MATCHES "Debug") OR (SITE_BUILD_TYPE MATCHES "Release") ) )

# Named SITE_BUILD_NAME
string( SUBSTRING $ENV{CIRCLE_SHA1} 0 7 commit )
set( what $ENV{CIRCLE_BRANCH} )
set( SITE_BUILD_NAME_SUFFIX _${commit}_${what} )

set( SITE_BUILD_NAME "CircleCI-${SITE_PLATFORM}-${SITE_BUILD_TYPE}${SITE_BUILD_NAME_SUFFIX}" )

set( CTEST_BUILD_NAME "${SITE_BUILD_NAME}-BuildTest-${SITE_CTEST_MODE}" )

###################

set( CTEST_CONFIGURATION_TYPE "${SITE_BUILD_TYPE}")
set( CMAKE_BUILD_TYPE "${SITE_BUILD_TYPE}")
set( BUILD_TESTING ON )
set( CTK_BUILD_EXAMPLES OFF )


###################


ctest_start( "${SITE_CTEST_MODE}" )

ctest_configure( BUILD "${CTK_BINARY_DIR}"
    SOURCE "${CTK_SOURCE_DIR}" )

ctest_build( BUILD ${CTK_BINARY_DIR} )

ctest_test( BUILD ${CTEST_BINARY_DIRECTORY} )

ctest_submit()
