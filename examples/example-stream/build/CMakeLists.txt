cmake_minimum_required(VERSION 2.6)

project(example-stream)

include(../../build/ExamplesCommon.cmake)

set(EXAMPLE_STREAM_INCLUDE
  )
  
set(EXAMPLE_STREAM_SRC
  ${SRC_DIR}/main.c
  )

source_group("include" FILES ${EXAMPLE_STREAM_INCLUDE})
source_group("src" FILES ${EXAMPLE_STREAM_SRC})

add_executable(example-stream
  ${EXAMPLE_STREAM_INCLUDE}
  ${EXAMPLE_STREAM_SRC}
  )

target_link_libraries(example-stream
  gorilla
  )
