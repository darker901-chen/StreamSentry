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
)

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
endif()
