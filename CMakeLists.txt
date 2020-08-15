cmake_minimum_required(VERSION 3.0)
project(torch_server)

find_package(OpenCV REQUIRED)
find_package(Torch REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(server ./code/main.cpp)
target_link_libraries(server ${OpenCV_LIBS} ${TORCH_LIBRARIES})

set_property(TARGET server PROPERTY CXX_STANDARD 14)
