cmake_minimum_required(VERSION 2.6)

project(example-tone)

include(../../build/ExamplesCommon.cmake)

set(EXAMPLE_TONE_INCLUDE
  )
  
set(EXAMPLE_TONE_SRC
  ${SRC_DIR}/main.c
  )

source_group("include" FILES ${EXAMPLE_TONE_INCLUDE})
source_group("src" FILES ${EXAMPLE_TONE_SRC})

add_executable(example-tone
  ${EXAMPLE_TONE_INCLUDE}
  ${EXAMPLE_TONE_SRC}
  )

target_link_libraries(example-tone
  gorilla
  )
