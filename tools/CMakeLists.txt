add_executable(savekit-dump dump/main.cpp)
target_link_libraries(savekit-dump PRIVATE savekit::savekit)
