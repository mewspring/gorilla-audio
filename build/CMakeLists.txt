cmake_minimum_required(VERSION 2.6)

project(Gorilla)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_COMPILER_IS_GNUCC)
  set(LINUX "1")
endif()

if(MSVC)
  SET(PLATFORM_NAME "win32")
endif()
if(APPLE)
  SET(PLATFORM_NAME "osx")
endif()
if(LINUX)
  SET(PLATFORM_NAME "linux")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/../bin/${PLATFORM_NAME}/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/../bin/${PLATFORM_NAME}/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/../bin/${PLATFORM_NAME}/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR}/../bin/${PLATFORM_NAME}/Release)

set(CMAKE_DEBUG_POSTFIX D)

if(APPLE)
  add_definitions(-mmacosx-version-min=10.5)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mmacosx-version-min=10.5")
  set(CMAKE_SHARED_LIBRARY_C_FLAGS "${CMAKE_SHARED_LIBRARY_C_FLAGS} -mmacosx-version-min=10.5")
  set(CMAKE_SHARED_LIBRARY_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CXX_FLAGS} -mmacosx-version-min=10.5")
endif()

if(MSVC)
  set_target_properties(PROPERTIES PREFIX "../") # The Prefix Hack!
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /Gm")
endif()

include_directories(../../include)

set(GORILLA_INCLUDE
  )
  
set(GORILLA_SRC
  )

# OGG/Vorbis
set(LIBOGGDIR ../ext/libogg)
set(LIBVORBISDIR ../ext/libvorbis)

include_directories(${LIBOGGDIR}/include)
include_directories(${LIBVORBISDIR}/include)
include_directories(${LIBOGGDIR}/src)
include_directories(${LIBVORBISDIR}/lib)

set(OGGVORBIS_INCLUDE
  ${LIBOGGDIR}/include/ogg/ogg.h
  ${LIBOGGDIR}/include/ogg/os_types.h
  ${LIBVORBISDIR}/include/vorbis/codec.h
  ${LIBVORBISDIR}/include/vorbis/vorbisenc.h
  ${LIBVORBISDIR}/include/vorbis/vorbisfile.h
  )
  
set(OGGVORBIS_SRC
  ${LIBOGGDIR}/src/bitwise.c
  ${LIBOGGDIR}/src/framing.c
  ${LIBVORBISDIR}/lib/analysis.c
  ${LIBVORBISDIR}/lib/bitrate.c
  ${LIBVORBISDIR}/lib/block.c
  ${LIBVORBISDIR}/lib/codebook.c
  ${LIBVORBISDIR}/lib/envelope.c
  ${LIBVORBISDIR}/lib/floor0.c
  ${LIBVORBISDIR}/lib/floor1.c
  ${LIBVORBISDIR}/lib/info.c
  ${LIBVORBISDIR}/lib/lpc.c
  ${LIBVORBISDIR}/lib/lsp.c
  ${LIBVORBISDIR}/lib/mapping0.c
  ${LIBVORBISDIR}/lib/mdct.c
  ${LIBVORBISDIR}/lib/psy.c
  ${LIBVORBISDIR}/lib/registry.c
  ${LIBVORBISDIR}/lib/res0.c
  ${LIBVORBISDIR}/lib/sharedbook.c
  ${LIBVORBISDIR}/lib/smallft.c
  ${LIBVORBISDIR}/lib/synthesis.c
  ${LIBVORBISDIR}/lib/vorbisfile.c
  ${LIBVORBISDIR}/lib/window.c
  )

source_group("include" FILES ${GORILLA_INCLUDE})
source_group("src" FILES ${GORILLA_SRC})
source_group("include\\oggvorbis" FILES ${OGGVORBIS_INCLUDE})
source_group("src\\oggvorbis" FILES ${OGGVORBIS_SRC})

add_library(gorilla STATIC
  ${GORILLA_INCLUDE}
  ${GORILLA_SRC}
  ${OGGVORBIS_INCLUDE}
  ${OGGVORBIS_SRC}
  )
