# MODIFIED from cmake 2.8.12 for use with HTCondor

# - Find python3 libraries
# This module finds if Python is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#
#  PYTHON3LIBS_FOUND           - have the Python libs been found
#  PYTHON3_LIBRARIES           - path to the python library
#  PYTHON3_INCLUDE_PATH        - path to where Python.h is found (deprecated)
#  PYTHON3_INCLUDE_DIRS        - path to where Python.h is found
#  PYTHON3_DEBUG_LIBRARIES     - path to the debug library (deprecated)
#  PYTHON3LIBS_VERSION_STRING  - version of the Python libs found
#
# The Python_ADDITIONAL_VERSIONS variable can be used to specify a list of
# version numbers that should be taken into account when searching for Python.
# You need to set this variable before calling find_package(PythonLibs).
#
# If you'd like to specify the installation of Python to use, you should modify
# the following cache variables:
#  PYTHON_LIBRARY             - path to the python library
#  PYTHON_INCLUDE_DIR         - path to where Python.h is found

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

include(CMakeFindFrameworks)
# Search for the python framework on Apple.
CMAKE_FIND_FRAMEWORKS(Python3)

set(_PYTHON3_VERSIONS 3.3 3.2 3.1 3.0)

if(Python3Libs_FIND_VERSION)
    if(Python3Libs_FIND_VERSION MATCHES "^[0-9]+\\.[0-9]+(\\.[0-9]+.*)?$")
        string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" _PYTHON3_FIND_MAJ_MIN "${Python3Libs_FIND_VERSION}")
        string(REGEX REPLACE "^([0-9]+).*" "\\1" _PYTHON3_FIND_MAJ "${_PYTHON3_FIND_MAJ_MIN}")
        unset(_PYTHON3_FIND_OTHER_VERSIONS)
        if(Python3Libs_FIND_VERSION_EXACT)
            if(_PYTHON3_FIND_MAJ_MIN STREQUAL Python3Libs_FIND_VERSION)
                set(_PYTHON3_FIND_OTHER_VERSIONS "${Python3Libs_FIND_VERSION}")
            else()
                set(_PYTHON3_FIND_OTHER_VERSIONS "${Python3Libs_FIND_VERSION}" "${_PYTHON3_FIND_MAJ_MIN}")
            endif()
        else()
            foreach(_PYTHON3_V ${_PYTHON3${_PYTHON3_FIND_MAJ}_VERSIONS})
                if(NOT _PYTHON3_V VERSION_LESS _PYTHON3_FIND_MAJ_MIN)
                    list(APPEND _PYTHON3_FIND_OTHER_VERSIONS ${_PYTHON3_V})
                endif()
             endforeach()
        endif()
        unset(_PYTHON3_FIND_MAJ_MIN)
        unset(_PYTHON3_FIND_MAJ)
    else()
        set(_PYTHON3_FIND_OTHER_VERSIONS ${_PYTHON3${Python3Libs_FIND_VERSION}_VERSIONS})
    endif()
else()
    set(_PYTHON3_FIND_OTHER_VERSIONS ${_PYTHON3_VERSIONS} )
endif()

# Set up the versions we know about, in the order we will search. Always add
# the user supplied additional versions to the front.
# If FindPythonInterp has already found the major and minor version, 
# insert that version between the user supplied versions and the stock
# version list. 
if(DEFINED PYTHON3_VERSION_MAJOR AND DEFINED PYTHON3_VERSION_MINOR)
  set(_Python3_VERSIONS
    ${Python3_ADDITIONAL_VERSIONS}
    ${PYTHON3_VERSION_MAJOR}.${PYTHON3_VERSION_MINOR}
    ${_PYTHON3_FIND_OTHER_VERSIONS}
    )
else()
  set(_Python3_VERSIONS
    ${Python3_ADDITIONAL_VERSIONS}
    ${_PYTHON3_FIND_OTHER_VERSIONS}
    )
endif()

unset(_PYTHON3_FIND_OTHER_VERSIONS)
unset(_PYTHON3_VERSIONS)

foreach(_CURRENT_VERSION ${_Python3_VERSIONS})
  string(REPLACE "." "" _CURRENT_VERSION_NO_DOTS ${_CURRENT_VERSION})
  if(WIN32)
    find_library(PYTHON3_DEBUG_LIBRARY
      NAMES python${_CURRENT_VERSION_NO_DOTS}_d python
      PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs/Debug
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs/Debug
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
      )
  endif()

  find_library(PYTHON3_LIBRARY
    NAMES
    python${_CURRENT_VERSION_NO_DOTS}
    python${_CURRENT_VERSION}mu
    python${_CURRENT_VERSION}m
    python${_CURRENT_VERSION}u
    python${_CURRENT_VERSION}
    PATHS
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/libs
    # Avoid finding the .dll in the PATH.  We want the .lib.
    NO_SYSTEM_ENVIRONMENT_PATH
  )
  # Look for the static library in the Python config directory
  find_library(PYTHON3_LIBRARY
    NAMES python${_CURRENT_VERSION_NO_DOTS} python${_CURRENT_VERSION}
    # Avoid finding the .dll in the PATH.  We want the .lib.
    NO_SYSTEM_ENVIRONMENT_PATH
    # This is where the static library is usually located
    PATH_SUFFIXES python${_CURRENT_VERSION}/config
  )

  # For backward compatibility, honour value of PYTHON_INCLUDE_PATH, if
  # PYTHON_INCLUDE_DIR is not set.
  if(DEFINED PYTHON3_INCLUDE_PATH AND NOT DEFINED PYTHON3_INCLUDE_DIR)
    set(PYTHON3_INCLUDE_DIR "${PYTHON3_INCLUDE_PATH}" CACHE PATH
      "Path to where Python.h is found" FORCE)
  endif()

  set(PYTHON3_FRAMEWORK_INCLUDES)
  if(Python3_FRAMEWORKS AND NOT PYTHON3_INCLUDE_DIR)
    foreach(dir ${Python3_FRAMEWORKS})
      set(PYTHON3_FRAMEWORK_INCLUDES ${PYTHON3_FRAMEWORK_INCLUDES}
        ${dir}/Versions/${_CURRENT_VERSION}/include/python${_CURRENT_VERSION})
    endforeach()
  endif()

  find_path(PYTHON3_INCLUDE_DIR
    NAMES Python.h
    PATHS
      ${PYTHON3_FRAMEWORK_INCLUDES}
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
      [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]/include
    PATH_SUFFIXES
      python${_CURRENT_VERSION}mu
      python${_CURRENT_VERSION}m
      python${_CURRENT_VERSION}u
      python${_CURRENT_VERSION}
  )

  # For backward compatibility, set PYTHON_INCLUDE_PATH.
  set(PYTHON3_INCLUDE_PATH "${PYTHON3_INCLUDE_DIR}")

  if(PYTHON3_INCLUDE_DIR AND EXISTS "${PYTHON3_INCLUDE_DIR}/patchlevel.h")
    file(STRINGS "${PYTHON3_INCLUDE_DIR}/patchlevel.h" python_version_str
         REGEX "^#define[ \t]+PY_VERSION[ \t]+\"[^\"]+\"")
    string(REGEX REPLACE "^#define[ \t]+PY_VERSION[ \t]+\"([^\"]+)\".*" "\\1"
                         PYTHON3LIBS_VERSION_STRING "${python_version_str}")
    unset(python_version_str)
  endif()

  if(PYTHON3_LIBRARY AND PYTHON3_INCLUDE_DIR)
    break()
  endif()
endforeach()

mark_as_advanced(
  PYTHON3_DEBUG_LIBRARY
  PYTHON3_LIBRARY
  PYTHON3_INCLUDE_DIR
)

# We use PYTHON_INCLUDE_DIR, PYTHON_LIBRARY and PYTHON_DEBUG_LIBRARY for the
# cache entries because they are meant to specify the location of a single
# library. We now set the variables listed by the documentation for this
# module.
set(PYTHON3_INCLUDE_DIRS "${PYTHON3_INCLUDE_DIR}")
set(PYTHON3_DEBUG_LIBRARIES "${PYTHON3_DEBUG_LIBRARY}")

# These variables have been historically named in this module different from
# what SELECT_LIBRARY_CONFIGURATIONS() expects.
set(PYTHON3_LIBRARY_DEBUG "${PYTHON3_DEBUG_LIBRARY}")
set(PYTHON3_LIBRARY_RELEASE "${PYTHON3_LIBRARY}")
include(SelectLibraryConfigurations)
SELECT_LIBRARY_CONFIGURATIONS(PYTHON3)
# SELECT_LIBRARY_CONFIGURATIONS() sets ${PREFIX}_FOUND if it has a library.
# Unset this, this prefix doesn't match the module prefix, they are different
# for historical reasons.
unset(PYTHON3_FOUND)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Python3Libs
                                  REQUIRED_VARS PYTHON3_LIBRARIES PYTHON3_INCLUDE_DIRS
                                  VERSION_VAR PYTHON3LIBS_VERSION_STRING)

