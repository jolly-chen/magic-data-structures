set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_LINKER "/usr/bin/g++")
set(CMAKE_CXX_STANDARD 26)

set(MANUAL_SOURCES
    aos2soa
    aos2soa_contiguous
    # aosoa2soaos_contiguous
)

foreach(src ${MANUAL_SOURCES})
    add_executable(${src}_norefl ${src}.cpp)
endforeach()
