import os
import sys
import glob
import shutil
import excons
from excons.tools import tbb
from excons.tools import unity
from excons.tools import dl
from excons.tools import gl
from excons.tools import glew
from excons.tools import ilmbase
from excons.tools import zlib
from excons.tools import openexr

use_externals = (sys.platform == "win32" and excons.Build64() and excons.GetArgument("use-externals", 1, int) != 0)
if use_externals:
  # Provided externals are built using runtime version 12.0
  # 'mscver' has to be set before excons.MakeBaseEnv is called for the toolchain to be properly setup by SCons
  excons.SetArgument("mscver", "12.0")

env = excons.MakeBaseEnv()

# I don't know whst this whole PatchLibrary is. Looks like a hack that we don't
# really need. Let's disable it for now by defining fcMaster
defines = ["fcMaster"]
inc_dirs = []
lib_dirs = []
libs = []
customs = []
install_files = {"unity/FrameCapturer/Scripts": glob.glob("FrameCapturer/Assets/FrameCapturer/Scripts/*.cs"),
                 "unity/FrameCapturer/Prefabs": glob.glob("FrameCapturer/Assets/FrameCapturer/Prefabs/*.prefab"),
                 "unity/FrameCapturer/Shaders": glob.glob("FrameCapturer/Assets/FrameCapturer/Shaders/*.shader")}
sources = filter(lambda x: os.path.basename(x) not in ["pch.cpp", "AddLibraryPath.cpp"], glob.glob("Plugin/*.cpp"))

if excons.GetArgument("debug", 0, int):
  defines.append("fcDebug")

if excons.GetArgument("exr", 1, int) != 0:
  defines.append("fcSupportEXR")

if excons.GetArgument("gif", 0, int) != 0:
  defines.append("fcSupportGIF")

if excons.GetArgument("gl", 1, int) != 0:
  defines.append("fcSupportOpenGL")

if use_externals:
  if excons.GetArgument("d3d9", 1, int) != 0:
    defines.append("fcSupportD3D9")
  
  if excons.GetArgument("d3d11", 0, int) != 0:
    defines.append("fcSupportD3D11")

  inc_dirs.extend(["Plugin/external/ilmbase-2.2.0/Half",
                   "Plugin/external/ilmbase-2.2.0/Iex",
                   "Plugin/external/ilmbase-2.2.0/IexMath",
                   "Plugin/external/ilmbase-2.2.0/Imath",
                   "Plugin/external/ilmbase-2.2.0/IlmThread",
                   "Plugin/external/ilmbase-2.2.0/config",
                   "Plugin/external/openexr-2.2.0/IlmImf",
                   "Plugin/external/openexr-2.2.0/config",
                   "Plugin/external/glew-1.12.0/include",
                   "Plugin/external/zlib-1.2.8",
                   "Plugin/external"])
  
  lib_dirs.append("Plugin/external/libs/x86_64")
  
  embed_libs = []

else:
  defines.append("fcNoAutoLink")
  defines.append("fcDontForceStaticGLEW")
  
  if sys.platform == "win32":
    if excons.GetArgument("d3d9", 1, int) != 0:
      defines.append("fcSupportD3D9")

    if excons.GetArgument("d3d11", 0, int) != 0:
      defines.append("fcSupportD3D11")
  
  if "fcSupportEXR" in defines:
    customs.append(openexr.Require(ilmbase=True, zlib=True))

  if "fcSupportOpenGL" in defines:
    customs.extend([glew.Require, gl.Require])

  tbb_incdir, tbb_libdir = excons.GetDirs("tbb")
  if tbb_incdir or tbb_libdir:
    defines.append("fcWithTBB")
    customs.append(tbb.Require)

  embed_libs = excons.GetArgument("embed-libs", [])
  if embed_libs:
    if os.path.isdir(embed_libs):
      pat = "/*." + ("dylib" if sys.platform == "darwin" else ("dll" if sys.platform == "win32" else "so"))
      embed_libs = glob.glob(embed_libs + pat)
    else:
      embed_libs = [embed_libs]

capturer = {"name": "FrameCapturer",
            "type": "dynamicmodule",
            "defs": defines,
            "incdirs": inc_dirs,
            "libdirs": lib_dirs,
            "libs": libs,
            "custom": customs,
            "srcs": sources,
            "install": install_files}

unity.Plugin(capturer, libs=embed_libs)

if sys.platform == "win32":
  path_hack = {"name": "AddLibraryPath",
               "type": "dynamicmodule",
               "custom": [dl.Require],
               "srcs": ["Plugin/AddLibraryPath.cpp"]}

  unity.Plugin(path_hack, package="FrameCapturer")

  # Add 'AddLibraryPath' as a dependency for 'AlembicImporter'
  capturer["deps"] = ["AddLibraryPath"]

  targets = [path_hack, capturer]

else:
  targets = [capturer]

excons.DeclareTargets(env, targets)

Default(["FrameCapturer"])
