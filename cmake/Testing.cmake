if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
  string(RANDOM LENGTH 8 ALPHABET "012345679" SEED)
  macro(microcosm_add_tests BIN_NAME)
    set(options)
    set(oneValArgs)
    set(multiValArgs SOURCES DEPENDS)
    cmake_parse_arguments(args "${options}" "${oneValArgs}" "${multiValArgs}" ${ARGN})
    add_executable(${BIN_NAME} "${CMAKE_SOURCE_DIR}/testing/doctest.cc")
    set_target_properties(
      ${BIN_NAME}
      PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
      )
    target_sources(${BIN_NAME} PUBLIC "${args_SOURCES}")
    target_include_directories(${BIN_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/testing")
    target_link_libraries(${BIN_NAME} PUBLIC ${PROJECT_NAME}::headers)
    if(args_DEPENDS)
      foreach(DEPEND ${args_DEPENDS})
        target_link_libraries(${BIN_NAME} PUBLIC ${DEPEND})
      endforeach(DEPEND)
    endif()
    add_test(
      NAME ${BIN_NAME}
      COMMAND $<TARGET_FILE:${BIN_NAME}> -rs=${SEED}
      )
  endmacro()
else()
  macro(microcosm_add_tests BIN_NAME)
    # Do nothing.
  endmacro()
endif()
