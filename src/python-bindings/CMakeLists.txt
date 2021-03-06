
if(WINDOWS)
  if(WITH_PYTHON_BINDINGS AND PYTHONLIBS_FOUND)
    #only for VS2012 and above
    if(NOT (MSVC_VERSION LESS 1700))
        #only for boost-1.54.0
        IF("${BOOST_VER}" STREQUAL "boost-1.54.0")
            message(STATUS "===> Generating python-bindings")
      
            include_directories(${PYTHON_INCLUDE_DIRS} ${BOOST_INCLUDE})
            link_directories(${BOOST_LD})

            set(PYTHON_LIBRARY_BOOST_ONE ${BOOST_LD}/boost_python-vc110-mt-1_54.lib)
            set(PYTHON_LIBRARY_BOOST_TWO ${BOOST_LD}/libboost_python-vc110-mt-1_54.lib)

            # python27_d.lib allows for Debug builds to compile, but the bindings don't work unless you
            # happen to have EXACTLY the same build of python as the one we used to make python27_d.dll
            # It isn't needed at all for RelWithDebugInfo builds.  When you build Debug, the mere act of including <pyconfig.h>
            # will generate a reference to the python27_d library in the object files, so it doesn't need
            # to be passed to the linker, it just needs to exist somewhere in the LIBPATH
            # set(PYTHON_DEBUG_LIBRARY ${BOOST_LD}/python27_d.lib)

            GET_FILENAME_COMPONENT(PYTHON_LIBRARY_FILENAME ${PYTHON_LIBRARIES} NAME)
            STRING(REGEX REPLACE ".*python([0-9]+[.]?[0-9]+).*" "\\1" _PYTHON_VERSION ${PYTHON_LIBRARY_FILENAME})
            if ( ${PACKAGE_VERSION} MATCHES "([0-9]+)[.]([0-9]+)[.]([0-9]+)" )
              set( PYCLASSAD_LIB_NAME "pyclassad${_PYTHON_VERSION}_${CMAKE_MATCH_1}_${CMAKE_MATCH_2}_${CMAKE_MATCH_3}" )
            else()
              message(FATAL_ERROR "Can't determine HTCondor version!")
            endif()

            # generate a dummy python27_d.dll that will allow HTCondor to build _DEBUG but will NOT result in
            # working python bindings.  If you have a module what includes <pyconfig.h> directly rather than using
            # the python_bindings_common.h wrapper, then dropping the python27_d.lib file produced here in the LIBPATH
            # will prevent the python-bindings build from failing, but the bindings won't work.  In order to get them
            # to work you would have to use python_bindings_common.h, or build the python27 sources with _DEBUG defined.
            add_library( python27_d SHARED "debug.WINDOWS.cpp" )
            target_link_libraries( python27_d ${PYTHON_LIBRARIES} )

            condor_shared_lib( pyclassad classad.cpp classad_wrapper.h exprtree_wrapper.h )
            target_link_libraries( pyclassad ${CLASSADS_FOUND} ${PYTHON_LIBRARY_BOOST_ONE} ${PYTHON_LIBRARY_BOOST_TWO} ${PYTHON_LIBRARIES} )
            set_target_properties( pyclassad PROPERTIES OUTPUT_NAME "${PYCLASSAD_LIB_NAME}" )

            # Note we do not use condor_shared_lib below because we want a separate install target.
            add_library( classad_module SHARED classad_module.cpp classad_parsers.cpp )
            target_link_libraries( classad_module pyclassad ${PCRE_FOUND} ${PYTHON_LIBRARY_BOOST_ONE} ${PYTHON_LIBRARY_BOOST_TWO} ${PYTHON_LIBRARIES} )
            set_target_properties(classad_module PROPERTIES PREFIX "" OUTPUT_NAME classad )
            set_target_properties(classad_module PROPERTIES SUFFIX ".pyd" )

            # This is to be used as a CLASSAD_USER_LIBS extension so we can invoke
            # python functions.
            add_library( classad_python_user SHARED classad_python_user.cpp )
            target_link_libraries( classad_python_user condor_utils pyclassad ${CONDOR_WIN_LIBS})
            set_target_properties(classad_python_user PROPERTIES SUFFIX ".pyd" )
            install ( TARGETS classad_python_user DESTINATION "${C_LIBEXEC}" )

            add_library( htcondor SHARED htcondor.cpp collector.cpp negotiator.cpp config.cpp daemon_and_ad_types.cpp dc_tool.cpp export_headers.h old_boost.h schedd.cpp secman.cpp module_lock.cpp export_compat_classad.cpp enable_deprecation_warnings.cpp )
            target_link_libraries( htcondor pyclassad condor_utils ${PYTHON_LIBRARY_BOOST_ONE} ${PYTHON_LIBRARY_BOOST_TWO} ${PYTHON_LIBRARIES} ${CONDOR_WIN_LIBS} )
            set_target_properties( htcondor PROPERTIES SUFFIX ".pyd" )

            install ( TARGETS pyclassad DESTINATION ${C_LIB} )
            install ( TARGETS htcondor classad_module DESTINATION ${C_PYTHONARCHLIB} )


            string( COMPARE EQUAL ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} INSOURCEBUILD)
            if (BUILD_TESTING)
              if(NOT INSOURCEBUILD)
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/event_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/event_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/event_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/event_tests.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/job.log DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/job.log COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/job.log ${CMAKE_CURRENT_BINARY_DIR}/tests/job.log )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/log_reader_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/log_reader_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/log_reader_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/log_reader_tests.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/job_queue.log DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/job_queue.log COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/job_queue.log ${CMAKE_CURRENT_BINARY_DIR}/tests/job_queue.log )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/classad_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/classad_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/classad_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/classad_tests.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_version_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_version_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_version_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_version_tests.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_tests.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update_tests.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test.old.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.old.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.old.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test.old.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.old.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.old.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.old.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.old.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_submit.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_submit.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_submit.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_submit.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/submit.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/submit.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_sleep.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_sleep.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_sleep.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_sleep.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_large.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_large.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_large.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_large.ad )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update.py ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update.py )
                add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test_log.txt DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_log.txt COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_log.txt ${CMAKE_CURRENT_BINARY_DIR}/tests/test_log.txt )
              endif()
              add_custom_target ( python_bindings_tests ALL DEPENDS tests/log_reader_tests.py tests/event_tests.py tests/job_queue.log tests/job.log tests/classad_tests.py tests/htcondor_version_tests.py tests/htcondor_tests.py tests/delayed_update_tests.py tests/test.ad tests/test_multiple.ad tests/test.old.ad tests/test_multiple.old.ad tests/submit.ad tests/submit_sleep.ad tests/submit_large.ad tests/delayed_submit.ad tests/delayed_update.py tests/test_log.txt ${CMAKE_CURRENT_BINARY_DIR}/test_driver )
              enable_testing()
            endif()
        endif()
    endif(NOT (MSVC_VERSION LESS 1700))    
  endif()
else()
  if ( WITH_PYTHON_BINDINGS AND PYTHONLIBS_FOUND AND Boost_PYTHON_LIBRARY AND NOT SOLARIS )
    configure_file (
  	"${PROJECT_SOURCE_DIR}/src/python-bindings/test_driver.in"
  	"${CMAKE_CURRENT_BINARY_DIR}/test_driver"
    )

    set ( CMAKE_LIBRARY_PATH_ORIG ${CMAKE_LIBRARY_PATH} )
    set ( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/lib64 )
    set ( CMAKE_LIBRARY_PATH CMAKE_LIBRARY_PATH_ORIG)

    if (PROPER AND "${PYTHON_VERSION_MAJOR}" MATCHES "3")
      set ( PYTHON_BOOST_LIB boost_python3 )
    else()
      set ( PYTHON_BOOST_LIB boost_python )
    endif()

    include_directories(${PYTHON_INCLUDE_DIRS} ${BOOST_INCLUDE})
    link_directories(${BOOST_LD})

    GET_FILENAME_COMPONENT(PYTHON_LIBRARY_FILENAME ${PYTHON_LIBRARIES} NAME)
    STRING(REGEX REPLACE ".*python([0-9]+[.]?[0-9]+).*" "\\1" _PYTHON_VERSION ${PYTHON_LIBRARY_FILENAME})
    if ( ${PACKAGE_VERSION} MATCHES "([0-9]+)[.]([0-9]+)[.]([0-9]+)" )
      set( PYCLASSAD_LIB_NAME "pyclassad${_PYTHON_VERSION}_${CMAKE_MATCH_1}_${CMAKE_MATCH_2}_${CMAKE_MATCH_3}" )
    else()
      message(FATAL_ERROR "Can't determine HTCondor version!")
    endif()
    condor_shared_lib( pyclassad classad.cpp classad_wrapper.h exprtree_wrapper.h )
    target_link_libraries( pyclassad ${CLASSADS_FOUND} ${PYTHON_LIBRARIES} -l${PYTHON_BOOST_LIB} )
    set_target_properties( pyclassad PROPERTIES OUTPUT_NAME "${PYCLASSAD_LIB_NAME}" )
    set_target_properties( pyclassad PROPERTIES COMPILE_FLAGS "-fPIC")

    # Note we do not use condor_shared_lib below because we want a separate install target.
    add_library( classad_module SHARED classad_module.cpp classad_parsers.cpp )
    target_link_libraries( classad_module pyclassad -l${PYTHON_BOOST_LIB} ${PYTHON_LIBRARIES} )
    set_target_properties(classad_module PROPERTIES PREFIX "" OUTPUT_NAME classad )
    set_target_properties(classad_module PROPERTIES SUFFIX ".so" )
    if ( NOT CMAKE_SKIP_RPATH )
      set_target_properties(classad_module PROPERTIES INSTALL_RPATH "${PYTHON_RPATH}")
    endif()

    # This is to be used as a CLASSAD_USER_LIBS extension so we can invoke
    # python functions.
    add_library( classad_python_user SHARED classad_python_user.cpp )
    target_link_libraries( classad_python_user condor_utils pyclassad )
    set_target_properties(classad_python_user PROPERTIES SUFFIX ".so" )
    if ( NOT CMAKE_SKIP_RPATH )
      set_target_properties(classad_python_user PROPERTIES INSTALL_RPATH "${CONDOR_RPATH}")
    endif()
    install ( TARGETS classad_python_user DESTINATION "${C_LIBEXEC}" )


    set_source_files_properties(config.cpp collector.cpp negotiator.cpp secman.cpp dc_tool.cpp schedd.cpp classad.cpp classad_module.cpp classad_parsers.cpp event.cpp log_reader.cpp PROPERTIES COMPILE_FLAGS "-Wno-strict-aliasing -Wno-cast-qual -Wno-deprecated -Wno-write-strings")
    add_library( htcondor SHARED htcondor.cpp collector.cpp negotiator.cpp config.cpp daemon_and_ad_types.cpp dc_tool.cpp export_headers.h old_boost.h schedd.cpp secman.cpp event.cpp module_lock.cpp log_reader.cpp export_compat_classad.cpp enable_deprecation_warnings.cpp )
    target_link_libraries( htcondor pyclassad condor_utils -l${PYTHON_BOOST_LIB} ${PYTHON_LIBRARIES} )
    set_target_properties( htcondor PROPERTIES PREFIX "" )
    set_target_properties( htcondor PROPERTIES SUFFIX ".so" )
    if ( NOT CMAKE_SKIP_RPATH )
      set_target_properties(htcondor PROPERTIES INSTALL_RPATH "${PYTHON_RPATH}")
    endif()

    install ( TARGETS pyclassad DESTINATION ${C_LIB} )
    install ( TARGETS htcondor classad_module DESTINATION ${C_PYTHONARCHLIB} )

    if ( DARWIN )
          add_custom_command( TARGET classad_module htcondor
                  POST_BUILD
                  COMMAND ${CMAKE_SOURCE_DIR}/src/condor_scripts/macosx_rewrite_libs classad.so htcondor.so )
          add_custom_command( TARGET pyclassad
                  POST_BUILD
                  COMMAND ${CMAKE_SOURCE_DIR}/src/condor_scripts/macosx_rewrite_libs lib${PYCLASSAD_LIB_NAME}.dylib )
    endif()

    string( COMPARE EQUAL ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} INSOURCEBUILD)
    if (BUILD_TESTING)
      if(NOT INSOURCEBUILD)
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/event_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/event_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/event_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/event_tests.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/job.log DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/job.log COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/job.log ${CMAKE_CURRENT_BINARY_DIR}/tests/job.log )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/log_reader_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/log_reader_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/log_reader_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/log_reader_tests.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/job_queue.log DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/job_queue.log COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/job_queue.log ${CMAKE_CURRENT_BINARY_DIR}/tests/job_queue.log )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/classad_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/classad_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/classad_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/classad_tests.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_version_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_version_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_version_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_version_tests.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/htcondor_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/htcondor_tests.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update_tests.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update_tests.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update_tests.py ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update_tests.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test.old.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.old.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.old.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test.old.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.old.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.old.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.old.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.old.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiple.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/test_multiple.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_submit.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_submit.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_submit.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_submit.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/submit.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/submit.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_sleep.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_sleep.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_sleep.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_sleep.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_large.ad DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_large.ad COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/submit_large.ad ${CMAKE_CURRENT_BINARY_DIR}/tests/submit_large.ad )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update.py DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update.py COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/delayed_update.py ${CMAKE_CURRENT_BINARY_DIR}/tests/delayed_update.py )
        add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tests/test_log.txt DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_log.txt COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_log.txt ${CMAKE_CURRENT_BINARY_DIR}/tests/test_log.txt )
      endif()
      add_custom_target ( python_bindings_tests ALL DEPENDS tests/log_reader_tests.py tests/event_tests.py tests/job_queue.log tests/job.log tests/classad_tests.py tests/htcondor_version_tests.py tests/htcondor_tests.py tests/delayed_update_tests.py tests/test.ad tests/test_multiple.ad tests/test.old.ad tests/test_multiple.old.ad tests/submit.ad tests/submit_sleep.ad tests/submit_large.ad tests/delayed_submit.ad tests/delayed_update.py tests/test_log.txt ${CMAKE_CURRENT_BINARY_DIR}/test_driver )
      enable_testing()
      if ( LINUX )
        if ( ${SYSTEM_NAME} MATCHES "rhel5" )
            message(STATUS "classad_test python bindings tests removed for RedHat 5 OS")
        else()
            add_test(python_classad_test test_driver tests/classad_tests.py -v)
            add_test(python_htcondor_test test_driver tests/htcondor_version_tests.py -v)
        endif()
      endif ( LINUX )
    endif (BUILD_TESTING)
  endif ( WITH_PYTHON_BINDINGS AND PYTHONLIBS_FOUND AND Boost_PYTHON_LIBRARY AND NOT SOLARIS )
endif(WINDOWS)
