cmake_minimum_required(VERSION 3.0)
project(torch_server)

set(cmake_install_prefix "../")

# SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/data/cl/libtorch")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(MySQL REQUIRED)

find_package(OpenCV REQUIRED)
find_package(Torch REQUIRED)


include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${MYSQL_INCLUDE_DIR})

# Add header file include directories
include_directories(./code/buffer ./code/http ./code/log ./code/model ./code/pool ./code/server ./code/timer)

# Add block directories
set(BUFFER_SRC ./code/buffer/buffer.cpp)
set(HTTP_SRC ./code/http/http_conn.cpp ./code/http/http_request.cpp ./code/http/http_response.cpp)
set(LOG_SRC ./code/log/log.cpp)
set(MODEL_SRC ./code/model/torch_model.cpp)
set(POOL_SRC ./code/pool/sql_conn_pool.cpp)
set(SERVER_SRC ./code/server/server.cpp ./code/server/epoller.cpp)
set(TIMER_SRC ./code/timer/timer_heap.cpp)

add_executable(server ./code/main.cpp ${BUFFER_SRC} ${HTTP_SRC} ${LOG_SRC} ${MODEL_SRC} ${POOL_SRC} ${SERVER_SRC} ${TIMER_SRC})
target_link_libraries(server ${OpenCV_LIBS} ${TORCH_LIBRARIES} ${MYSQL_LIBRARIES})

set_property(TARGET server PROPERTY CXX_STANDARD 14)
