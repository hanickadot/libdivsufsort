### cmake file for building libdivsufsort Package ###
cmake_minimum_required(VERSION 3.14)

## Project information ##
project(libdivsufsort VERSION 2.0.1)
set(PROJECT_VENDOR "Yuta Mori")
set(PROJECT_CONTACT "yuta.256@gmail.com")
set(PROJECT_URL "https://github.com/y-256/libdivsufsort")
set(PROJECT_DESCRIPTION "A lightweight suffix sorting library")

## Add subdirectories ##
add_subdirectory(include)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)

