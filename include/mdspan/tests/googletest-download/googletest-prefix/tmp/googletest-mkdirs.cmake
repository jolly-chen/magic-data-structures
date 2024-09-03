# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-src")
  file(MAKE_DIRECTORY "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-src")
endif()
file(MAKE_DIRECTORY
  "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-build"
  "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix"
  "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix/tmp"
  "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix/src/googletest-stamp"
  "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix/src"
  "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix/src/googletest-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix/src/googletest-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jollychen/phd/magic-data-structures/include/mdspan/tests/googletest-download/googletest-prefix/src/googletest-stamp${cfgdir}") # cfgdir has leading slash
endif()
