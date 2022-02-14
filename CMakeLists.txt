cmake_minimum_required(VERSION 3.12...3.23)

project(
  snitch
  LANGUAGES C
  VERSION 0.1.0
  DESCRIPTION
    "Language agnostic tool that collects TODOs in the source code and reports them as Issues"
  HOMEPAGE_URL "https://github.com/Phytolizer/snitch"
)

include(FetchContent)
FetchContent_Declare(
  pcre
  GIT_REPOSITORY "https://github.com/PhilipHazel/pcre2.git"
  GIT_TAG 111cd470b55d0c06bfb157c8f4521907d3d95fdb
)
FetchContent_MakeAvailable(pcre)

add_library(minIni minIni/minIni.c)

add_executable(snitch main.c walk.c string_span.c)
target_compile_features(snitch PRIVATE c_std_11)
target_link_libraries(snitch PRIVATE pcre2-8 minIni)
