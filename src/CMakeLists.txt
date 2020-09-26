set(ITKIOHDF5Container_SRCS
  itkHDF5ContainerImageIOFactory.cxx
  itkHDF5ContainerImageIO.cxx
  )

itk_module_add_library(ITKIOHDF5Container ${ITKIOHDF5Container_SRCS})
