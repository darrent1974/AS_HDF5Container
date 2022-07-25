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
#ifndef itkHDF5ContainerImageIOFactory_h
#define itkHDF5ContainerImageIOFactory_h
#include "HDF5ContainerExport.h"

#include "itkObjectFactoryBase.h"
#include "itkImageIOBase.h"

namespace itk
{
  /**
 *\class HDF5ContainerImageIOFactory
 * \author Darren Thompson
 * \brief Create instances of HDF5ContainerImageIO objects using an object
 * factory.
 * \ingroup CSIROCT
 */
  class HDF5Container_EXPORT HDF5ContainerImageIOFactory : public ObjectFactoryBase
  {
  public:
    ITK_DISALLOW_COPY_AND_ASSIGN(HDF5ContainerImageIOFactory);

    /** Standard class type aliases. */
    using Self = HDF5ContainerImageIOFactory;
    using Superclass = ObjectFactoryBase;
    using Pointer = SmartPointer<Self>;
    using ConstPointer = SmartPointer<const Self>;

    /** Class methods used to interface with the registered factories. */
    const char *
    GetITKSourceVersion() const override;

    const char *
    GetDescription() const override;

    /** Method for class instantiation. */
    itkFactorylessNewMacro(Self);

    /** Run-time type information (and related methods). */
    itkTypeMacro(HDF5ContainerImageIOFactory, ObjectFactoryBase);

    /** Register one factory of this type  */
    static void
    RegisterOneFactory()
    {
      HDF5ContainerImageIOFactory::Pointer metaFactory = HDF5ContainerImageIOFactory::New();

      ObjectFactoryBase::RegisterFactoryInternal(metaFactory);
    }

  protected:
    HDF5ContainerImageIOFactory();
    ~HDF5ContainerImageIOFactory() override;
    void
    PrintSelf(std::ostream &os, Indent indent) const override;
  };
} // end namespace itk

#endif
