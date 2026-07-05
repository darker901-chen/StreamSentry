cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE
    src/plugin-main.c
    src/filter.c
    src/filter.h
    src/coord-map.c
    src/coord-map.h
    src/plate-gen.c
    src/plate-gen.h
    src/shared-state.c
    src/shared-state.h
    src/geom-resolve.c
    src/geom-resolve.h
    src/watcher.cpp
    src/watcher.h
)

# Windows detection stack: DWM (cloaked check), COM/UIA (password fields),
# Win32 window enumeration + process query. libobs already pulls user32.
if(WIN32)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE dwmapi ole32 oleaut32 uuid user32)
endif()

# Optional performance instrumentation (watcher tick cost + render decision
# cost logged periodically). OFF for release builds; turned ON only to
# capture the M3 perf/soak numbers.
option(STREAMSENTRY_PERF_LOG "Log watcher/render performance samples" OFF)
if(STREAMSENTRY_PERF_LOG)
  target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE STREAMSENTRY_PERF_LOG)
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Unit tests for the pure modules (no libobs dependency); ctest runs
# them locally during verification.
option(STREAMSENTRY_BUILD_TESTS "Build StreamSentry unit tests" ON)
if(STREAMSENTRY_BUILD_TESTS)
  enable_testing()
  add_executable(coord-map-tests tests/coord-map-tests.c src/coord-map.c)
  add_executable(plate-gen-tests tests/plate-gen-tests.c src/plate-gen.c)
  add_test(NAME coord-map-tests COMMAND coord-map-tests)
  add_test(NAME plate-gen-tests COMMAND plate-gen-tests)

  # Watcher self-test: exercises the real COM MTA watcher thread + Win32
  # enumeration + blocklist matching, stubbing the two libobs symbols the
  # watcher references (obs_log, os_gettime_ns) so no obs.dll is needed.
  # NOT registered with add_test(): it launches notepad and fires a toast,
  # so it is run explicitly for M2 evidence, not in the pure ctest sweep.
  if(WIN32)
    add_executable(watcher-selftest tests/watcher-selftest.cpp src/watcher.cpp src/shared-state.c)
    target_include_directories(
      watcher-selftest
      PRIVATE src "$<TARGET_PROPERTY:OBS::libobs,INTERFACE_INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(watcher-selftest PRIVATE OBS::w32-pthreads dwmapi ole32 oleaut32 uuid user32)
    add_custom_command(
      TARGET watcher-selftest
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:OBS::w32-pthreads>"
              "$<TARGET_FILE_DIR:watcher-selftest>"
    )
  endif()
endif()
