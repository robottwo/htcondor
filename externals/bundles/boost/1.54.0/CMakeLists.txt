 ###############################################################
 #
 # Copyright 2011 Red Hat, Inc.
 #
 # Licensed under the Apache License, Version 2.0 (the "License"); you
 # may not use this file except in compliance with the License.  You may
 # obtain a copy of the License at
 #
 #    http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 ###############################################################

# Search for system's boost libraries.
if (NOT WINDOWS)

    set(Boost_USE_MULTITHREADED ON)

    if (NOT PROPER)
        set (BOOST_COMPONENTS thread system)
    else()

	if (BUILD_TESTING)
	  set (BOOST_COMPONENTS unit_test_framework ${BOOST_COMPONENTS})
	endif()
        if (WITH_PYTHON_BINDINGS)
          set (BOOST_COMPONENTS python ${BOOST_COMPONENTS})
        endif()

    endif()

    # The following is helpful if you are trying to debug cmake find module
    #  set (Boost_DEBUG TRUE)
    message (STATUS "Boost components: ${BOOST_COMPONENTS}" )
    find_package( Boost 1.33.1 COMPONENTS ${BOOST_COMPONENTS} )
    if (Boost_PYTHON_LIBRARY)
        set(Boost_PYTHON2_LIBRARY "${Boost_PYTHON_LIBRARY}")
        set(Boost_PYTHON_LIBRARY)
    endif()

    # always prefer system libraries where possible
    if(Boost_FOUND)

	if (NOT PREFER_CPP11 AND NOT PREFER_TR1)
	  check_cxx_source_compiles("
	  #include <boost/unordered_map.hpp>
	  int main() {
	      boost::unordered_map<int,int> foo;
	      return 0;
	  }
	  " HAVE_BOOST_UNORDER_MAP )

	  # we don't have any unordered map?
	  if (NOT HAVE_BOOST_UNORDER_MAP)
	    message (STATUS "WARNING did not detect unordered_map")
	    set ( SYSTEM_NOT_UP_TO_SNUFF TRUE )
	  endif()

	endif()

        # set vars b/c we are good to go.
        if (NOT SYSTEM_NOT_UP_TO_SNUFF)
            append_var (CONDOR_EXTERNAL_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
            set (BOOST_VER ${Boost_VERSION})
            set (BOOST_INCLUDE ${Boost_INCLUDE_DIRS})
            set (BOOST_LD ${Boost_LIBRARY_DIRS})
        endif()

    endif(Boost_FOUND)

    if (NOT PROPER)

        set(BUILD_OPTIONS --layout=system variant=release cxxflags=-fPIC linkflags=-fPIC)

	if (DARWIN)
		if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
			set(TOOLSET clang-darwin)
		else()
			set(TOOLSET darwin)
		endif()
		if (cxx_11)
			set(BUILD_OPTIONS ${BUILD_OPTIONS} "cxxflags=-std=c++11 -stdlib=libc++ -fPIC" "linkflags=-std=c++11 -stdlib=libc++ -fPIC")
		endif()
	else()
		set(TOOLSET gcc)
	endif()

		set(INCLUDE_LOC include)
        set(BOOTSTRAP ./bootstrap.sh )
        set(EXEC ./)

	set(BOOST_FILENAME boost_1_54_0)
	condor_pre_external( BOOST ${BOOST_FILENAME} "lib;${INCLUDE_LOC}" "done")

	set(BOOST_MIN_BUILD_DEP --with-thread --with-system --with-test --with-python)
	set(BOOST_PATCH echo "nothing")
	set(BOOST_INSTALL echo "nothing")
	unset(BOOST_INCLUDE)

	if (PYTHON2INTERP_FOUND AND PYTHON2LIBS_FOUND)
		message(STATUS "Bootstrapping python for boost")
		set(Boost_PYTHON2_LIBRARY "external" PARENT_SCOPE)
		set(BOOTSTRAP_PYTHON2_OPTION "--with-python=${PYTHON2_EXECUTABLE}")
		set(BUILD_OPTIONS ${BUILD_OPTIONS} "python=${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
	else ()
		set(BOOTSTRAP_PYTHON2_OPTION )
	endif()
	if (BUILD_PYTHON3)
		file(WRITE "${BOOST_STAGE}/user-config.jam" "using python : ${PYTHON3_VERSION_MAJOR}.${PYTHON3_VERSION_MINOR} : ${PYTHON3_EXECUTABLE} : ${PYTHON3_INCLUDE_DIR} ;\n")
	else ()
		file(WRITE "${BOOST_STAGE}/user-config.jam" "")
	endif()
	# See: https://svn.boost.org/trac/boost/ticket/8973
	if (LINUX)
		set(BOOST_PATCH patch -N -p2 -i ${CMAKE_CURRENT_SOURCE_DIR}/boost.patch)
	else ()
		set(BOOST_PATCH echo "nothing")
	endif()

	# so the idea of privately staging boost is silly.
	ExternalProject_Add(boost
		#-- Download Step ----------
		 DOWNLOAD_DIR ${BOOST_STAGE}/dl
		 URL ${EXTERNALS_SOURCE_URL}/${BOOST_FILENAME}.tar.gz
		#--Patch step ----------
		 PATCH_COMMAND ${BOOST_PATCH}
		#--Configure step ----------
		 CONFIGURE_COMMAND ${BOOTSTRAP} --prefix=${BOOST_INSTALL_LOC} ${BOOTSTRAP_PYTHON2_OPTION} ${CMD_TERM}
			cmake -E copy "${BOOST_STAGE}/user-config.jam" tools/build/v2/user-config.jam ${CMD_TERM}
			echo "Configure complete"
		#--Build Step ----------
		BUILD_COMMAND ${EXEC}bjam ${BOOST_MIN_BUILD_DEP} --prefix=${BOOST_INSTALL_LOC} --libdir=${BOOST_INSTALL_LOC}/lib define=BOOST_HAS_THREADS ${BUILD_OPTIONS} toolset=${TOOLSET} link=static install
		BUILD_IN_SOURCE 1
		#--install Step ----------
		INSTALL_DIR ${BOOST_INSTALL_LOC}
		INSTALL_COMMAND touch ${BOOST_INSTALL_LOC}/done )

	condor_post_external( boost "include" "lib" )

    endif( NOT PROPER )

else( NOT WINDOWS )

# Search for system's boost libraries.
condor_pre_external( BOOST boost-1.54.0 "lib;boost" "done")

if(BOOST_DOWNLOAD_WIN)
  set (BOOST_CONFIGURE echo "Nothing to configure")
	set (BOOST_MAKE echo "No make necessary")
  set (BOOST_INSTALL tar -pczf boost.tar.gz boost/ && tar -zxvf boost.tar.gz -C "${BOOST_INSTALL_LOC}/boost/" && cp *.lib ${BOOST_INSTALL_LOC}/lib && touch ${BOOST_INSTALL_LOC}/done)
  
  ExternalProject_Add(boost
    #-- Download Step ----------
    DOWNLOAD_DIR ${BOOST_STAGE}/dl
    URL ${EXTERNALS_SOURCE_URL}/${BOOST_DOWNLOAD_WIN}
    CONFIGURE_COMMAND ${BOOST_CONFIGURE}
    #--install Step ----------
    BUILD_COMMAND ${BOOST_MAKE}
    BUILD_IN_SOURCE 1
    INSTALL_DIR ${BOOST_INSTALL_LOC}
    INSTALL_COMMAND ${BOOST_INSTALL})
  
  set(BOOST_ROOT ${BOOST_INSTALL_LOC} PARENT_SCOPE)
  set(BOOST_SHORTVER 1_54 PARENT_SCOPE)
  condor_post_external( boost "boost" "lib" )
endif()

endif( NOT WINDOWS )

# update configure information
if (BOOST_VER)
	message (STATUS "external configured (BOOST_INCLUDE=${BOOST_INCLUDE}) version:(${BOOST_VER}) link directories (${BOOST_LD})")
	set( HAVE_EXT_BOOST ON PARENT_SCOPE )
	set( BOOST_VER ${BOOST_VER} PARENT_SCOPE )
	set( BOOST_INCLUDE ${BOOST_INCLUDE} PARENT_SCOPE )
	set( BOOST_LD ${BOOST_LD} PARENT_SCOPE )
	
	if (Boost_PYTHON2_LIBRARY)
        set( Boost_PYTHON2_LIBRARY ${Boost_PYTHON2_LIBRARY} PARENT_SCOPE )
        dprint("Found Python 2.x library: ${Boost_PYTHON2_LIBRARY} ")
    endif()
	
else(BOOST_VER)
	message (WARNING "**boost not found **.")
endif(BOOST_VER)
