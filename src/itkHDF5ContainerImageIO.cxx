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
#include "itkVersion.h"
#include "itkHDF5ContainerImageIO.h"
#include "itkMetaDataObject.h"
#include "itkArray.h"
#include "itksys/SystemTools.hxx"
#include "itk_H5Cpp.h"

#include <vector>
#include <regex>
#include <string>
#include <algorithm>

namespace itk
{

HDF5ContainerImageIO::HDF5ContainerImageIO()
{
  const char * extensions[] = { ".hdf", ".h4", ".hdf4", ".h5", ".hdf5", ".he4", ".he5", ".hd5" };

  for (auto ext : extensions)
  {
    this->AddSupportedWriteExtension(ext);
    this->AddSupportedReadExtension(ext);
  }

  this->Self::SetMaximumCompressionLevel(9);
  this->Self::SetCompressionLevel(5);
}

HDF5ContainerImageIO::~HDF5ContainerImageIO()
{
  this->CloseH5File();
}

void
HDF5ContainerImageIO ::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "H5File: " << this->m_H5File << std::endl;
  os << indent << "Path: " << this->m_Path << std::endl;
  os << indent << "DataSetName: " << this->m_DataSetName << std::endl;

  if (m_Overwrite)
  {
    os << indent << "Overwrite: On" << std::endl;
  }
  else
  {
    os << indent << "Overwrite: Off" << std::endl;
  }

  if (m_UseChunking)
  {
    os << indent << "UseChunking: On" << std::endl;
  }
  else
  {
    os << indent << "UseChunking: Off" << std::endl;
  }

  if (m_UseMetaData)
  {
    os << indent << "UseMetaData: On" << std::endl;
  }
  else
  {
    os << indent << "UseMetaData: Off" << std::endl;
  }

  if (m_UseInferredDimensions)
  {
    os << indent << "UseInferredDimensions: On" << std::endl;
  }
  else
  {
    os << indent << "UseInferredDimensions: Off" << std::endl;
  }
}

//
// strings defining HDF file layout for image data.
namespace
{
const std::string Origin("Origin");
const std::string Directions("Directions");
const std::string Spacing("Spacing");
const std::string Dimensions("Dimension");
const std::string MetaDataName("ITKMetaData");

template <typename TScalar>
H5::PredType
GetType()
{
  itkGenericExceptionMacro(<< "Type not handled "
                           << "in HDF5 File: " << typeid(TScalar).name());
}
#define GetH5TypeSpecialize(CXXType, H5Type)                                                                           \
  template <>                                                                                                          \
  H5::PredType GetType<CXXType>()                                                                                      \
  {                                                                                                                    \
    return H5Type;                                                                                                     \
  }

GetH5TypeSpecialize(float, H5::PredType::NATIVE_FLOAT) GetH5TypeSpecialize(double, H5::PredType::NATIVE_DOUBLE)

  GetH5TypeSpecialize(char, H5::PredType::NATIVE_CHAR) GetH5TypeSpecialize(unsigned char, H5::PredType::NATIVE_UCHAR)

    GetH5TypeSpecialize(short int, H5::PredType::NATIVE_SHORT)
      GetH5TypeSpecialize(short unsigned int, H5::PredType::NATIVE_USHORT)

        GetH5TypeSpecialize(int, H5::PredType::NATIVE_INT) GetH5TypeSpecialize(unsigned int, H5::PredType::NATIVE_UINT)

          GetH5TypeSpecialize(long int, H5::PredType::NATIVE_LONG)
            GetH5TypeSpecialize(long unsigned int, H5::PredType::NATIVE_ULONG)

              GetH5TypeSpecialize(long long int, H5::PredType::NATIVE_LLONG)
                GetH5TypeSpecialize(unsigned long long int, H5::PredType::NATIVE_ULLONG)

/* The following types are not implemented.  This comment serves
 * to indicate that the full complement of possible H5::PredType
 * types are not implemented int the ITK IO reader/writer
 * GetH5TypeSpecialize(bool,              H5::PredType::NATIVE_HBOOL)
 */

#undef GetH5TypeSpecialize

                  inline IOComponentEnum PredTypeToComponentType(H5::DataType & type)
{
  if (type == H5::PredType::NATIVE_UCHAR)
  {
    return IOComponentEnum::UCHAR;
  }
  else if (type == H5::PredType::NATIVE_CHAR)
  {
    return IOComponentEnum::CHAR;
  }
  else if (type == H5::PredType::NATIVE_USHORT)
  {
    return IOComponentEnum::USHORT;
  }
  else if (type == H5::PredType::NATIVE_SHORT)
  {
    return IOComponentEnum::SHORT;
  }
  else if (type == H5::PredType::NATIVE_UINT)
  {
    return IOComponentEnum::UINT;
  }
  else if (type == H5::PredType::NATIVE_INT)
  {
    return IOComponentEnum::INT;
  }
  else if (type == H5::PredType::NATIVE_ULONG)
  {
    return IOComponentEnum::ULONG;
  }
  else if (type == H5::PredType::NATIVE_LONG)
  {
    return IOComponentEnum::LONG;
  }
  else if (type == H5::PredType::NATIVE_LLONG)
  {
    return IOComponentEnum::LONGLONG;
  }
  else if (type == H5::PredType::NATIVE_ULLONG)
  {
    return IOComponentEnum::ULONGLONG;
  }
  else if (type == H5::PredType::NATIVE_FLOAT)
  {
    return IOComponentEnum::FLOAT;
  }
  else if (type == H5::PredType::NATIVE_DOUBLE)
  {
    return IOComponentEnum::DOUBLE;
  }
  itkGenericExceptionMacro(<< "unsupported HDF5 data type with id " << type.getId());
}

H5::PredType
ComponentToPredType(IOComponentEnum cType)
{
  switch (cType)
  {
    case IOComponentEnum::UCHAR:
      return H5::PredType::NATIVE_UCHAR;
    case IOComponentEnum::CHAR:
      return H5::PredType::NATIVE_CHAR;
    case IOComponentEnum::USHORT:
      return H5::PredType::NATIVE_USHORT;
    case IOComponentEnum::SHORT:
      return H5::PredType::NATIVE_SHORT;
    case IOComponentEnum::UINT:
      return H5::PredType::NATIVE_UINT;
    case IOComponentEnum::INT:
      return H5::PredType::NATIVE_INT;
    case IOComponentEnum::ULONG:
      return H5::PredType::NATIVE_ULONG;
    case IOComponentEnum::LONG:
      return H5::PredType::NATIVE_LONG;
    case IOComponentEnum::ULONGLONG:
      return H5::PredType::NATIVE_ULLONG;
    case IOComponentEnum::LONGLONG:
      return H5::PredType::NATIVE_LLONG;
    case IOComponentEnum::FLOAT:
      return H5::PredType::NATIVE_FLOAT;
    case IOComponentEnum::DOUBLE:
      return H5::PredType::NATIVE_DOUBLE;
    case IOComponentEnum::LDOUBLE:
    case IOComponentEnum::UNKNOWNCOMPONENTTYPE:
      itkGenericExceptionMacro(<< "unsupported IOComponentEnum" << static_cast<char>(cType));
  }

  itkGenericExceptionMacro(<< "unsupported IOComponentEnum" << static_cast<char>(cType));
}

std::string
ComponentToString(IOComponentEnum cType)
{
  std::string rval;
  switch (cType)
  {
    case IOComponentEnum::UCHAR:
      rval = "UCHAR";
      break;
    case IOComponentEnum::CHAR:
      rval = "CHAR";
      break;
    case IOComponentEnum::USHORT:
      rval = "USHORT";
      break;
    case IOComponentEnum::SHORT:
      rval = "SHORT";
      break;
    case IOComponentEnum::UINT:
      rval = "UINT";
      break;
    case IOComponentEnum::INT:
      rval = "INT";
      break;
    case IOComponentEnum::ULONG:
      rval = "ULONG";
      break;
    case IOComponentEnum::LONG:
      rval = "LONG";
      break;
    case IOComponentEnum::ULONGLONG:
      rval = "ULONGLONG";
      break;
    case IOComponentEnum::LONGLONG:
      rval = "LONGLONG";
      break;
    case IOComponentEnum::FLOAT:
      rval = "FLOAT";
      break;
    case IOComponentEnum::DOUBLE:
      rval = "DOUBLE";
      break;
    default:
      itkGenericExceptionMacro(<< "unsupported IOComponentEnum" << static_cast<char>(cType));
  }
  return rval;
}
// Function:    H5Object::doesAttrExist
///\brief       test for existence of attribute
///\param       name - IN: Name of the attribute
///\return      true if attribute exists, false otherwise
///\exception   none
// Programmer   Kent Williams 2011
//--------------------------------------------------------------------------
static bool
doesAttrExist(const H5::H5Object & object, const char * const name)
{
  return (H5Aexists(object.getId(), name) > 0 ? true : false);
}

} // namespace

void
HDF5ContainerImageIO ::WriteScalar(const std::string & path, const bool & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = H5::PredType::NATIVE_HBOOL;
  H5::DataSet   scalarSet = this->m_H5File->createDataSet(path, scalarType, scalarSpace);
  //
  // HDF5 can't distinguish
  // between bool and int datasets
  // in a disk file. So add an attribute
  // labeling this as a bool
  const std::string isBoolName("isBool");
  H5::Attribute     isBool = scalarSet.createAttribute(isBoolName, scalarType, scalarSpace);
  bool              trueVal(true);
  isBool.write(scalarType, &trueVal);
  isBool.close();
  auto tempVal = static_cast<int>(value);
  scalarSet.write(&tempVal, scalarType);
  scalarSet.close();
}

void
HDF5ContainerImageIO ::WriteScalarDataSetAttrib(H5::DataSet & ds, const std::string & name, const bool & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = H5::PredType::NATIVE_HBOOL;

  if (ds.attrExists(name))
    itkExceptionMacro(<< "DataSet attribute already exists: " << name);

  H5::Attribute scalarAttrib = ds.createAttribute(name, scalarType, scalarSpace);

  bool tempVal = static_cast<bool>(value);
  scalarAttrib.write(scalarType, &tempVal);
  scalarAttrib.close();
}

void
HDF5ContainerImageIO ::WriteScalar(const std::string & path, const long & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = H5::PredType::NATIVE_INT;
  H5::PredType  attrType = H5::PredType::NATIVE_HBOOL;
  H5::DataSet   scalarSet = this->m_H5File->createDataSet(path, scalarType, scalarSpace);
  //
  // HDF5 can't distinguish
  // between long and int datasets
  // in a disk file. So add an attribute
  // labeling this as a long.
  const std::string isLongName("isLong");
  H5::Attribute     isLong = scalarSet.createAttribute(isLongName, attrType, scalarSpace);
  bool              trueVal(true);
  isLong.write(attrType, &trueVal);
  isLong.close();
  auto tempVal = static_cast<int>(value);
  scalarSet.write(&tempVal, scalarType);
  scalarSet.close();
}

void
HDF5ContainerImageIO ::WriteScalar(const std::string & path, const unsigned long & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = H5::PredType::NATIVE_UINT;
  H5::PredType  attrType = H5::PredType::NATIVE_HBOOL;
  H5::DataSet   scalarSet = this->m_H5File->createDataSet(path, scalarType, scalarSpace);
  //
  // HDF5 can't distinguish
  // between unsigned long and unsigned int datasets
  // in a disk file. So add an attribute
  // labeling this as an unsigned long.
  const std::string isUnsignedLongName("isUnsignedLong");
  H5::Attribute     isUnsignedLong = scalarSet.createAttribute(isUnsignedLongName, attrType, scalarSpace);
  bool              trueVal(true);
  isUnsignedLong.write(attrType, &trueVal);
  isUnsignedLong.close();
  auto tempVal = static_cast<int>(value);
  scalarSet.write(&tempVal, scalarType);
  scalarSet.close();
}

void
HDF5ContainerImageIO ::WriteScalar(const std::string & path, const long long & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = H5::PredType::STD_I64LE;
  H5::PredType  attrType = H5::PredType::NATIVE_HBOOL;
  H5::DataSet   scalarSet = this->m_H5File->createDataSet(path, scalarType, scalarSpace);
  //
  // HDF5 can't distinguish
  // between long and long long datasets
  // in a disk file. So add an attribute
  // labeling this as a long long
  const std::string isLLongName("isLLong");
  H5::Attribute     isLLong = scalarSet.createAttribute(isLLongName, attrType, scalarSpace);
  bool              trueVal(true);
  isLLong.write(attrType, &trueVal);
  isLLong.close();
  scalarSet.write(&value, scalarType);
  scalarSet.close();
}

void
HDF5ContainerImageIO ::WriteScalar(const std::string & path, const unsigned long long & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = H5::PredType::STD_U64LE;
  H5::PredType  attrType = H5::PredType::NATIVE_HBOOL;
  H5::DataSet   scalarSet = this->m_H5File->createDataSet(path, scalarType, scalarSpace);
  //
  // HDF5 can't distinguish
  // between unsigned long and unsigned long long
  // datasets in a disk file. So add an attribute
  // labeling this as a unsigned long long
  const std::string isULLongName("isULLong");
  H5::Attribute     isULLong = scalarSet.createAttribute(isULLongName, attrType, scalarSpace);
  bool              trueVal(true);
  isULLong.write(attrType, &trueVal);
  isULLong.close();
  scalarSet.write(&value, scalarType);
  scalarSet.close();
}

template <typename TScalar>
void
HDF5ContainerImageIO ::WriteScalar(const std::string & path, const TScalar & value)
{
  hsize_t       numScalars(1);
  H5::DataSpace scalarSpace(1, &numScalars);
  H5::PredType  scalarType = GetType<TScalar>();
  H5::DataSet   scalarSet = this->m_H5File->createDataSet(path, scalarType, scalarSpace);
  scalarSet.write(&value, scalarType);
  scalarSet.close();
}

template <typename TScalar>
TScalar
HDF5ContainerImageIO ::ReadScalar(const std::string & DataSetName)
{
  hsize_t       dim[1];
  H5::DataSet   scalarSet = this->m_H5File->openDataSet(DataSetName);
  H5::DataSpace Space = scalarSet.getSpace();

  if (Space.getSimpleExtentNdims() != 1)
  {
    itkExceptionMacro(<< "Wrong # of dims for TransformType "
                      << "in HDF5 File");
  }
  Space.getSimpleExtentDims(dim, nullptr);
  if (dim[0] != 1)
  {
    itkExceptionMacro(<< "Elements > 1 for scalar type "
                      << "in HDF5 File");
  }
  TScalar      scalar;
  H5::PredType scalarType = GetType<TScalar>();
  scalarSet.read(&scalar, scalarType);
  scalarSet.close();
  return scalar;
}

void
HDF5ContainerImageIO::WriteString(const std::string & path, const std::string & value)
{
  hsize_t       numStrings(1);
  H5::DataSpace strSpace(1, &numStrings);
  H5::StrType   strType(H5::PredType::C_S1, H5T_VARIABLE);
  H5::DataSet   strSet = this->m_H5File->createDataSet(path, strType, strSpace);
  strSet.write(value, strType);
  strSet.close();
}

void
HDF5ContainerImageIO::WriteString(const std::string & path, const char * s)
{
  std::string _s(s);
  WriteString(path, _s);
}

void
HDF5ContainerImageIO::WriteStringAttr(H5::DataSet & ds, const std::string & name, const std::string & value)
{
  H5::DataSpace strSpace(H5S_SCALAR);
  H5::StrType   strType(H5::PredType::C_S1, H5T_VARIABLE);

  H5::Attribute attrString(ds.createAttribute(name, strType, strSpace));
  // const H5::StrType strwritebuf(value);
  attrString.write(strType, value);
  attrString.close();
}

void
HDF5ContainerImageIO::WriteStringAttr(H5::DataSet & ds, const std::string & name, const char * s)
{
  std::string _s(s);
  WriteStringAttr(ds, name, _s);
}

std::string
HDF5ContainerImageIO ::ReadString(const std::string & path)
{
  std::string   rval;
  hsize_t       numStrings(1);
  H5::DataSpace strSpace(1, &numStrings);
  H5::StrType   strType(H5::PredType::C_S1, H5T_VARIABLE);
  H5::DataSet   strSet = this->m_H5File->openDataSet(path);
  strSet.read(rval, strType, strSpace);
  strSet.close();
  return rval;
}

template <typename TScalar>
void
HDF5ContainerImageIO::WriteVector(const std::string & path, const std::vector<TScalar> & vec)
{
  hsize_t       dim(vec.size());
  H5::DataSpace vecSpace(1, &dim);
  H5::PredType  vecType = GetType<TScalar>();
  H5::DataSet   vecSet = this->m_H5File->createDataSet(path, vecType, vecSpace);
  vecSet.write(vec.data(), vecType);
  vecSet.close();
}

template <typename TScalar>
void
HDF5ContainerImageIO::WriteVectorDataSetAttrib(H5::DataSet &                ds,
                                               const std::string &          name,
                                               const std::vector<TScalar> & vec)
{
  hsize_t       dim(vec.size());
  H5::DataSpace vecSpace(1, &dim);
  H5::PredType  vecType = GetType<TScalar>();
  H5::Attribute vecAttrib = ds.createAttribute(name, vecType, vecSpace);
  vecAttrib.write(vecType, vec.data());
  vecAttrib.close();
}

template <typename TScalar>
std::vector<TScalar>
HDF5ContainerImageIO::ReadVector(const std::string & DataSetName)
{
  std::vector<TScalar> vec;
  hsize_t              dim[1];
  H5::DataSet          vecSet = this->m_H5File->openDataSet(DataSetName);
  H5::DataSpace        Space = vecSet.getSpace();

  if (Space.getSimpleExtentNdims() != 1)
  {
    itkExceptionMacro(<< "Wrong # of dims for TransformType "
                      << "in HDF5 File");
  }
  Space.getSimpleExtentDims(dim, nullptr);
  vec.resize(dim[0]);
  H5::PredType vecType = GetType<TScalar>();
  vecSet.read(vec.data(), vecType);
  vecSet.close();
  return vec;
}

template <typename TScalar>
std::vector<TScalar>
HDF5ContainerImageIO::ReadVectorDataSetAttrib(const H5::DataSet & ds, const std::string & name)
{
  std::vector<TScalar> vec;
  hsize_t              dim[1];
  H5::Attribute        vecAttrib = ds.openAttribute(name);
  H5::DataSpace        Space = vecAttrib.getSpace();

  if (Space.getSimpleExtentNdims() != 1)
  {
    itkExceptionMacro(<< "Wrong # of dims for TransformType "
                      << "in HDF5 File");
  }
  Space.getSimpleExtentDims(dim, nullptr);
  vec.resize(dim[0]);
  H5::PredType vecType = GetType<TScalar>();
  vecAttrib.read(vecType, vec.data());
  vecAttrib.close();
  return vec;
}

void
HDF5ContainerImageIO::WriteDirections(const std::string & path, const std::vector<std::vector<double>> & dir)
{
  hsize_t dim[2];
  dim[1] = dir.size();
  dim[0] = dir[0].size();
  const std::unique_ptr<double[]> buf(new double[dim[0] * dim[1]]);
  unsigned                        k(0);
  for (unsigned i = 0; i < dim[1]; i++)
  {
    for (unsigned j = 0; j < dim[0]; j++)
    {
      buf[k] = dir[i][j];
      k++;
    }
  }

  H5::DataSpace dirSpace(2, dim);
  H5::DataSet   dirSet = this->m_H5File->createDataSet(path, H5::PredType::NATIVE_DOUBLE, dirSpace);
  dirSet.write(buf.get(), H5::PredType::NATIVE_DOUBLE);
  dirSet.close();
}

void
HDF5ContainerImageIO::WriteDirectionsDataSetAttributes(H5::DataSet                              ds,
                                                       const std::string &                      name,
                                                       const std::vector<std::vector<double>> & dir)
{
  hsize_t dim[2];
  dim[1] = dir.size();
  dim[0] = dir[0].size();
  const std::unique_ptr<double[]> buf(new double[dim[0] * dim[1]]);
  unsigned                        k(0);
  for (unsigned i = 0; i < dim[1]; i++)
  {
    for (unsigned j = 0; j < dim[0]; j++)
    {
      buf[k] = dir[i][j];
      k++;
    }
  }

  H5::DataSpace dirSpace(2, dim);
  H5::Attribute dirAttrib = ds.createAttribute(name, H5::PredType::NATIVE_DOUBLE, dirSpace);
  dirAttrib.write(H5::PredType::NATIVE_DOUBLE, buf.get());
  dirAttrib.close();
}

std::vector<std::vector<double>>
HDF5ContainerImageIO ::ReadDirections(const std::string & path)
{
  std::vector<std::vector<double>> rval;
  H5::DataSet                      dirSet = this->m_H5File->openDataSet(path);
  H5::DataSpace                    dirSpace = dirSet.getSpace();
  hsize_t                          dim[2];
  if (dirSpace.getSimpleExtentNdims() != 2)
  {
    itkExceptionMacro(<< " Wrong # of dims for Image Directions "
                      << "in HDF5 File");
  }
  dirSpace.getSimpleExtentDims(dim, nullptr);
  rval.resize(dim[1]);
  for (unsigned i = 0; i < dim[1]; i++)
  {
    rval[i].resize(dim[0]);
  }
  H5::FloatType dirType = dirSet.getFloatType();
  if (dirType.getSize() == sizeof(double))
  {
    const std::unique_ptr<double[]> buf(new double[dim[0] * dim[1]]);
    dirSet.read(buf.get(), H5::PredType::NATIVE_DOUBLE);
    int k = 0;
    for (unsigned i = 0; i < dim[1]; i++)
    {
      for (unsigned j = 0; j < dim[0]; j++)
      {
        rval[i][j] = buf[k];
        k++;
      }
    }
  }
  else
  {
    const std::unique_ptr<float[]> buf(new float[dim[0] * dim[1]]);
    dirSet.read(buf.get(), H5::PredType::NATIVE_FLOAT);
    int k = 0;
    for (unsigned i = 0; i < dim[1]; i++)
    {
      for (unsigned j = 0; j < dim[0]; j++)
      {
        rval[i][j] = buf[k];
        k++;
      }
    }
  }
  dirSet.close();
  return rval;
}

std::vector<std::vector<double>>
HDF5ContainerImageIO ::ReadDirectionsDataSetAttributes(const H5::DataSet & ds, const std::string & name)
{
  std::vector<std::vector<double>> rval;
  H5::Attribute                    dirAttrib = ds.openAttribute(name);
  H5::DataSpace                    dirSpace = dirAttrib.getSpace();
  hsize_t                          dim[2];
  if (dirSpace.getSimpleExtentNdims() != 2)
  {
    itkExceptionMacro(<< " Wrong # of dims for Image Directions "
                      << "in HDF5 File");
  }
  dirSpace.getSimpleExtentDims(dim, nullptr);
  rval.resize(dim[1]);
  for (unsigned i = 0; i < dim[1]; i++)
  {
    rval[i].resize(dim[0]);
  }
  H5::FloatType dirType = dirAttrib.getFloatType();
  if (dirType.getSize() == sizeof(double))
  {
    const std::unique_ptr<double[]> buf(new double[dim[0] * dim[1]]);
    dirAttrib.read(H5::PredType::NATIVE_DOUBLE, buf.get());
    int k = 0;
    for (unsigned i = 0; i < dim[1]; i++)
    {
      for (unsigned j = 0; j < dim[0]; j++)
      {
        rval[i][j] = buf[k];
        k++;
      }
    }
  }
  else
  {
    const std::unique_ptr<float[]> buf(new float[dim[0] * dim[1]]);
    dirAttrib.read(H5::PredType::NATIVE_FLOAT, buf.get());
    int k = 0;
    for (unsigned i = 0; i < dim[1]; i++)
    {
      for (unsigned j = 0; j < dim[0]; j++)
      {
        rval[i][j] = buf[k];
        k++;
      }
    }
  }
  dirAttrib.close();
  return rval;
}

template <typename TType>
void
HDF5ContainerImageIO ::StoreMetaData(MetaDataDictionary * metaDict,
                                     const std::string &  HDFPath,
                                     const std::string &  name,
                                     unsigned long        numElements)
{
  if (numElements == 1)
  {
    auto val = this->ReadScalar<TType>(HDFPath);
    EncapsulateMetaData<TType>(*metaDict, name, val);
  }
  else
  {
    //
    // store as itk::Array -- consistent with how
    // metadatadictionary actually is used in ITK
    std::vector<TType> valVec = this->ReadVector<TType>(HDFPath);
    Array<TType>       val(static_cast<typename Array<TType>::SizeValueType>(valVec.size()));
    for (unsigned int i = 0; i < val.GetSize(); i++)
    {
      val[i] = valVec[i];
    }
    EncapsulateMetaData<Array<TType>>(*metaDict, name, val);
  }
}

bool
HDF5ContainerImageIO ::CanWriteFile(const char * name)
{
  return this->HasSupportedWriteExtension(name);
}

// This method will only test if the header looks like an
// HDF5 Header.  Some code is redundant with ReadImageInformation
// a StateMachine could provide a better implementation
bool
HDF5ContainerImageIO ::CanReadFile(const char * FileNameToRead)
{
  // HDF5 is overly verbose in complaining that
  //     a file does not exist.
  if (!itksys::SystemTools::FileExists(FileNameToRead))
  {
    return false;
  }

  // HDF5 is so exception happy, we have to worry about
  // it throwing a wobbly here if the file doesn't exist
  // or has some other problem.
  bool rval = true;
  try
  {

    htri_t ishdf5 = H5Fis_hdf5(FileNameToRead);

    if (ishdf5 <= 0)
    {
      return false;
    }

    H5::H5File h5file(FileNameToRead, H5F_ACC_RDONLY);
  }
  catch (...)
  {
    rval = false;
  }
  return rval;
}

void
HDF5ContainerImageIO ::CloseH5File()
{
  if (this->m_H5File != nullptr)
  {
    this->m_H5File->close();
    delete this->m_H5File;
    this->m_H5File = nullptr;
  }
}

void
HDF5ContainerImageIO ::ReadImageInformation()
{
  try
  {
    this->CloseH5File();

    // Open file as read-only
    this->m_H5File = new H5::H5File(this->GetFileName(), H5F_ACC_RDONLY);

    // Check for the existence of the path
    if (!this->GetPathExists(this->GetPath()))
      itkExceptionMacro(<< this->GetPath() << " does not exist");

    H5::DataSet ds(this->GetDataSet());

    // Intialise the image by reading all
    // ITK related dataset attributes
    this->ReadDataSetAttributes(ds);

    //
    // read metadata
    MetaDataDictionary & metaDict(this->GetMetaDataDictionary());
    // Necessary to clear dict if ImageIO object is re-used
    metaDict.Clear();

    if (this->GetUseMetaData())
    {
      // Check if MetaData group exists, throw exception if not
      if (!this->GetGroup().exists(MetaDataName))
        itkExceptionMacro(<< MetaDataName << " does not exist");

      // Read and populate MetaData dict
      this->ReadImageMetaData(metaDict);
    }
  }
  // catch failure caused by the H5File operations
  catch (H5::AttributeIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  catch (H5::FileIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSet operations
  catch (H5::DataSetIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataSpaceIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataTypeIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
}

void
HDF5ContainerImageIO ::ReadImageMetaData(MetaDataDictionary & metaDict)
{
  // Construct MetaData path
  std::string MetaDataGroupName(this->GetPath());
  MetaDataGroupName += ("/" + MetaDataName + "/");

  itkDebugMacro(<< "MetaDataGroupName: " << MetaDataGroupName);

  H5::Group metaGroup(this->m_H5File->openGroup(MetaDataGroupName));
  for (unsigned int i = 0; i < metaGroup.getNumObjs(); i++)
  {
    H5std_string name = metaGroup.getObjnameByIdx(i);

    itkDebugMacro(<< "Reading MetaData item: " << name);

    std::string localMetaDataName(MetaDataGroupName);
    localMetaDataName += name;
    H5::DataSet   metaDataSet = this->m_H5File->openDataSet(localMetaDataName);
    H5::DataType  metaDataType = metaDataSet.getDataType();
    H5::DataSpace metaDataSpace = metaDataSet.getSpace();
    if (metaDataSpace.getSimpleExtentNdims() != 1)
    {
      // ignore > 1D metadata
      continue;
    }
    hsize_t metaDataDims[1];
    metaDataSpace.getSimpleExtentDims(metaDataDims);
    //
    // work around bool/int confusion on disk.
    if (metaDataType == H5::PredType::NATIVE_INT)
    {
      if (doesAttrExist(metaDataSet, "isBool"))
      {
        // itk::Array<bool> apparently can't
        // happen because vnl_vector<bool> isn't
        // instantiated
        bool val;
        auto tmpVal = this->ReadScalar<int>(localMetaDataName);
        val = tmpVal != 0;
        EncapsulateMetaData<bool>(metaDict, name, val);
      }
      else if (doesAttrExist(metaDataSet, "isLong"))
      {
        auto val = this->ReadScalar<long>(localMetaDataName);
        EncapsulateMetaData<long>(metaDict, name, val);
      }
      else if (doesAttrExist(metaDataSet, "isUnsignedLong"))
      {
        auto val = this->ReadScalar<unsigned long>(localMetaDataName);
        EncapsulateMetaData<unsigned long>(metaDict, name, val);
      }
      else
      {
        this->StoreMetaData<int>(&metaDict, localMetaDataName, name, metaDataDims[0]);
      }
    }
    else if (metaDataType == H5::PredType::NATIVE_CHAR)
    {
      this->StoreMetaData<char>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else if (metaDataType == H5::PredType::NATIVE_UCHAR)
    {
      if (doesAttrExist(metaDataSet, "isBool"))
      {
        // itk::Array<bool> apparently can't
        // happen because vnl_vector<bool> isn't
        // instantiated
        bool val;
        auto tmpVal = this->ReadScalar<int>(localMetaDataName);
        val = tmpVal != 0;
        EncapsulateMetaData<bool>(metaDict, name, val);
      }
      else
      {
        this->StoreMetaData<unsigned char>(&metaDict, localMetaDataName, name, metaDataDims[0]);
      }
    }
    else if (metaDataType == H5::PredType::NATIVE_SHORT)
    {
      this->StoreMetaData<short>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else if (metaDataType == H5::PredType::NATIVE_USHORT)
    {
      this->StoreMetaData<unsigned short>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else if (metaDataType == H5::PredType::NATIVE_UINT)
    {
      if (doesAttrExist(metaDataSet, "isUnsignedLong"))
      {
        auto val = this->ReadScalar<unsigned long>(localMetaDataName);
        EncapsulateMetaData<unsigned long>(metaDict, name, val);
      }
      else
      {
        this->StoreMetaData<unsigned int>(&metaDict, localMetaDataName, name, metaDataDims[0]);
      }
    }
    else if (metaDataType == H5::PredType::NATIVE_LONG)
    {
      if (doesAttrExist(metaDataSet, "isLLong"))
      {
        auto val = this->ReadScalar<long long>(localMetaDataName);
        EncapsulateMetaData<long long>(metaDict, name, val);
      }
      else
      {
        this->StoreMetaData<long>(&metaDict, localMetaDataName, name, metaDataDims[0]);
      }
    }
    else if (metaDataType == H5::PredType::NATIVE_ULONG)
    {
      if (doesAttrExist(metaDataSet, "isULLong"))
      {
        auto val = this->ReadScalar<unsigned long long>(localMetaDataName);
        EncapsulateMetaData<unsigned long long>(metaDict, name, val);
      }
      else
      {
        this->StoreMetaData<unsigned long>(&metaDict, localMetaDataName, name, metaDataDims[0]);
      }
    }
    else if (metaDataType == H5::PredType::NATIVE_LLONG)
    {
      this->StoreMetaData<long long int>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else if (metaDataType == H5::PredType::NATIVE_ULLONG)
    {
      this->StoreMetaData<unsigned long long int>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else if (metaDataType == H5::PredType::NATIVE_FLOAT)
    {
      this->StoreMetaData<float>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else if (metaDataType == H5::PredType::NATIVE_DOUBLE)
    {
      this->StoreMetaData<double>(&metaDict, localMetaDataName, name, metaDataDims[0]);
    }
    else
    {
      H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
      if (metaDataType == strType)
      {
        std::string val = this->ReadString(localMetaDataName);
        EncapsulateMetaData<std::string>(metaDict, name, val);
      }
    }
  }
}

void
HDF5ContainerImageIO ::SetupStreaming(H5::DataSpace * imageSpace, H5::DataSpace * slabSpace)
{
  ImageIORegion                    regionToRead = this->GetIORegion();
  ImageIORegion::SizeType          size = regionToRead.GetSize();
  ImageIORegion::IndexType         start = regionToRead.GetIndex();
  int                              numComponents = this->GetNumberOfComponents();
  const int                        HDFDim(this->GetNumberOfDimensions() + (numComponents > 1 ? 1 : 0));
  const std::unique_ptr<hsize_t[]> offset(new hsize_t[HDFDim]);
  const std::unique_ptr<hsize_t[]> stride(new hsize_t[HDFDim]);
  const std::unique_ptr<hsize_t[]> HDFSize(new hsize_t[HDFDim]);
  const int                        limit = regionToRead.GetImageDimension();

  //
  // fastest moving dimension is intra-voxel
  // index
  int i = 0;
  if (numComponents > 1)
  {
    offset[HDFDim - 1] = 0;
    HDFSize[HDFDim - 1] = numComponents;
    ++i;
  }

  for (int j = 0; j < limit && i < HDFDim; ++i, ++j)
  {
    // Set dataspace properties from user-specified or existing values
    offset[HDFDim - i - 1] = this->GetUseDataSetOffset() ? m_DataSetOffset[j] : start[j];
    HDFSize[HDFDim - i - 1] = this->GetUseDataSetSize() ? m_DataSetSize[j] : size[j];
    stride[HDFDim - i - 1] = this->GetUseDataSetStride() ? m_DataSetStride[j] : 1;
  }

  while (i < HDFDim)
  {
    offset[HDFDim - i - 1] = 0;
    HDFSize[HDFDim - i - 1] = 1;
    ++i;
  }

  slabSpace->setExtentSimple(HDFDim, HDFSize.get());
  imageSpace->selectHyperslab(H5S_SELECT_SET, HDFSize.get(), offset.get(), stride.get());
}

void
HDF5ContainerImageIO ::Read(void * buffer)
{
  ImageIORegion            regionToRead = this->GetIORegion();
  ImageIORegion::SizeType  size = regionToRead.GetSize();
  ImageIORegion::IndexType start = regionToRead.GetIndex();

  itkDebugMacro(<< "regionToRead: " << regionToRead);

  // Get dataset
  H5::DataSet ds(this->GetDataSet());

  H5::DataType  voxelType = ds.getDataType();
  H5::DataSpace imageSpace = ds.getSpace();

  H5::DataSpace dspace;
  this->SetupStreaming(&imageSpace, &dspace);

  try
  {
    ds.read(buffer, voxelType, dspace, imageSpace);
  }
  catch (H5::AttributeIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  catch (H5::FileIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSet operations
  catch (H5::DataSetIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataSpaceIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataTypeIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  catch (H5::Exception & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
}

template <typename TType>
bool
HDF5ContainerImageIO ::WriteMeta(const std::string & name, MetaDataObjectBase * metaObjBase)
{
  auto * metaObj = dynamic_cast<MetaDataObject<TType> *>(metaObjBase);
  if (metaObj == nullptr)
  {
    return false;
  }
  TType val = metaObj->GetMetaDataObjectValue();
  this->WriteScalar(name, val);
  return true;
}

template <typename TType>
bool
HDF5ContainerImageIO ::WriteMetaDataSetAttrib(H5::DataSet &        ds,
                                              const std::string &  name,
                                              MetaDataObjectBase * metaObjBase)
{
  auto * metaObj = dynamic_cast<MetaDataObject<TType> *>(metaObjBase);
  if (metaObj == nullptr)
  {
    return false;
  }
  TType val = metaObj->GetMetaDataObjectValue();
  this->WriteScalarDataSetAttrib(ds, name, val);
  return true;
}

template <typename TType>
bool
HDF5ContainerImageIO ::WriteMetaArray(const std::string & name, MetaDataObjectBase * metaObjBase)
{
  using MetaDataArrayObject = MetaDataObject<Array<TType>>;
  auto * metaObj = dynamic_cast<MetaDataArrayObject *>(metaObjBase);
  if (metaObj == nullptr)
  {
    return false;
  }
  Array<TType>       val = metaObj->GetMetaDataObjectValue();
  std::vector<TType> vecVal(val.GetSize());
  for (unsigned i = 0; i < val.size(); i++)
  {
    vecVal[i] = val[i];
  }
  this->WriteVector(name, vecVal);
  return true;
}

H5::H5File *
HDF5ContainerImageIO::GetH5File(const H5::FileAccPropList fapl)
{
  H5::H5File * file{ nullptr };

  // Check for an existing HDF5 file, it it already exists re-open, rather
  // than re-creating it.
  if (!itksys::SystemTools::FileExists(this->GetFileName()))
  {
    try
    {
      // Attempt to create a new file
      file = new H5::H5File(this->GetFileName(), H5F_ACC_TRUNC, H5::FileCreatPropList::DEFAULT, fapl);

      return file;
    }
    catch (H5::FileIException & error)
    {
      itkExceptionMacro(<< error.getCDetailMsg());
    }
  }

  try
  {
    // Attempt to reopen an existing file in read/write mode
    file = new H5::H5File(this->GetFileName(), H5F_ACC_RDWR, H5::FileCreatPropList::DEFAULT, fapl);

    return file;
  }
  catch (H5::FileIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  catch (H5::Exception & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }

  return file;
}

std::vector<std::string>
HDF5ContainerImageIO::GetPathElements(const std::string & path)
{
  // Regex for tokenizing strings separated by backslashes
  std::regex reg("/");

  // Get an iterator after filtering through the regex
  std::sregex_token_iterator iter(path.begin(), path.end(), reg, -1);

  // Keep a dummy end iterator - Needed to construct a vector
  // using (start, end) iterators.
  std::sregex_token_iterator end;
  std::vector<std::string>   vec_path_elements(iter, end);

  // Remove any empty strings
  vec_path_elements.erase(std::remove(vec_path_elements.begin(), vec_path_elements.end(), ""), vec_path_elements.end());

  return vec_path_elements;
}

bool
HDF5ContainerImageIO::GetPathExists(const std::string & path)
{
  try
  {
    H5::Exception::dontPrint();
    return this->m_H5File->exists(path);
  }
  catch (H5::Exception & error)
  {
    return false;
  }
}

H5::Group
HDF5ContainerImageIO::CreateGroupFromPath()
{
  H5::Group group;

  try
  {
    H5::Exception::dontPrint();

    // Get a vector of individual path element strings
    std::vector<std::string> vec_path_elements(this->GetPathElements(this->GetPath()));

    std::string path_incremental;

    // Iterate through the elements creating groups
    // by appending the path elements
    for (auto elem : vec_path_elements)
    {
      // Append backslash + element
      path_incremental += ("/" + elem);

      // Skip creating, if it already exists
      if (this->GetPathExists(path_incremental))
        continue;

      group = this->m_H5File->createGroup(path_incremental);
      itkDebugMacro(<< "Created group: " << path_incremental);
    }

    return group;
  }
  catch (H5::Exception & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
}

H5::Group
HDF5ContainerImageIO::GetGroup()
{
  if (this->GetPathExists(this->GetPath()))
  {
    try
    {
      return this->m_H5File->openGroup(this->GetPath());
    }
    catch (H5::Exception & error)
    {
      itkExceptionMacro(<< error.getCDetailMsg());
    }
  }

  // Create and return the group
  return this->CreateGroupFromPath();
}

std::string
HDF5ContainerImageIO::GetDataSetPath() const
{
  return std::string(this->GetPath()) + "/" + std::string(this->GetDataSetName());
}

H5::DataSet
HDF5ContainerImageIO::GetDataSet()
{
  return this->m_H5File->openDataSet(this->GetDataSetPath());
}

void
HDF5ContainerImageIO::WriteDataSetAttributes(H5::DataSet ds)
{
  // Write ITK specific dataset attributes
  this->WriteVectorDataSetAttrib(ds, Origin, this->m_Origin);
  this->WriteVectorDataSetAttrib(ds, Spacing, this->m_Spacing);
  this->WriteVectorDataSetAttrib(ds, Dimensions, this->m_Dimensions);
  this->WriteDirectionsDataSetAttributes(ds, Directions, this->m_Direction);
}

void
HDF5ContainerImageIO::ReadDataSetAttributes(const H5::DataSet & ds)
{
  // Read ITK specific dataset attributes, if the attribute does not
  // exist then default values will be assumed.

  // set the componentType
  H5::DataType imageVoxelType(ds.getDataType());
  this->m_ComponentType = PredTypeToComponentType(imageVoxelType);
  itkDebugMacro(<< "Component Type: " << this->m_ComponentType);

  H5::DataSpace                    space(ds.getSpace());
  hsize_t                          nInferredDims(space.getSimpleExtentNdims());
  hsize_t                          nDims;
  const std::unique_ptr<hsize_t[]> Dims(new hsize_t[nInferredDims]);

  space.getSimpleExtentDims(Dims.get());

  if (ds.attrExists(Dimensions))
  {
    // Image dimensions are specified explicitely
    this->UseInferredDimensionsOff();
    this->m_Dimensions = this->ReadVectorDataSetAttrib<ImageIOBase::SizeValueType>(ds, Dimensions);

    // Dimensionality is defined by the vector size
    nDims = this->m_Dimensions.size();
    this->SetNumberOfDimensions(nDims);

    // Account for non-scalar image datasets
    if (nInferredDims > this->GetNumberOfDimensions())
      this->SetNumberOfComponents(Dims[nDims - 1]);
  }
  else
  {
    // Image dimensions will be inferred by querying the
    // datasets dataspace. Only scalar images are supported
    // for this method as there is not enough information to
    // determine the number of vector components
    this->UseInferredDimensionsOn();

    nDims = nInferredDims;

    this->SetNumberOfDimensions(nDims);
    this->SetNumberOfComponents(1);

    itkDebugMacro(<< "Number of inferred dimensions: " << this->GetNumberOfDimensions());

    // Set image dimensions (reverse order)
    for (hsize_t i = 0; i < nDims; i++)
      this->SetDimensions(nDims - i - 1, Dims[i]);
  }

  // Check parameters
  if (this->GetUseDataSetSize())
  {
    if (m_DataSetSize.size() != nDims)
      itkExceptionMacro(<< "Invalid size dimension: " << nDims);

    // Override dimensions with user specified values
    for (hsize_t i = 0; i < nDims; i++)
      this->SetDimensions(i, m_DataSetSize[i]);
  }

  if (ds.attrExists(Directions))
    this->m_Direction = this->ReadDirectionsDataSetAttributes(ds, Directions);

  if (ds.attrExists(Origin))
    this->m_Origin = this->ReadVectorDataSetAttrib<double>(ds, Origin);

  if (ds.attrExists(Spacing))
    this->m_Spacing = this->ReadVectorDataSetAttrib<double>(ds, Spacing);

  if (this->GetUseDataSetOffset())
  {
    if (m_DataSetOffset.size() != nDims)
      itkExceptionMacro(<< "Invalid offset dimension: " << nDims);
  }

  if (this->GetUseDataSetStride())
  {
    if (m_DataSetStride.size() != nDims)
      itkExceptionMacro(<< "Invalid stride dimension: " << nDims);

    // If a user-specified stride is used then spacing needs to be adjusted
    for (hsize_t i = 0; i < nDims; i++)
      this->m_Spacing[i] *= (double)m_DataSetStride[i];

    if (!this->GetUseDataSetSize())
    {
      // In this case a stride is specified, but no corresponding explicit
      // size is given. Therefore, compute the dataset size to read
      // (max size/stride - offset)
      for (hsize_t i = 0; i < nDims; i++)
        if (m_DataSetStride[i] > 1)
        {
          size_t offset(this->GetUseDataSetOffset() ? m_DataSetOffset[i] : 0);
          this->SetDimensions(i, this->GetDimensions(i) / m_DataSetStride[i] - offset);
        }
    }
  }

  this->Modified();

  itkDebugMacro(<< *this);
}

void
HDF5ContainerImageIO::WriteImageMetaData(H5::Group & group, const MetaDataDictionary & metaDict)
{
  // Create the image dataset in the group
  H5::Group groupMetaData = group.createGroup(MetaDataName);

  // Create the full path string of the MetaData group
  std::string objBaseName(this->GetPath());
  objBaseName += ("/" + MetaDataName + "/");

  auto it = metaDict.Begin(), end = metaDict.End();

  // Loop through ITK MetaData
  for (; it != end; it++)
  {
    MetaDataObjectBase * metaObj = it->second.GetPointer();
    std::string          objName(objBaseName + it->first);

    itkDebugMacro(<< "Creating MetaData dataset: " << objName);

    // scalars
    if (this->WriteMeta<bool>(objName, metaObj))
      continue;

    if (this->WriteMeta<char>(objName, metaObj))
      continue;

    if (this->WriteMeta<unsigned char>(objName, metaObj))
      continue;

    if (this->WriteMeta<short>(objName, metaObj))
      continue;

    if (this->WriteMeta<unsigned short>(objName, metaObj))
      continue;

    if (this->WriteMeta<int>(objName, metaObj))
      continue;

    if (this->WriteMeta<unsigned int>(objName, metaObj))
      continue;

    if (this->WriteMeta<long>(objName, metaObj))
      continue;

    if (this->WriteMeta<unsigned long>(objName, metaObj))
      continue;

    if (this->WriteMeta<long long int>(objName, metaObj))
      continue;

    if (this->WriteMeta<unsigned long long int>(objName, metaObj))
      continue;

    if (this->WriteMeta<float>(objName, metaObj))
      continue;

    if (this->WriteMeta<double>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<char>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<unsigned char>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<short>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<unsigned short>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<int>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<unsigned int>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<long>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<unsigned long>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<float>(objName, metaObj))
      continue;

    if (this->WriteMetaArray<double>(objName, metaObj))
      continue;

    //
    // C String Arrays
    {
      auto * cstringObj = dynamic_cast<MetaDataObject<char *> *>(metaObj);
      auto * constCstringObj = dynamic_cast<MetaDataObject<const char *> *>(metaObj);
      if (cstringObj != nullptr || constCstringObj != nullptr)
      {
        const char * val;
        if (cstringObj != nullptr)
        {
          val = cstringObj->GetMetaDataObjectValue();
        }
        else
        {
          val = constCstringObj->GetMetaDataObjectValue();
        }
        this->WriteString(objName, val);
        continue;
      }
    }
    //
    // std::string
    {
      auto * stdStringObj = dynamic_cast<MetaDataObject<std::string> *>(metaObj);
      if (stdStringObj != nullptr)
      {
        std::string val = stdStringObj->GetMetaDataObjectValue();
        this->WriteString(objName, val);
        continue;
      }
    }
  }
}

void
HDF5ContainerImageIO::WriteImageInformation()
{
  //
  // guard so that image information
  if (this->m_ImageInformationWritten)
  {
    return;
  }

  try
  {
    this->CloseH5File();

    H5::FileAccPropList fapl;
#if (H5_VERS_MAJOR > 1) || (H5_VERS_MAJOR == 1) && (H5_VERS_MINOR > 10) ||                                             \
  (H5_VERS_MAJOR == 1) && (H5_VERS_MINOR == 10) && (H5_VERS_RELEASE >= 2)
    // File format which is backwards compatible with HDF5 version 1.8
    // Only HDF5 v1.10.2 has both setLibverBounds method and H5F_LIBVER_V18 constant
    fapl.setLibverBounds(H5F_LIBVER_V18, H5F_LIBVER_V18);
#elif (H5_VERS_MAJOR == 1) && (H5_VERS_MINOR == 10) && (H5_VERS_RELEASE < 2)
#  error The selected version of HDF5 library does not support setting backwards compatibility at run-time.\
  Please use a different version of HDF5, e.g. the one bundled with ITK (by setting ITK_USE_SYSTEM_HDF5 to OFF).
#endif

    this->m_H5File = this->GetH5File(fapl);

    H5::Group group = this->GetGroup();

    // First, check if the dataset already exists
    if (!this->GetOverwrite() && group.nameExists(this->GetDataSetName()))
      itkExceptionMacro("DataSet: " << this->GetDataSetName() << ", already exists");

    // Second, check if any ITK image metadata already exists
    if (!this->GetOverwrite() && group.nameExists(MetaDataName))
      itkExceptionMacro(<< MetaDataName << ", already exists");

    hsize_t numComponents = this->GetNumberOfComponents();
    hsize_t numDims = this->GetNumberOfDimensions();

    // HDF5 dimensions listed slowest moving first, ITK are fastest
    // moving first.
    std::unique_ptr<hsize_t[]> dims(new hsize_t[numDims + (numComponents == 1 ? 0 : 1)]);

    for (hsize_t i(0), j(numDims - 1); i < numDims; i++, j--)
    {
      dims[j] = this->m_Dimensions[i];
    }
    if (numComponents > 1)
    {
      dims[numDims] = numComponents;
      numDims++;
    }
    H5::DataSpace imageSpace(numDims, dims.get());
    H5::PredType  dataType = ComponentToPredType(this->GetComponentType());

    H5::DSetCreatPropList plist;

    if (this->GetUseCompression())
    {
      // Set compression level
      plist.setDeflate(this->GetCompressionLevel());
    }

    if (this->GetUseChunking())
    {
      // If chunking is selected set the chunk
      // size to be the N-1 dimension region
      dims[0] = 1;
      plist.setChunk(numDims, dims.get());
      dims.reset();
    }

    // Create the image dataset in the group
    H5::DataSet ds;

    ds = group.createDataSet(this->GetDataSetName(), dataType, imageSpace, plist);

    // Write ITK image specific attributes to the dataset
    this->WriteDataSetAttributes(ds);

    // Write ITK MetaData to the dataset in subgroup
    if (this->GetUseMetaData())
      this->WriteImageMetaData(group, this->GetMetaDataDictionary());
  }
  // catch failure caused by the H5File operations
  catch (H5::FileIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSet operations
  catch (H5::DataSetIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataSpaceIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataTypeIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  //
  // only write image information once.
  this->m_ImageInformationWritten = true;
}

/**
 * Write the image Information before writing data
 */
void
HDF5ContainerImageIO ::Write(const void * buffer)
{
  this->WriteImageInformation();

  try
  {
    int numComponents = this->GetNumberOfComponents();
    int numDims = this->GetNumberOfDimensions();
    // HDF5 dimensions listed slowest moving first, ITK are fastest
    // moving first.
    const std::unique_ptr<hsize_t[]> dims(new hsize_t[numDims + (numComponents == 1 ? 0 : 1)]);

    for (int i(0), j(numDims - 1); i < numDims; i++, j--)
    {
      dims[j] = this->m_Dimensions[i];
    }
    if (numComponents > 1)
    {
      dims[numDims] = numComponents;
      numDims++;
    }
    H5::DataSpace imageSpace(numDims, dims.get());
    H5::PredType  dataType = ComponentToPredType(this->GetComponentType());
    H5::DataSpace dspace;
    this->SetupStreaming(&imageSpace, &dspace);

    H5::DataSet ds(this->GetDataSet());

    ds.write(buffer, dataType, dspace, imageSpace);
  }
  // catch failure caused by the H5File operations
  catch (H5::FileIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSet operations
  catch (H5::DataSetIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataSpaceIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
  // catch failure caused by the DataSpace operations
  catch (H5::DataTypeIException & error)
  {
    itkExceptionMacro(<< error.getCDetailMsg());
  }
}

//
// GetHeaderSize -- return 0
ImageIOBase::SizeType
HDF5ContainerImageIO ::GetHeaderSize() const
{
  return 0;
}

bool
HDF5ContainerImageIO ::DataSetExists()
{
  try
  {
    this->CloseH5File();

    if (!itksys::SystemTools::FileExists(this->GetFileName()))
      // File doesn't exist
      return false;

    // Open file as read-only
    this->m_H5File = new H5::H5File(this->GetFileName(), H5F_ACC_RDONLY);

    // Check for the existence of the path
    if (!this->GetPathExists(this->GetPath()))
    {
      this->CloseH5File();
      return false;
    }

    H5::DataSet ds(this->GetDataSet());
    this->CloseH5File();
    return true;
  }
  catch (...)
  {
    // The dataset already exists
    this->CloseH5File();
    return false;
  }
}

} // end namespace itk
