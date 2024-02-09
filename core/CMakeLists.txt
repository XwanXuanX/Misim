cmake_minimum_required(VERSION 3.20)

# C++23 standards
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define a project
project(Misim)

# Repo structure
include_directories(inc)
add_executable(misim src/main.cpp)
