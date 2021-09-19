cmake_minimum_required(VERSION 3.10.2)
project(deepstream_from_scratch VERSION 0.1.0)

add_subdirectory(src)
add_subdirectory(apps)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
