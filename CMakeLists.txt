# CMake version.
cmake_minimum_required(VERSION 3.22)

# Project.
project(
  Microcosm
  VERSION 0.0.1
  DESCRIPTION "My personal C++ utility library"
  LANGUAGES C CXX
  )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(cmake/CompilerFlags.cmake)
include(cmake/Libraries.cmake)
include(cmake/Testing.cmake)
add_subdirectory(src)

# Install.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY ExactVersion
  )
set(INCLUDE_DIR include)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  PATH_VARS INCLUDE_DIR
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
  )
install(
  FILES
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION
    "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake"
  )
