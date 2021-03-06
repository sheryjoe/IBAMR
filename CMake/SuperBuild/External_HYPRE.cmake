###########################################################################
#
#  Copyright (c) Kitware Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0.txt
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
###########################################################################
#
# HYPRE
#
set(HYPRE_REGISTER
"**************************************************************************\n"
"-- Please register to use HYPRE at:\n"
"--  https://computation.llnl.gov/casc/hypre/download/hypre-2.8.0b_reg.html\n"
"-- **************************************************************************")
# Make sure this file is included only once
get_filename_component(CMAKE_CURRENT_LIST_FILENAME ${CMAKE_CURRENT_LIST_FILE} NAME_WE)
if(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED)
  return()
endif()
set(${CMAKE_CURRENT_LIST_FILENAME}_FILE_INCLUDED 1)

# Sanity checks
if(DEFINED HYPRE_DIR AND NOT EXISTS ${HYPRE_DIR})
  message(FATAL_ERROR "HYPRE_DIR variable is defined but corresponds to non-existing directory")
endif()

set(HYPRE_DEPENDENCIES "OPENMPI;LAPACK")

# Include dependent projects if any
message(STATUS ${HYPRE_REGISTER})
CheckExternalProjectDependency(HYPRE)
set(proj HYPRE)

if(NOT DEFINED HYPRE_DIR)
  # Set CMake OSX variable to pass down the external project
  set(CMAKE_OSX_EXTERNAL_PROJECT_ARGS)
  if(APPLE)
    list(APPEND CMAKE_OSX_EXTERNAL_PROJECT_ARGS
      -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
      -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
  endif()

#     message(STATUS "Adding project:${proj}")
  if(BUILD_SHARED_LIBS)
    set(SHARED_LIB_CONF --enable-shared --disable-static)
    # Hack -- Delete build directory to triger rebuild.
    if(NOT ${proj}_SHARED_BUILD AND EXISTS ${IBAMR_BINARY_DIR}/SuperBuild/${proj})
      file(REMOVE_RECURSE ${IBAMR_BINARY_DIR}/SuperBuild/${proj})
      file(REMOVE_RECURSE ${IBAMR_BINARY_DIR}/CMake/${proj}${ep_suffix})
    endif()

    set(${proj}_SHARED_BUILD TRUE CACHE INTERNAL "" FORCE)
  else()
    set(SHARED_LIB_CONF --enable-static --disable-shared)

    if(${proj}_SHARED_BUILD AND EXISTS ${IBAMR_BINARY_DIR}/SuperBuild/${proj})
      file(REMOVE_RECURSE ${IBAMR_BINARY_DIR}/SuperBuild/${proj})
      file(REMOVE_RECURSE ${IBAMR_BINARY_DIR}/CMake/${proj}${ep_suffix})
    endif()

    set(${proj}_SHARED_BUILD FALSE CACHE INTERNAL "" FORCE)
  endif()

  ExternalProject_Add(${proj}
    SOURCE_DIR ${IBAMR_BINARY_DIR}/SuperBuild/${proj}
    BINARY_DIR ${IBAMR_BINARY_DIR}/SuperBuild/${proj}/src
    PREFIX CMake/${proj}${ep_suffix}
    URL ${HYPRE_URL}/${HYPRE_GZ}
    URL_MD5 ${HYPRE_MD5}
    UPDATE_COMMAND ""
    INSTALL_COMMAND make install
    CONFIGURE_COMMAND ./configure
      CC=${ep_install_dir}/bin/mpicc
      CXX=${ep_install_dir}/bin/mpicxx
      FC=${ep_install_dir}/bin/mpif90
      "CFLAGS=${ep_common_c_flags} ${ep_build_type_c_flags}"
      "CXXFLAGS=${ep_common_cxx_flags} ${ep_build_type_cxx_flags}"
      "FCFLAGS=${CMAKE_Fortran_FLAGS} ${ep_build_type_fortran_flags}"
      "FFLAGS=${CMAKE_Fortran_FLAGS} ${ep_build_type_fortran_flags}"
      "LDFLAGS=-L${ep_install_dir}/lib -Wl,-rpath,${ep_install_dir}/lib"
      --libdir=${ep_install_dir}/lib
      --prefix=${ep_install_dir}
      --with-MPI-include=${ep_install_dir}/include
      --with-MPI-libs=nsl;aio;rt
      --with-blas-lib-dirs=${LAPACK_DIR}
      --with-lapack-lib-dirs=${LAPACK_DIR}
      --with-lapack-libs=lapack
      --with-blas-libs=blas
      --without-babel
      --without-mli
      --without-fei
      --without-superlu
      ${SHARED_LIB_CONF}
    LOG_DOWNLOAD 1
    LOG_CONFIGURE 1
    LOG_TEST 1
    LOG_BUILD 1
    LOG_INSTALL 1
    DEPENDS
      ${HYPRE_DEPENDENCIES}
    )
  set(${proj}_DIR ${ep_install_dir})

else()
  EmptyExternalProject(${proj} "${proj_DEPENDENCIES}")
endif()

list(APPEND IBAMR_SUPERBUILD_EP_ARGS
  -DHYPRE_DIR:PATH=${ep_install_dir}
  -DHYPRE_INCLUDE_PATH:PATH=${ep_install_dir}/include)

list(APPEND INCLUDE_PATHS ${ep_install_dir}/include)
list(APPEND LIBRARY_PATHS ${ep_install_dir}/lib)
list(APPEND EXTERNAL_LIBRARIES -lHYPRE)
