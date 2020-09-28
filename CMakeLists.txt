cmake_minimum_required(VERSION 3.17)

if(NOT ITK_SOURCE_DIR)
  include(itk-module-init.cmake)
endif()

project(ITKIOHDF5Container)

set(ITKIOHDF5Container_LIBRARIES ITKIOHDF5Container)

if(NOT ITK_SOURCE_DIR)
  find_package(ITK REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${ITK_CMAKE_DIR})
  include(ITKModuleExternal)
else()
  set(ITK_DIR ${CMAKE_BINARY_DIR})
  itk_module_impl()
endif()
