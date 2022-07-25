set(HDF5Container_SRCS
  itkHDF5ContainerImageIOFactory.cxx
  itkHDF5ContainerImageIO.cxx
  )

itk_module_add_library(HDF5Container ${HDF5Container_SRCS})
