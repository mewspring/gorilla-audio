cmake_minimum_required(VERSION 2.6)

project(example-player)

include(../../build/ExamplesCommon.cmake)

set(EXAMPLE_PLAYER_INCLUDE
  )
  
set(EXAMPLE_PLAYER_SRC
  ${SRC_DIR}/main.c
  )

source_group("include" FILES ${EXAMPLE_PLAYER_INCLUDE})
source_group("src" FILES ${EXAMPLE_PLAYER_SRC})

add_executable(example-player
  ${EXAMPLE_PLAYER_INCLUDE}
  ${EXAMPLE_PLAYER_SRC}
  )

target_link_libraries(example-player
  gorilla
  )
