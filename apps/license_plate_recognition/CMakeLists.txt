add_executable(lpr lpr.cpp)
target_compile_features(lpr PUBLIC cxx_std_17)
target_link_libraries(lpr PUBLIC deep_stream)