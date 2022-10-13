#!/usr/bin/env bash

# Run this script inside a dockcross container to build Python wheels for an ITK module.
#
# Versions can be restricted by passing them in as arguments to the script.
# For example,
#
#   /tmp/dockcross-manylinux-x64 manylinux-build-module-wheels.sh cp39
#
# Shared library dependencies can be included in the wheel by mounting them to /usr/lib64 or /usr/local/lib64 
# before running this script.
# 
# For example,
#
#   DOCKER_ARGS="-v /path/to/lib.so:/usr/local/lib64/lib.so"
#   /tmp/dockcross-manylinux-x64 -a "$DOCKER_ARGS" manylinux-build-module-wheels.sh
#

# -----------------------------------------------------------------------
# Script argument parsing
#
usage()
{
  echo "Usage:
  manylinux-build-module-wheels
    [ -h | --help ]           show usage
    [ -c | --cmake_options ]  space-separated string of CMake options to forward to the module (e.g. \"-DBUILD_TESTING=OFF\")
    [ -x | --exclude_libs ]   semicolon-separated library names to exclude when repairing wheel (e.g. \"libcuda.so\")
    [ -m | --modules ]        space-separated string of dependent ITK modules to pre-build (e.g. \"UnitConversion\")
    [ python_version ]        build wheel for a specific python version. (e.g. cp39)"
  exit 2
}

PARSED_ARGS=$(getopt -a -n dockcross-manylinux-download-cache-and-build-module-wheels \
  -o hc:x: --long help,cmake_options:,exclude_libs,modules: -- "$@")
eval set -- "$PARSED_ARGS"

while :
do
  case "$1" in
    -h | --help) usage; break ;;
    -c | --cmake_options) CMAKE_OPTIONS="$2" ; shift 2 ;;
    -x | --exclude_libs) EXCLUDE_LIBS="$2" ; shift 2 ;;
    -m | --modules) DEPENDENT_MODULES="$2" ; shift 2 ;;
    --) shift; break ;;
    *) echo "Unexpected option: $1.";
       usage; break ;;
  esac
done

PYTHON_VERSION="$@"
# -----------------------------------------------------------------------

# -----------------------------------------------------------------------
# These variables are set in common script:
#
ARCH=""
PYBINARIES=""

script_dir=$(cd $(dirname $0) || exit 1; pwd)
source "${script_dir}/manylinux-build-common.sh"
# -----------------------------------------------------------------------

echo "CMAKE_OPTIONS = ${CMAKE_OPTIONS}"
echo "DEPENDENT_MODULES = ${DEPENDENT_MODULES}"

# Assemble cmake ITK module and Ninja build target strings
# to pass to cmake in order to reconfigure the build tree
# for dependent ITK modules
ITK_DEPENDENT_MODULES=""
NINJA_TARGETS=""

if [[ ! -z ${DEPENDENT_MODULES} ]]; then
  for module in ${DEPENDENT_MODULES}; do
    echo "module = $module"
    ITK_DEPENDENT_MODULES="${ITK_DEPENDENT_MODULES} -DModule_$module:BOOL=ON"
    NINJA_TARGETS="${NINJA_TARGETS} Modules/Remote/$module/$module-all Wrapping/Modules/$module/all"
  done

  echo "ITK_DEPENDENT_MODULES = ${ITK_DEPENDENT_MODULES}"
  echo "NINJA_TARGETS = ${NINJA_TARGETS}"  
fi

# Set up library paths in container so that shared libraries can be added to wheels
sudo ldconfig
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/work/oneTBB-prefix/lib64:/usr/lib:/usr/lib64:/usr/local/lib:/usr/local/lib64

# Compile wheels re-using standalone project and archive cache
for PYBIN in "${PYBINARIES[@]}"; do
    Python3_EXECUTABLE=${PYBIN}/python
    Python3_INCLUDE_DIR=$( find -L ${PYBIN}/../include/ -name Python.h -exec dirname {} \; )

    echo ""
    echo "Python3_EXECUTABLE:${Python3_EXECUTABLE}"
    echo "Python3_INCLUDE_DIR:${Python3_INCLUDE_DIR}"

    if [[ -e /work/requirements-dev.txt ]]; then
      ${PYBIN}/pip install --upgrade -r /work/requirements-dev.txt
    fi
    version=$(basename $(dirname ${PYBIN}))
    # Remove "m" -- not present in Python 3.8 and later
    version=${version:0:9}
    itk_build_dir=/work/$(basename /ITKPythonPackage/ITK-${version}-manylinux2014_${ARCH})
    ln -fs /ITKPythonPackage/ITK-${version}-manylinux2014_${ARCH} $itk_build_dir
    if [[ ! -d ${itk_build_dir} ]]; then
      echo 'ITK build tree not available!' 1>&2
      exit 1
    fi        
    itk_source_dir=/work/ITK-source/ITK
    ln -fs /ITKPythonPackage/ITK-source/ /work/ITK-source
    if [[ ! -d ${itk_source_dir} ]]; then
      echo 'ITK source tree not available!' 1>&2
      exit 1
    fi

    tbb_dir=/work/oneTBB-prefix/lib/cmake/TBB
    echo "tbb_dir = ${tbb_dir}"

    # If any dependent modules are specified the a parital
    # rebuild of the ITK build tree is required
    if [[ ! -z ${DEPENDENT_MODULES} ]]; then
      pushd ${itk_build_dir}  > /dev/null 2>&1
      echo "re-running cmake"

      cmake \
        ${CMAKE_OPTIONS} \
        ${ITK_DEPENDENT_MODULES} \
        ${itk_source_dir}
 
      echo 're-running ninja'
      ninja ${NINJA_TARGETS}
      popd > /dev/null 2>&1 
    fi
 
    # Build wheel
    ${PYBIN}/python setup.py bdist_wheel --build-type Release -G Ninja -- \
      -DITK_DIR:PATH=${itk_build_dir} \
      -DITK_USE_SYSTEM_SWIG:BOOL=ON \
      -DWRAP_ITK_INSTALL_COMPONENT_IDENTIFIER:STRING=PythonWheel \
      -DSWIG_EXECUTABLE:FILEPATH=${itk_build_dir}/Wrapping/Generators/SwigInterface/swig/bin/swig \
      -DCMAKE_CXX_COMPILER_TARGET:STRING=$(uname -p)-linux-gnu \
      -DBUILD_TESTING:BOOL=OFF \
      -DPython3_EXECUTABLE:FILEPATH=${Python3_EXECUTABLE} \
      -DPython3_INCLUDE_DIR:PATH=${Python3_INCLUDE_DIR} \
    || exit 1
    ${PYBIN}/python setup.py clean

done

if test "${ARCH}" == "x64"; then
  # Make sure auditwheel is installed for this python exe before importing
  # it in auditwheel_whitelist_monkeypatch.py
  sudo ${Python3_EXECUTABLE} -m pip install auditwheel
  for whl in dist/*linux_$(uname -p).whl; do
    # Repair wheel using monkey patch to exclude shared libraries provided in whitelist
    ${Python3_EXECUTABLE} "${script_dir}/auditwheel_whitelist_monkeypatch.py" \
      repair ${whl} -w /work/dist/ --whitelist "${EXCLUDE_LIBS}"
    rm ${whl}
  done
fi
if compgen -G "dist/itk*-linux*.whl" > /dev/null; then
  for itk_wheel in dist/itk*-linux*.whl; do
    mv ${itk_wheel} ${itk_wheel/linux/manylinux2014}
  done
fi
