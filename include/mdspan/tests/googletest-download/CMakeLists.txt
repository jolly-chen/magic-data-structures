# from https://github.com/google/googletest/blob/master/googletest/README.md
cmake_minimum_required(VERSION 3.5)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           main
  SOURCE_DIR        "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-src"
  BINARY_DIR        "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
