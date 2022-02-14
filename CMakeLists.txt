cmake_minimum_required(VERSION 3.12...3.23)

project(
  snitch
  LANGUAGES C
  VERSION 0.1.0
  DESCRIPTION
    "Language agnostic tool that collects TODOs in the source code and reports them as Issues"
  HOMEPAGE_URL "https://github.com/Phytolizer/snitch"
)

add_executable(snitch main.c)
