itk_module_test()
set(ITKIOHDF5ContainerTests
  itkHDF5ContainerImageIOTest.cxx
  itkHDF5ContainerImageIOStreamingReadWriteTest.cxx
)

CreateTestDriver(ITKIOHDF5Container  "${ITKIOHDF5Container-Test_LIBRARIES}" "${ITKIOHDF5ContainerTests}")

itk_add_test(NAME itkHDF5ContainerImageIOTest
  COMMAND ITKIOHDFContainer5TestDriver itkHDF5ContainerImageIOTest ${ITK_TEST_OUTPUT_DIR} )
itk_add_test(NAME itkHDF5ContainerImageIOStreamingReadWriteTest
  COMMAND ITKIOHDF5ContainerTestDriver itkHDF5ContainerImageIOStreamingReadWriteTest ${ITK_TEST_OUTPUT_DIR} )
