cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 COMPONENTS BuildInternals)

include(${RunCMake_TEST}.cmake)
