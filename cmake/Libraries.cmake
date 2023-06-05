set(LIBS "" CACHE INTERNAL "List of libraries.")

option(BUILD_NATIVE "Build for native hardware?" OFF)

# Include GNUInstallDirs and print them out.
include(GNUInstallDirs)
message(STATUS "INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "INSTALL_INCLUDEDIR: ${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "INSTALL_LIBDIR: ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "INSTALL_BINDIR: ${CMAKE_INSTALL_BINDIR}")

# Tell CMake to install the include directory, containing all of the header files for the project.
install(
  DIRECTORY "${PROJECT_SOURCE_DIR}/include/"
  DESTINATION include
  )

# Install the targets. Note that, at this point in the CMake execution, no targets have actually been
# defined yet. The microcosm_add_library() macro below takes care of setting up the install info for
# each of the sub-libraries, and by the time CMake has descended into all of the src directories, the
# targets will all be populated.
install(
  EXPORT ${PROJECT_NAME}_Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
  )

# Find fmt. This is the only dependency the entire project requires. Supposedly the formatting
# functionality will one day be part of the standard library, but not yet.
find_package(fmt REQUIRED)

# Set up the headers INTERFACE link target. This ropes in the Microcosm include directory and the fmt dependency.
add_library(headers INTERFACE)
add_library(${PROJECT_NAME}::headers ALIAS headers)
target_include_directories(
  headers
  INTERFACE
    $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${${PROJECT_NAME}_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
target_link_libraries(headers INTERFACE fmt::fmt-header-only)
install(
  TARGETS headers
  EXPORT ${PROJECT_NAME}_Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT headers
  )
list(APPEND LIBS "headers")
set(LIBS "${LIBS}" CACHE INTERNAL "")

include(GenerateExportHeader)
macro(microcosm_add_library LIB_NAME)
  set(options STATIC SHARED)
  set(oneValArgs EXPORT_FILENAME EXPORT_MACRO)
  set(multiValArgs SOURCES DEPENDS)
  cmake_parse_arguments(args "${options}" "${oneValArgs}" "${multiValArgs}" ${ARGN})

  if(args_SHARED)
    add_library(${LIB_NAME} SHARED ${args_SOURCES})
    set_target_properties(
      ${LIB_NAME}
      PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON
      )
  else()
    add_library(${LIB_NAME} STATIC ${args_SOURCES})
  endif()

  # Everything gets an alias under the project name.
  add_library(${PROJECT_NAME}::${LIB_NAME} ALIAS ${LIB_NAME})

  # Everything is C++20 and outputs to the lib directory.
  set_target_properties(
    ${LIB_NAME}
    PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED YES
      OUTPUT_NAME ${PROJECT_NAME}_${LIB_NAME}
      VERSION ${PROJECT_VERSION}
      SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )

  # If necessary, build and tune for the native hardware.
  if(BUILD_NATIVE AND UNIX) 
    target_compile_options(
      ${LIB_NAME}
      PRIVATE 
        "$<$<CONFIG:RELEASE>:-O3;-DNDEBUG;-march=native;-mtune=native>"
      )
  endif()

  # Everything links to the headers by default.
  target_link_libraries(${LIB_NAME} PUBLIC ${PROJECT_NAME}::headers)

  # Link to given dependencies. 
  if(args_DEPENDS)
    foreach(DEPEND ${args_DEPENDS})
      target_link_libraries(${LIB_NAME} PUBLIC ${DEPEND})
    endforeach(DEPEND)
  endif()

  # Generate the export header.
  set(EXPORT_MACRO "${args_EXPORT_MACRO}")
  set(EXPORT_FILENAME "${args_EXPORT_FILENAME}")
  get_filename_component(EXPORT_DIRNAME ${EXPORT_FILENAME} DIRECTORY)
  generate_export_header(
    ${LIB_NAME}
    EXPORT_MACRO_NAME ${EXPORT_MACRO}
    EXPORT_FILE_NAME "${PROJECT_BINARY_DIR}/include/${EXPORT_FILENAME}"
    )
  install(
    FILES "${PROJECT_BINARY_DIR}/include/${EXPORT_FILENAME}"
    DESTINATION "include/${EXPORT_DIRNAME}"
    )

  # Install the library target.
  install(
    TARGETS ${LIB_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${LIB_NAME}
    )

  # Add to the list of libraries.
  list(APPEND LIBS "${LIB_NAME}")
  set(LIBS "${LIBS}" CACHE INTERNAL "")
endmacro()
