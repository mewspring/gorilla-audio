cmake_minimum_required(VERSION 2.6)

project(example-sound)

include(../../build/ExamplesCommon.cmake)

set(EXAMPLE_SOUND_INCLUDE
  )
  
set(EXAMPLE_SOUND_SRC
  ${SRC_DIR}/main.c
  )

source_group("include" FILES ${EXAMPLE_SOUND_INCLUDE})
source_group("src" FILES ${EXAMPLE_SOUND_SRC})

add_executable(example-sound
  ${EXAMPLE_SOUND_INCLUDE}
  ${EXAMPLE_SOUND_SRC}
  )

target_link_libraries(example-sound
  gorilla
  )
