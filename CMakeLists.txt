## template for a CMake C++ Library
cmake_minimum_required(VERSION 3.14)
project(libpressio_jit VERSION "0.0.2" LANGUAGES CXX)

#correct was to set a default build type
# https://blog.kitware.com/cmake-and-the-default-build-type/
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type was set. Setting build type to ${default_build_type}.")
  set(CMAKE_BUILD_TYPE ${default_build_type} CACHE 
    STRING "Choose the type to build" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

include(CTest)
include(GNUInstallDirs)
include(CheckCXXCompilerFlag)

#compiler flags and standard conformance checks
check_cxx_compiler_flag("-fno-omit-frame-pointer" HAVE_NO_OMIT_FRAME_POINTER)
set(NO_OMIT_FRAME_POINTER_FLAG "")
if(HAVE_NO_OMIT_FRAME_POINTER)
  set(NO_OMIT_FRAME_POINTER_FLAG "-fno-omit-frame-pointer")
endif()

if(NOT USE_SANITIZER)
  set(USE_SANITIZER "" CACHE STRING "use a build sanitizer" FORCE)
endif()
set_property(CACHE USE_SANITIZER PROPERTY STRINGS "" "Address" "Thread" "Undefined" "Leak" "Memory")
if (USE_SANITIZER STREQUAL "Address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
elseif (USE_SANITIZER STREQUAL "Thread")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
elseif (USE_SANITIZER STREQUAL "Undefined")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
elseif (USE_SANITIZER STREQUAL "Leak")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
elseif (USE_SANITIZER STREQUAL "Memory")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
endif()


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "build libpressio_jit as a shared library" ON)

find_package(LibPressio REQUIRED)
find_package(std_compat REQUIRED)

add_library(libpressio_jit
  ./src/libpressio_jit.cc
  ./src/plugins/generators/template.cc
  ./src/plugins/generators/generator_base.cc
  ./include/libpressio_jit.h
  ./include/libpressio_jit_ext/cpp/generator.h
  )
target_link_libraries(libpressio_jit PUBLIC LibPressio::libpressio)
target_compile_features(libpressio_jit PUBLIC cxx_std_17)
target_include_directories(libpressio_jit 
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libpressio_jit>
  )
target_compile_options(libpressio_jit PRIVATE 
  $<$<CONFIG:Debug>: -Wall  -Wextra -Wpedantic>
  $<$<CONFIG:RelWithDebInfo>: ${NO_OMIT_FRAME_POINTER_FLAG}>
  )

option(LIBPRESSIO_JIT_HAS_POORJIT "build support for the poorjit jit compressor" OFF)
if(LIBPRESSIO_JIT_HAS_POORJIT)
    find_package(poorjit REQUIRED)
    find_package(Boost REQUIRED)
    target_sources(libpressio_jit PRIVATE src/plugins/compressors/poorjit.cc)
    target_link_libraries(libpressio_jit PRIVATE poorjit::poorjit Boost::headers)
endif()

option(LIBPRESSIO_JIT_WERROR ON)
if(LIBPRESSIO_JIT_WERROR)
  target_compile_options(libpressio_jit PRIVATE 
    $<$<CONFIG:Debug>: -Werror>
    )
endif()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/libpressio_jit_version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/libpressio_jit_version.h
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/libpressio_jit.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/libpressio_jit.pc
  @ONLY
  )

export(TARGETS libpressio_jit NAMESPACE libpressio_jit:: FILE libpressio_jit.cmake)
install(TARGETS libpressio_jit EXPORT libpressio_jitTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/libpressio_jitConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/libpressio_jitConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpressio_jit
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/libpressio_jitConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libpressio_jitConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libpressio_jitConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpressio_jit
)
install(EXPORT libpressio_jitTargets NAMESPACE libpressio_jit:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpressio_jit)
install(DIRECTORY include/ DESTINATION
  ${CMAKE_INSTALL_INCLUDEDIR}/libpressio_jit)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/libpressio_jit_version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libpressio_jit)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpressio_jit.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pkgconfig)


if(BUILD_TESTING)
  add_subdirectory(test)
endif()

option(USE_CLANG_TIDY "include clang-tidy warnings in the build log" OFF)
if(USE_CLANG_TIDY)
  find_program(CLANG_TIDY clang-tidy)
  set_target_properties(libpressio_jit PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY}")
  set_target_properties(libpressio_jit PROPERTIES C_CLANG_TIDY "${CLANG_TIDY}")
endif()

option(USE_INCLUDE_WHAT_YOU_USE "include include-what-you-use warnings in the build log" OFF)
if(USE_INCLUDE_WHAT_YOU_USE)
  find_program(INCLUDE_WHAT_YOU_USE include-what-you-use)
  set_target_properties(libpressio_jit PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "${INCLUDE_WHAT_YOU_USE}")
  set_target_properties(libpressio_jit PROPERTIES C_INCLUDE_WHAT_YOU_USE "${INCLUDE_WHAT_YOU_USE}")
endif()


option(BUILD_DOCS "build the documentation" OFF)
if(BUILD_DOCS)
  find_package(Doxygen REQUIRED dot)
  set(DOXYGEN_MAN_LINKS YES)
  set(DOXYGEN_GENERATE_MAN YES)
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_EXTRACT_LOCAL_METHODS YES)
  set(DOXYGEN_EXTRACT_STATIC YES)
  set(DOXYGEN_MACRO_EXPANSION YES)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
  doxygen_add_docs(
    docs
    ${PROJECT_SOURCE_DIR}/README.md
    ${PROJECT_SOURCE_DIR}/include
    COMMENT "Generate Documenation"
  )
endif()
# vim: ft=cmake :
