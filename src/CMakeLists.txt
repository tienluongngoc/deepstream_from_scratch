set(LIBRARY_NAME deep_stream)
set(DEEP_STREAM_LIBS /opt/nvidia/deepstream/deepstream-5.1/lib)

find_package(PkgConfig) 
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
list(APPEND INCLUDE_DIRS ${GLIB_INCLUDE_DIRS}${GSTREAMER_INCLUDE_DIRS})
list(APPEND DEPS ${GLIB_LIBRARY_DIRS}${GSTREAMER_LIBRARY_DIRS})
list(APPEND DEPS "${DEEP_STREAM_LIBS}/libnvdsgst_meta.so")
list(APPEND DEPS "${DEEP_STREAM_LIBS}/libnvds_meta.so")
list(APPEND DEPS "${DEEP_STREAM_LIBS}/libnvbufsurface.so")
list(APPEND DEPS "${DEEP_STREAM_LIBS}/libnvbufsurftransform.so")

list(APPEND INCLUDE_DIRS "${DEEP_STREAM_LIBS}/../sources/includes/")


file(GLOB_RECURSE HEADER_LIST "${PROJECT_SOURCE_DIR}/include/*[.hpp, .h]")
file(GLOB_RECURSE SOURCE_LIST "${PROJECT_SOURCE_DIR}/src/*.cpp")

list(APPEND INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include/")

find_package(CUDA REQUIRED)
message(STATUS "Find CUDA include at ${CUDA_INCLUDE_DIRS}")
message(STATUS "Find CUDA libraries: ${CUDA_LIBRARIES}")

add_library(${LIBRARY_NAME} ${SOURCE_LIST} ${HEADER_LIST})
target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
target_include_directories(${LIBRARY_NAME} PUBLIC ${INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS})
target_link_libraries(${LIBRARY_NAME} PUBLIC ${GSTREAMER_LIBRARIES} ${DEPS} ${CUDA_LIBRARIES})
