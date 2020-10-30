set(DOCUMENTATION "This module contains an ImageIO class for reading and writing
ITK Images stored HDF5 containers, differing from the module in the <a href=\"http://www.hdfgroup.org/HDF5/\">HDF5</a>
data model and file format.")

itk_module(ITKIOHDF5Container
  ENABLE_SHARED
  DEPENDS
    ITKKWSys
    ITKIOImageBase
  PRIVATE_DEPENDS
    ITKHDF5
  TEST_DEPENDS
    ITKTestKernel
    ITKImageSources
  FACTORY_NAMES
    ImageIO::HDF5Container
  DESCRIPTION
    "${DOCUMENTATION}"
)
