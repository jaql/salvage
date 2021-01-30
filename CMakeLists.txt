cmake_minimum_required(VERSION 3.17)
project(jetsam C)

set(CMAKE_C_STANDARD 11)

find_package(CURL REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(NOT CMAKE_USE_PTHREADS_INIT)
    error(FATAL_MESSAGE "pthreads required")
endif()

add_executable(flotsam flotsam.c heap.h heap.c heap.h http.h http.c init.c init.h log.h opts.c wait.c wait.h)
add_executable(jetsam jetsam.c exec.h exec.c heap.h heap.c heap.h http.h http.c init.c init.h log.h opts.c)

target_link_libraries(flotsam PRIVATE ${CURL_LIBRARIES} Threads::Threads)
target_link_libraries(jetsam PRIVATE ${CURL_LIBRARIES} Threads::Threads)
