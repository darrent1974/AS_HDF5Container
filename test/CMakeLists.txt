itk_module_test()
set(HDF5ContainerTests
  itkHDF5ContainerImageIOTest.cxx
  itkHDF5ContainerImageIOStreamingReadWriteTest.cxx
)

CreateTestDriver(HDF5Container  "${HDF5Container-Test_LIBRARIES}" "${IOHDF5ContainerTests}")

itk_add_test(NAME itkHDF5ContainerImageIOTest
  COMMAND ITKIOHDFContainer5TestDriver itkHDF5ContainerImageIOTest ${ITK_TEST_OUTPUT_DIR} )
itk_add_test(NAME itkHDF5ContainerImageIOStreamingReadWriteTest
  COMMAND HDF5ContainerTestDriver itkHDF5ContainerImageIOStreamingReadWriteTest ${ITK_TEST_OUTPUT_DIR} )
