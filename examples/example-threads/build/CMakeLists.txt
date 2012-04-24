cmake_minimum_required(VERSION 2.6)

project(example-threads)

include(../../build/ExamplesCommon.cmake)

set(EXAMPLE_THREADS_INCLUDE
  )
  
set(EXAMPLE_THREADS_SRC
  ${SRC_DIR}/main.c
  )

source_group("include" FILES ${EXAMPLE_THREADS_INCLUDE})
source_group("src" FILES ${EXAMPLE_THREADS_SRC})

add_executable(example-threads
  ${EXAMPLE_THREADS_INCLUDE}
  ${EXAMPLE_THREADS_SRC}
  )

target_link_libraries(example-threads
  gorilla
  )
