# This is a cmake 2.8.12 stock module modified for use with htcondor

# - Find python interpreter
# This module finds if Python3 interpreter is installed and determines where the
# executables are. This code sets the following variables:
#
#  PYTHON3INTERP_FOUND         - Was the Python executable found
#  PYTHON3_EXECUTABLE          - path to the Python interpreter
#
#  PYTHON3_VERSION_STRING      - Python version found e.g. 2.5.2
#  PYTHON3_VERSION_MAJOR       - Python major version found e.g. 2
#  PYTHON3_VERSION_MINOR       - Python minor version found e.g. 5
#  PYTHON3_VERSION_PATCH       - Python patch version found e.g. 2
#
# The Python3_ADDITIONAL_VERSIONS variable can be used to specify a list of
# version numbers that should be taken into account when searching for Python3.
# You need to set this variable before calling find_package(Python3Interp).

#=============================================================================
# Copyright 2005-2010 Kitware, Inc.
# Copyright 2011 Bjoern Ricks <bjoern.ricks@gmail.com>
# Copyright 2012 Rolf Eike Beer <eike@sf-mail.de>
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

unset(_Python3_NAMES)

set(_PYTHON3_VERSIONS 3.3 3.2 3.1 3.0)

if(Python3Interp_FIND_VERSION)
    if(Python3Interp_FIND_VERSION MATCHES "^[0-9]+\\.[0-9]+(\\.[0-9]+.*)?$")
        string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" _PYTHON_FIND_MAJ_MIN "${Python3Interp_FIND_VERSION}")
        string(REGEX REPLACE "^([0-9]+).*" "\\1" _PYTHON_FIND_MAJ "${_PYTHON_FIND_MAJ_MIN}")
        list(APPEND _Python3_NAMES python${_PYTHON_FIND_MAJ_MIN} python${_PYTHON_FIND_MAJ})
        unset(_PYTHON3_FIND_OTHER_VERSIONS)
        if(NOT Python3Interp_FIND_VERSION_EXACT)
            foreach(_PYTHON3_V ${_PYTHON3${_PYTHON3_FIND_MAJ}_VERSIONS})
                if(NOT _PYTHON3_V VERSION_LESS _PYTHON3_FIND_MAJ_MIN)
                    list(APPEND _PYTHON3_FIND_OTHER_VERSIONS ${_PYTHON_V})
                endif()
             endforeach()
        endif()
        unset(_PYTHON3_FIND_MAJ_MIN)
        unset(_PYTHON3_FIND_MAJ)
    else()
        list(APPEND _Python3_NAMES python${Python3Interp_FIND_VERSION})
        set(_PYTHON_FIND_OTHER_VERSIONS ${_PYTHON3${Python3Interp_FIND_VERSION}_VERSIONS})
    endif()
else()
    set(_PYTHON3_FIND_OTHER_VERSIONS ${_PYTHON3_VERSIONS} )
endif()

list(APPEND _Python3_NAMES python3)

# Search for the current active python version first
find_program(PYTHON3_EXECUTABLE NAMES ${_Python3_NAMES})

# Set up the versions we know about, in the order we will search. Always add
# the user supplied additional versions to the front.
set(_Python3_VERSIONS
  ${Python3_ADDITIONAL_VERSIONS}
  ${_PYTHON3_FIND_OTHER_VERSIONS}
  )

unset(_PYTHON3_FIND_OTHER_VERSIONS)
unset(_PYTHON3_VERSIONS)

# Search for newest python version if python executable isn't found
if(NOT PYTHON3_EXECUTABLE)
    foreach(_CURRENT_VERSION ${_Python3_VERSIONS})
      set(_Python3_NAMES python${_CURRENT_VERSION})
      if(WIN32)
        list(APPEND _Python3_NAMES python)
      endif()
      find_program(PYTHON3_EXECUTABLE
        NAMES ${_Python3_NAMES}
        PATHS [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\${_CURRENT_VERSION}\\InstallPath]
        )
    endforeach()
endif()

# determine python version string
if(PYTHON3_EXECUTABLE)
    execute_process(COMMAND "${PYTHON3_EXECUTABLE}" -c
                            "import sys; sys.stdout.write(';'.join([str(x) for x in sys.version_info[:3]]))"
                    OUTPUT_VARIABLE _VERSION
                    RESULT_VARIABLE _PYTHON3_VERSION_RESULT
                    ERROR_QUIET)
    if(NOT _PYTHON3_VERSION_RESULT)
        string(REPLACE ";" "." PYTHON3_VERSION_STRING "${_VERSION}")
        list(GET _VERSION 0 PYTHON3_VERSION_MAJOR)
        list(GET _VERSION 1 PYTHON3_VERSION_MINOR)
        list(GET _VERSION 2 PYTHON3_VERSION_PATCH)
        if(PYTHON3_VERSION_PATCH EQUAL 0)
            # it's called "Python 2.7", not "2.7.0"
            string(REGEX REPLACE "\\.0$" "" PYTHON3_VERSION_STRING "${PYTHON3_VERSION_STRING}")
        endif()
    endif()
    unset(_PYTHON3_VERSION_RESULT)
    unset(_VERSION)
endif()

# handle the QUIETLY and REQUIRED arguments and set PYTHON3INTERP_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Python3Interp REQUIRED_VARS PYTHON3_EXECUTABLE VERSION_VAR PYTHON3_VERSION_STRING)

mark_as_advanced(PYTHON3_EXECUTABLE)
