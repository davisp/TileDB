## cf https://stackoverflow.com/questions/1815688/how-to-use-ccache-with-cmake
## leading to https://invent.kde.org/kde/konsole/merge_requests/26/diffs

find_program(CCACHE_FOUND NAMES "sccache" "ccache")
if (CCACHE_FOUND)
  message(STATUS "Found ccache: ${CCACHE_FOUND}")

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" # GNU is GNU GCC
      OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # without this compiler messages in `make` backend would be uncolored
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fdiagnostics-color=auto")
  endif()

  set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})

  if (WIN32)
    file(COPY_FILE ${CCACHE_FOUND} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)

    set(CMAKE_VS_GLOBALS
      "CLToolExe=cl.exe"
      "CLToolPath=${CMAKE_BINARY_DIR}"
      "TrackFileAccess=false"
      "UseMultiToolTask=true"
      "DebugInformationFormat=OldStyle"
    )
  endif()
else()
  message(FATAL_ERROR "Unable to find ccache")
endif()
