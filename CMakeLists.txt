cmake_minimum_required(VERSION 3.20...3.29)
# 3.20 is the minimum requirement for LLVM
# https://www.llvm.org/docs/CMake.html#quick-start

project(
	Wasserstein
	VERSION 1.1.0
	LANGUAGES CXX)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
	target_link_libraries(wasserstein PUBLIC OpenMP::OpenMP_CXX)
endif()

find_package(
	Python
	COMPONENTS Interpreter Development.Module
	REQUIRED)

find_package(
	SWIG
	COMPONENTS python
	REQUIRED)

add_library(wasserstein wasserstein.cpp Wasserstein.hh)

include(UseSWIG)

swig_add_library(
	wasserstein
	LANGUAGE python
	SOURCES wasserstein/swig/wasserstein.i wasserstein/wasserstein.cpp)
if(WIN32)
	set_property(TARGET wasserstein 
		PROPERTY SUFFIX ".${Python_SOABI}.pyd")
else()
	set_property(TARGET wasserstein
		PROPERTY SUFFIX ".${Python_SOABI}${CMAKE_SHARED_MODULE_SUFFIX}")
endif()
target_link_libraries(wasserstein PRIVATE Python::Module)
