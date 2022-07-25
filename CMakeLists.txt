cmake_minimum_required(VERSION 2.8.12...3.17)
cmake_policy(VERSION 3.17)

if(NOT ITK_SOURCE_DIR)
  include(itk-module-init.cmake)
endif()

project(HDF5Container)

set(HDF5Container_LIBRARIES HDF5Container)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  include(ITKModuleExternal)
else()
  set(ITK_DIR ${CMAKE_BINARY_DIR})
  itk_module_impl()
endif()
