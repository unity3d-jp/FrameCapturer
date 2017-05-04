CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(usdi)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
FIND_PACKAGE(OpenEXR QUIET)
FIND_PACKAGE(OpenGL QUIET)
FIND_PACKAGE(GLEW QUIET)

SET(ISPC "/usr/local/bin/ispc" CACHE PATH "Path to Intel ISPC")
MARK_AS_ADVANCED(FORCE ISPC)

OPTION(FC_ENABLE_PNG "Enable Png sequence recorder." ON)
OPTION(FC_ENABLE_EXR "Enable Exr sequence recorder." ON)
OPTION(FC_ENABLE_GIF "Enable Gif recorder." ON)
OPTION(FC_ENABLE_WEBM "Enable WebM recorder." ON)
OPTION(FC_ENABLE_MP4 "Enable MP4 recorder." ON)
OPTION(FC_ENABLE_ISPC "Use Intel ISPC to generate SIMDified code. It can significantly boost performance." ON)

OPTION(FC_ENABLE_DEBUG "Enable debug functionalities." ON)
OPTION(FC_ENABLE_TESTS "Build tests." ON)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11")

IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    OPTION(FC_ENABLE_OSX_BUNDLE "Build bundle." OFF)
    SET(CMAKE_MACOSX_RPATH ON)

    IF(FC_ENABLE_OSX_BUNDLE)
        SET(CMAKE_SKIP_RPATH ON)
    ELSE()
        SET(CMAKE_SKIP_RPATH OFF)
    ENDIF()
ENDIF()

SET(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../FrameCapturer/Assets")
SET(PLUGINS_DIR "${ASSETS_DIR}/UTJ/FrameCapturer/Plugins/x86_64")


INCLUDE_DIRECTORIES(
    .
    ${INCLUDES}
    ${OPENEXR_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIR}
)


# GraphicsInterface
IF(USDI_ENABLE_GRAPHICS_INTERFACE)
    FILE(GLOB GI_CXX_FILES GraphicsInterface/*.cpp)
    FILE(GLOB GI_H_FILES GraphicsInterface/*.h)
    ADD_LIBRARY(GraphicsInterface STATIC ${GI_CXX_FILES} ${GI_H_FILES})
    TARGET_INCLUDE_DIRECTORIES(GraphicsInterface PUBLIC
        ./GraphicsInterface
        ./External/Unity/include
    )
ENDIF()


# MeshUtils
IF(FC_ENABLE_ISPC)
    if(NOT EXISTS ${ISPC})
        # try to download ISPC
        SET(ISPC_VERSION 1.9.1)
        IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            SET(ISPC_DIR ispc-v${ISPC_VERSION}-linux)
        ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            SET(ISPC_DIR ispc-v${ISPC_VERSION}-osx)
        ENDIF()
        SET(ISPC ${CMAKE_CURRENT_SOURCE_DIR}/External/${ISPC_DIR}/ispc CACHE PATH "" FORCE)

        if(NOT EXISTS ${ISPC})
            SET(ISPC_ARCHIVE ${ISPC_DIR}.tar.gz)
            FILE(DOWNLOAD http://downloads.sourceforge.net/project/ispcmirror/v${ISPC_VERSION}/${ISPC_ARCHIVE} ${CMAKE_CURRENT_BINARY_DIR}/${ISPC_ARCHIVE} SHOW_PROGRESS)
            EXECUTE_PROCESS(
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/External
                COMMAND tar -xzvf ${CMAKE_CURRENT_BINARY_DIR}/${ISPC_ARCHIVE}
            )
        ENDIF()
    ENDIF()

    SET(MUCORE_DIR ${CMAKE_CURRENT_BINARY_DIR}/MeshUtilsCore)
    ADD_CUSTOM_TARGET(MeshUtilsCore ALL
        # not use --opt=force-aligned-memory as I can't force USD to align memory to 0x20.
        # (Windows port is using with patched USD)
        COMMAND ${ISPC} ${CMAKE_CURRENT_SOURCE_DIR}/MeshUtils/MeshUtilsCore.ispc -o ${MUCORE_DIR}/MeshUtilsCore${CMAKE_CXX_OUTPUT_EXTENSION} -h ${MUCORE_DIR}/MeshUtilsCore.h --pic --target=sse2,sse4,avx --arch=x86-64 --opt=fast-masked-vload --opt=fast-math
    )
    SET(MUCORE_FILES
        ${MUCORE_DIR}/MeshUtilsCore.h
        ${MUCORE_DIR}/MeshUtilsCore${CMAKE_CXX_OUTPUT_EXTENSION}
        ${MUCORE_DIR}/MeshUtilsCore_sse2${CMAKE_CXX_OUTPUT_EXTENSION}
        ${MUCORE_DIR}/MeshUtilsCore_sse4${CMAKE_CXX_OUTPUT_EXTENSION}
        ${MUCORE_DIR}/MeshUtilsCore_avx${CMAKE_CXX_OUTPUT_EXTENSION}
    )

    # create dummy files to make cmake can find it
    FOREACH(F ${MUCORE_FILES})
        FILE(WRITE ${F} "")
    ENDFOREACH(F)
ENDIF()

FILE(GLOB MU_CXX_FILES MeshUtils/*.cpp MeshUtils/*.c)
FILE(GLOB MU_H_FILES MeshUtils/*.h)
ADD_LIBRARY(MeshUtils STATIC ${MU_CXX_FILES} ${MU_H_FILES} ${MUCORE_FILES})
TARGET_INCLUDE_DIRECTORIES(MeshUtils PUBLIC ./MeshUtils)
IF(FC_ENABLE_ISPC)
    ADD_DEFINITIONS(-DmuEnableISPC)
    ADD_DEPENDENCIES(MeshUtils MeshUtilsCore)
    TARGET_INCLUDE_DIRECTORIES(MeshUtils PUBLIC ${MUCORE_DIR})
ENDIF()



# fccore
FILE(GLOB FC_CXX_FILES ./*.cpp Foundation/*.cpp Encoder/*.cpp GraphicsDevice/*.cpp)
FILE(GLOB FC_H_FILES ./*.h Foundation/*.h Encoder/*.h GraphicsDevice/*.h)
IF(FC_ENABLE_OSX_BUNDLE)
    ADD_LIBRARY(fccore MODULE ${FC_CXX_FILES} ${FC_H_FILES})
ELSE()
    ADD_LIBRARY(fccore SHARED ${FC_CXX_FILES} ${FC_H_FILES})
ENDIF()
ADD_DEPENDENCIES(usdi MeshUtils)
TARGET_INCLUDE_DIRECTORIES(fccore PUBLIC .)
TARGET_LINK_LIBRARIES(fccore
    ${OPENEXR_Half_LIBRARY}
)

IF(FC_ENABLE_GIF)
    ADD_DEFINITIONS(-DfcSupportGIF)
ENDIF()
IF(FC_ENABLE_PNG)
    ADD_DEFINITIONS(-DfcSupportPNG)
    TARGET_LINK_LIBRARIES(fccore ${PNG_LIBRARY})
ENDIF()
IF(FC_ENABLE_EXR)
    ADD_DEFINITIONS(-DfcSupportEXR)
    TARGET_LINK_LIBRARIES(fccore ${OPENEXR_All_LIBRARY})
ENDIF()
IF(FC_ENABLE_WEBM)
    ADD_DEFINITIONS(-DfcSupportWebM)
    TARGET_LINK_LIBRARIES(fccore ${VPX_LIBRARY} ${OGGVORBIS_LIBRARY})
ENDIF()
IF(FC_ENABLE_MP4)
    ADD_DEFINITIONS(-DfcSupportMP4)
ENDIF()


IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    IF(FC_ENABLE_OSX_BUNDLE)
        SET_TARGET_PROPERTIES(fccore PROPERTIES BUNDLE ON)
    ELSE()
        SET_TARGET_PROPERTIES(fccore PROPERTIES PREFIX "")
        SET_TARGET_PROPERTIES(fccore PROPERTIES SUFFIX ".bundle")
    ENDIF()
ENDIF()


IF(NOT FC_ENABLE_OSX_BUNDLE)
    # tests
    IF(FC_ENABLE_TESTS)
        FILE(GLOB Tests_CXX_FILES Tests/*.cpp)
        FILE(GLOB Tests_H_FILES Tests/*.h)

        ADD_EXECUTABLE(fcTests ${Tests_CXX_FILES} ${Tests_H_FILES})
        ADD_DEPENDENCIES(fcTests fccore)
        TARGET_LINK_LIBRARIES(fcTests fccore)
    ENDIF()

    # deploy
    ADD_CUSTOM_TARGET(deploy ALL
        COMMAND rm -rf ${PLUGINS_DIR}/$<TARGET_FILE_NAME:fccore>
        COMMAND cp $<TARGET_FILE:fccore> ${PLUGINS_DIR}
        DEPENDS fccore
    )
ENDIF()
