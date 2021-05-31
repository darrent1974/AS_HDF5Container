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
 *         web site
 * http://analyzedirect.com/support/10.0Documents/Analyze_Resource_01.pdf
 * \author Hans J. Johnson
 *         The University of Iowa 2002
 */

#ifndef itkHDF5ContainerImageIO_h
#define itkHDF5ContainerImageIO_h
#include "ITKIOHDF5ContainerExport.h"
#include "itkMetaDataDictionary.h"
#include "itkMetaDataObjectBase.h"

// itk namespace first suppresses
// kwstyle error for the H5 namespace below
namespace itk {}
namespace H5 {
class H5File;
class DataSpace;
class DataSet;
class Group;
class FileAccPropList;

} // namespace H5

#include "itkStreamingImageIOBase.h"

namespace itk {
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

class ITKIOHDF5Container_EXPORT HDF5ContainerImageIO
  : public StreamingImageIOBase
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

  /** Set/Get the name of the HDF5 path data will be read/written */
  itkSetStringMacro(Path);
  itkGetStringMacro(Path);
  itkSetStringMacro(DataSetName);
  itkGetStringMacro(DataSetName);
  itkSetMacro(Overwrite, bool);
  itkGetMacro(Overwrite, bool);
  itkBooleanMacro(Overwrite);
  itkSetMacro(ReCreate, bool);
  itkGetMacro(ReCreate, bool);
  itkBooleanMacro(ReCreate);
  itkGetMacro(UseChunking, bool);
  itkSetMacro(UseChunking, bool);
  itkBooleanMacro(UseChunking);
  itkGetMacro(UseMetaData, bool);
  itkSetMacro(UseMetaData, bool);
  itkBooleanMacro(UseMetaData);
  itkGetMacro(UseInferredDimensions, bool);
  itkGetMacro(UseDataSetOffset, bool);
  itkSetMacro(UseDataSetOffset, bool);
  itkBooleanMacro(UseDataSetOffset);
  itkGetMacro(UseDataSetSize, bool);
  itkSetMacro(UseDataSetSize, bool);
  itkBooleanMacro(UseDataSetSize);
  itkGetMacro(UseDataSetStride, bool);
  itkSetMacro(UseDataSetStride, bool);
  itkBooleanMacro(UseDataSetStride);

  std::vector<unsigned int>& GetDataSetOffset() { return m_DataSetOffset; }

  std::vector<unsigned int>& GetDataSetSize() { return m_DataSetSize; }

  std::vector<unsigned int>& GetDataSetStride() { return m_DataSetStride; }

  /*-------- This part of the interfaces deals with reading data. ----- */

  /** Determine if the file can be read with this ImageIO implementation.
   * \author Darren Thompson
   * \param FileNameToRead The name of the file to test for reading.
   * \post Sets classes ImageIOBase::m_FileName variable to be FileNameToWrite
   * \return Returns true if this ImageIO can read the file specified.
   */
  bool CanReadFile(const char* FileNameToRead) override;

  /** Set the spacing and dimension information for the set filename. */
  void ReadImageInformation() override;

  /** Reads the data from disk into the memory buffer provided. */
  void Read(void* buffer) override;

  /*-------- This part of the interfaces deals with writing data. ----- */

  /** Determine if the file can be written with this ImageIO implementation.
   * \param FileNameToWrite The name of the file to test for writing.
   * \author Darren Thompson
   * \post Sets classes ImageIOBase::m_FileName variable to be FileNameToWrite
   * \return Returns true if this ImageIO can write the file specified.
   */
  bool CanWriteFile(const char* FileNameToWrite) override;

  /** Set the spacing and dimension information for the set filename. */
  void WriteImageInformation() override;

  /** Writes the data to disk from the memory buffer provided. Make sure
   * that the IORegions has been set properly. */
  void Write(const void* buffer) override;

  bool DataSetExists();

protected:
  HDF5ContainerImageIO();
  ~HDF5ContainerImageIO() override;

  itkSetMacro(UseInferredDimensions, bool);
  itkBooleanMacro(UseInferredDimensions);

  SizeType GetHeaderSize() const override;

  void PrintSelf(std::ostream& os, Indent indent) const override;

private:
  void WriteString(const std::string& path, const std::string& value);
  void WriteString(const std::string& path, const char* s);
  void WriteStringAttr(H5::DataSet& ds,
                       const std::string& name,
                       const std::string& value);
  void WriteStringAttr(H5::DataSet& ds, const std::string& name, const char* s);
  std::string ReadString(const std::string& path);

  void WriteScalar(const std::string& path, const bool& value);
  void WriteScalarDataSetAttrib(H5::DataSet& ds,
                                const std::string& name,
                                const bool& value);

  void WriteScalar(const std::string& path, const long& value);
  void WriteScalar(const std::string& path, const unsigned long& value);
  void WriteScalar(const std::string& path, const long long& value);
  void WriteScalar(const std::string& path, const unsigned long long& value);

  template<typename TScalar>
  void WriteScalar(const std::string& path, const TScalar& value);

  template<typename TScalar>
  TScalar ReadScalar(const std::string& DataSetName);

  template<typename TScalar>
  void WriteVector(const std::string& path, const std::vector<TScalar>& vec);

  template<typename TScalar>
  void WriteVectorDataSetAttrib(H5::DataSet& ds,
                                const std::string& name,
                                const std::vector<TScalar>& vec);

  template<typename TScalar>
  std::vector<TScalar> ReadVector(const std::string& DataSetName);

  template<typename TScalar>
  std::vector<TScalar> ReadVectorDataSetAttrib(const H5::DataSet& ds,
                                               const std::string& name);

  void WriteDirections(const std::string& path,
                       const std::vector<std::vector<double>>& dir);

  std::vector<std::vector<double>> ReadDirections(const std::string& path);

  template<typename TType>
  bool WriteMeta(const std::string& name, MetaDataObjectBase* metaObj);

  template<typename TType>
  bool WriteMetaDataSetAttrib(H5::DataSet& ds,
                              const std::string& name,
                              MetaDataObjectBase* metaObj);

  template<typename TType>
  bool WriteMetaArray(const std::string& name, MetaDataObjectBase* metaObj);
  template<typename TType>
  void StoreMetaData(MetaDataDictionary* metaDict,
                     const std::string& HDFPath,
                     const std::string& name,
                     unsigned long numElements);
  void SetupStreaming(H5::DataSpace* imageSpace, H5::DataSpace* slabSpace);

  void CloseH5File();

  void ResetH5File(const H5::FileAccPropList fapl);
  std::vector<std::string> GetPathElements(const std::string& path);
  bool GetPathExists(const std::string& path);
  H5::Group GetGroup();
  H5::Group CreateGroupFromPath();
  std::string GetDataSetPath() const;
  H5::DataSet GetDataSet();

  void WriteDataSetAttributes(H5::DataSet ds);
  void ReadDataSetAttributes(const H5::DataSet& ds);
  void WriteDirectionsDataSetAttributes(
    H5::DataSet ds,
    const std::string& name,
    const std::vector<std::vector<double>>& dir);
  std::vector<std::vector<double>> ReadDirectionsDataSetAttributes(
    const H5::DataSet& ds,
    const std::string& name);

  void WriteImageMetaData(H5::Group& group, const MetaDataDictionary& metaDict);
  void ReadImageMetaData(MetaDataDictionary& metaDict);

  std::unique_ptr<H5::H5File> m_H5File{ nullptr };
  bool m_ImageInformationWritten{ false };
  std::string m_Path{ "/" };
  std::string m_DataSetName{ "/data" };
  bool m_Overwrite{ false };
  bool m_ReCreate{ false };
  bool m_UseChunking{ false };
  bool m_UseMetaData{ false };
  std::vector<unsigned int> m_DataSetOffset;
  std::vector<unsigned int> m_DataSetSize;
  std::vector<unsigned int> m_DataSetStride;
  bool m_UseDataSetOffset{ false };
  bool m_UseDataSetSize{ false };
  bool m_UseDataSetStride{ false };
  bool m_UseInferredDimensions{ false };
};
} // end namespace itk

#endif // itkHDF5ContainerImageIO_h
