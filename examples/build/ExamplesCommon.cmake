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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/../bin)

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
set(DEBUG_CONFIGURATIONS "Debug")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUNICODE -D_UNICODE")
if(MSVC)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Gm")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /Gm")
endif()

set(INCLUDE_DIR ../include)
set(SRC_DIR ../src)
set(GORILLA_INCLUDE_DIR ../../../include)

include_directories(${INCLUDE_DIR})
include_directories(${GORILLA_INCLUDE_DIR})
add_subdirectory(../../../build ${CMAKE_BINARY_DIR}/gorilla)
