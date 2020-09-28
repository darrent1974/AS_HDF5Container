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
#include "itkHDF5ContainerImageIOFactory.h"
#include "itkHDF5ContainerImageIO.h"
#include "itkVersion.h"

namespace itk
{
void
HDF5ContainerImageIOFactory::PrintSelf(std::ostream &, Indent) const
{}

HDF5ContainerImageIOFactory::HDF5ContainerImageIOFactory()
{
  this->RegisterOverride(
    "itkImageIOBase", "itkHDF5ImageIO", "HDF5 Container Image IO", true, CreateObjectFunction<HDF5ContainerImageIO>::New());
}

HDF5ContainerImageIOFactory::~HDF5ContainerImageIOFactory() = default;

const char *
HDF5ContainerImageIOFactory::GetITKSourceVersion() const
{
  return ITK_SOURCE_VERSION;
}

const char *
HDF5ContainerImageIOFactory::GetDescription() const
{
  return "HDF5 Container ImageIO Factory, allows the loading of HDF5 images into insight";
}

// Undocumented API used to register during static initialization.
// DO NOT CALL DIRECTLY.

static bool HDF5ContainerImageIOFactoryHasBeenRegistered;

void ITKIOHDF5Container_EXPORT
     HDF5ContainerImageIOFactoryRegister__Private()
{
  if (!HDF5ContainerImageIOFactoryHasBeenRegistered)
  {
    HDF5ContainerImageIOFactoryHasBeenRegistered = true;
    HDF5ContainerImageIOFactory::RegisterOneFactory();
  }
}

} // end namespace itk
