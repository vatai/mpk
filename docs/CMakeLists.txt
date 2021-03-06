### Doxygen ###
# From: https://vicrucann.github.io/tutorials/quick-cmake-doxygen/
# first we can indicate the documentation build as an option and set it to ON by default
option(BUILD_DOC "Build documentation" ON)

message("Output: ${CMAKE_SOURCE_DIR}/pdmpk")
message("Output: ${CMAKE_PROJECT_DIR}/pdmpk")
# check if Doxygen is installed
find_package(Doxygen)
if (DOXYGEN_FOUND AND BUILD_DOC)
  set(CMAKE_DOXYGEN_WARN_IF_UNDOCUMENTED "YES")
  # set input and output files
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/../docs/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # request to configure the file
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  message("Doxygen build started")

  # note the option ALL which allows to build the docs together with the application
  add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM)
elseif (BUILD_DOC)
  message("Doxygen need to be installed to generate the doxygen documentation")
else()
    set(CMAKE_DOXYGEN_WARN_IF_UNDOCUMENTED "NO")
endif (DOXYGEN_FOUND AND BUILD_DOC)
