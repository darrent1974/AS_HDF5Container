/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
/**
 *         The specification for this file format is taken from the
 *         web site http://analyzedirect.com/support/10.0Documents/Analyze_Resource_01.pdf
 * \author Hans J. Johnson
 *         The University of Iowa 2002
 */

#ifndef itkHDF5ContainerImageIO_h
#define itkHDF5ContainerImageIO_h
#include "ITKIOHDF5ContainerExport.h"
#include "itkMetaDataObjectBase.h"
#include "itkMetaDataDictionary.h"

// itk namespace first suppresses
// kwstyle error for the H5 namespace below
namespace itk
{
}
namespace H5
{
  class H5File;
  class DataSpace;
  class DataSet;
} // namespace H5

#include "itkStreamingImageIOBase.h"

namespace itk
{
  /**
 *\class HDF5ContainerImageIO
 *
 * \author Darren Thompson
 * \brief Class that defines how to read HDF5 file format.
 *
 * \ingroup ITKIOHDF5Container
 *
 * HDF5 paths for elements in file
 *
 *
 */

  class ITKIOHDF5Container_EXPORT HDF5ContainerImageIO : public StreamingImageIOBase
  {
  public:
    ITK_DISALLOW_COPY_AND_ASSIGN(HDF5ContainerImageIO);

    /** Standard class type aliases. */
    using Self = HDF5ContainerImageIO;
    using Superclass = StreamingImageIOBase;
    using Pointer = SmartPointer<Self>;

    /** Method for creation through the object factory. */
    itkNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(HDF5ContainerImageIO, StreamingImageIOBase);

    /*-------- This part of the interfaces deals with reading data. ----- */

    /** Determine if the file can be read with this ImageIO implementation.
   * \author Hans J Johnson
   * \param FileNameToRead The name of the file to test for reading.
   * \post Sets classes ImageIOBase::m_FileName variable to be FileNameToWrite
   * \return Returns true if this ImageIO can read the file specified.
   */
    bool
    CanReadFile(const char *FileNameToRead) override;

    /** Set the spacing and dimension information for the set filename. */
    void
    ReadImageInformation() override;

    /** Reads the data from disk into the memory buffer provided. */
    void
    Read(void *buffer) override;

    /*-------- This part of the interfaces deals with writing data. ----- */

    /** Determine if the file can be written with this ImageIO implementation.
   * \param FileNameToWrite The name of the file to test for writing.
   * \author Darren Thompson
   * \post Sets classes ImageIOBase::m_FileName variable to be FileNameToWrite
   * \return Returns true if this ImageIO can write the file specified.
   */
    bool
    CanWriteFile(const char *FileNameToWrite) override;

    /** Set the spacing and dimension information for the set filename. */
    void
    WriteImageInformation() override;

    /** Writes the data to disk from the memory buffer provided. Make sure
   * that the IORegions has been set properly. */
    void
    Write(const void *buffer) override;

  protected:
    HDF5ContainerImageIO();
    ~HDF5ContainerImageIO() override;

    SizeType
    GetHeaderSize() const override;

    void
    PrintSelf(std::ostream &os, Indent indent) const override;

  private:
    void
    WriteString(const std::string &path, const std::string &value);
    void
    WriteString(const std::string &path, const char *s);
    std::string
    ReadString(const std::string &path);

    void
    WriteScalar(const std::string &path, const bool &value);
    void
    WriteScalar(const std::string &path, const long &value);
    void
    WriteScalar(const std::string &path, const unsigned long &value);
    void
    WriteScalar(const std::string &path, const long long &value);
    void
    WriteScalar(const std::string &path, const unsigned long long &value);

    template <typename TScalar>
    void
    WriteScalar(const std::string &path, const TScalar &value);

    template <typename TScalar>
    TScalar
    ReadScalar(const std::string &DataSetName);

    template <typename TScalar>
    void
    WriteVector(const std::string &path, const std::vector<TScalar> &vec);

    template <typename TScalar>
    std::vector<TScalar>
    ReadVector(const std::string &DataSetName);

    void
    WriteDirections(const std::string &path, const std::vector<std::vector<double>> &dir);

    std::vector<std::vector<double>>
    ReadDirections(const std::string &path);

    template <typename TType>
    bool
    WriteMeta(const std::string &name, MetaDataObjectBase *metaObj);
    template <typename TType>
    bool
    WriteMetaArray(const std::string &name, MetaDataObjectBase *metaObj);
    template <typename TType>
    void
    StoreMetaData(MetaDataDictionary *metaDict,
                  const std::string &HDFPath,
                  const std::string &name,
                  unsigned long numElements);
    void
    SetupStreaming(H5::DataSpace *imageSpace, H5::DataSpace *slabSpace);

    void
    CloseH5File();
    void
    CloseDataSet();

    H5::H5File *m_H5File{nullptr};
    H5::DataSet *m_VoxelDataSet{nullptr};
    bool m_ImageInformationWritten{false};
  };
} // end namespace itk

#endif // itkHDF5ContainerImageIO_h
