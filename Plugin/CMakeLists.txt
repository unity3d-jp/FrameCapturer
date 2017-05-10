CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(fccore)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
FIND_PACKAGE(OpenEXR QUIET)
FIND_PACKAGE(ZLIB QUIET)
FIND_PACKAGE(PNG QUIET)
FIND_PACKAGE(YUV QUIET)
FIND_PACKAGE(WEBM QUIET)
FIND_PACKAGE(VPX QUIET)
FIND_PACKAGE(OGG QUIET)
FIND_PACKAGE(VORBIS QUIET)
FIND_PACKAGE(OPUS QUIET)

SET(ISPC "/usr/local/bin/ispc" CACHE PATH "Path to Intel ISPC")
MARK_AS_ADVANCED(FORCE ISPC)

OPTION(FC_ENABLE_ISPC "Use Intel ISPC to generate SIMDified code. It can significantly boost performance." ON)
OPTION(FC_ENABLE_PNG "Enable Png sequence exporter." ON)
OPTION(FC_ENABLE_EXR "Enable Exr sequence exporter." ON)
OPTION(FC_ENABLE_GIF "Enable Gif exporter." ON)
OPTION(FC_ENABLE_WEBM "Enable WebM exporter." ON)
IF(FC_ENABLE_WEBM)
    OPTION(FC_ENABLE_VPX "Enable VPX encoder." ON)
    OPTION(FC_ENABLE_VORBIS "Enable vorbis encoder." ON)
    OPTION(FC_ENABLE_OPUS "Enable opus encoder." OFF)
ENDIF()
OPTION(FC_ENABLE_MP4 "Enable MP4 exporter." OFF)
IF(FC_ENABLE_MP4)
    OPTION(FC_ENABLE_OPENH264 "Enable OpenH264 encoder." OFF)
    OPTION(FC_ENABLE_FAAC "Enable FAAC encoder." OFF)
ENDIF()

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
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fccore
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${OPENEXR_INCLUDE_DIRS}
    ${YUV_INCLUDE_DIR}
    ${WEBM_INCLUDE_DIR}
    ${OGG_INCLUDE_DIR}
    ${VORBIS_INCLUDE_DIR}
    ${OPUS_INCLUDE_DIR}
)


# fccoreISPC
IF(FC_ENABLE_ISPC)
    if(NOT EXISTS ${ISPC})
        # try to download ISPC
        SET(ISPC_VERSION 1.9.1)
        IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            SET(ISPC_DIR ispc-v${ISPC_VERSION}-linux)
        ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
            SET(ISPC_DIR ispc-v${ISPC_VERSION}-osx)
        ENDIF()
        SET(ISPC ${CMAKE_CURRENT_SOURCE_DIR}/external/${ISPC_DIR}/ispc CACHE PATH "" FORCE)

        if(NOT EXISTS ${ISPC})
            SET(ISPC_ARCHIVE ${ISPC_DIR}.tar.gz)
            FILE(DOWNLOAD http://downloads.sourceforge.net/project/ispcmirror/v${ISPC_VERSION}/${ISPC_ARCHIVE} ${CMAKE_CURRENT_BINARY_DIR}/${ISPC_ARCHIVE} SHOW_PROGRESS)
            EXECUTE_PROCESS(
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/External
                COMMAND tar -xzvf ${CMAKE_CURRENT_BINARY_DIR}/${ISPC_ARCHIVE}
            )
        ENDIF()
    ENDIF()

    SET(FCISPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/fccoreISPC)
    ADD_CUSTOM_TARGET(fccoreISPC ALL
        COMMAND ${ISPC} ${CMAKE_CURRENT_SOURCE_DIR}/fccore/Foundation/ConvertKernel.ispc -o ${FCISPC_DIR}/ConvertKernel${CMAKE_CXX_OUTPUT_EXTENSION} -h ${FCISPC_DIR}/ConvertKernel_ispc.h --pic --target=sse2,sse4,avx --arch=x86-64 --opt=fast-masked-vload --opt=fast-math
    )
    SET(FCISPC_FILES
        ${FCISPC_DIR}/ConvertKernel_ispc.h
        ${FCISPC_DIR}/ConvertKernel${CMAKE_CXX_OUTPUT_EXTENSION}
        ${FCISPC_DIR}/ConvertKernel_sse2${CMAKE_CXX_OUTPUT_EXTENSION}
        ${FCISPC_DIR}/ConvertKernel_sse4${CMAKE_CXX_OUTPUT_EXTENSION}
        ${FCISPC_DIR}/ConvertKernel_avx${CMAKE_CXX_OUTPUT_EXTENSION}
    )

    # create dummy files to make cmake can find it
    FOREACH(F ${FCISPC_FILES})
        FILE(WRITE ${F} "")
    ENDFOREACH(F)
ENDIF()


# fccore
FILE(GLOB FC_CXX_FILES fccore/*.cpp fccore/Foundation/*.cpp fccore/Encoder/*.cpp fccore/GraphicsDevice/*.cpp)
FILE(GLOB FC_H_FILES fccore/*.h fccore/Foundation/*.h fccore/Encoder/*.h fccore/GraphicsDevice/*.h)
IF(FC_ENABLE_OSX_BUNDLE)
    ADD_LIBRARY(fccore MODULE ${FC_CXX_FILES} ${FC_H_FILES} ${FCISPC_FILES})
ELSE()
    ADD_LIBRARY(fccore SHARED ${FC_CXX_FILES} ${FC_H_FILES} ${FCISPC_FILES})
ENDIF()
TARGET_INCLUDE_DIRECTORIES(fccore PUBLIC .)
TARGET_LINK_LIBRARIES(fccore
    pthread ${OPENEXR_Half_LIBRARY} ${YUV_LIBRARY}
)
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    TARGET_LINK_LIBRARIES(fccore dl)
ENDIF()

IF(FC_ENABLE_ISPC)
    ADD_DEFINITIONS(-DfcEnableISPC)
    ADD_DEPENDENCIES(fccore fccoreISPC)
    TARGET_INCLUDE_DIRECTORIES(fccore PUBLIC ${FCISPC_DIR})
ENDIF()

IF(FC_ENABLE_GIF)
    ADD_DEFINITIONS(-DfcSupportGIF)
ENDIF()
IF(FC_ENABLE_PNG)
    ADD_DEFINITIONS(-DfcSupportPNG)
    TARGET_LINK_LIBRARIES(fccore ${PNG_LIBRARY})
ENDIF()
IF(FC_ENABLE_EXR)
    ADD_DEFINITIONS(-DfcSupportEXR)
    TARGET_LINK_LIBRARIES(fccore ${OPENEXR_LIBRARIES})
ENDIF()
IF(FC_ENABLE_WEBM)
    ADD_DEFINITIONS(-DfcSupportWebM)
    TARGET_LINK_LIBRARIES(fccore ${WEBM_LIBRARY})
    IF(FC_ENABLE_VPX)
        ADD_DEFINITIONS(-DfcSupportVPX)
        TARGET_LINK_LIBRARIES(fccore ${VPX_LIBRARY})
    ENDIF()
    IF(FC_ENABLE_VORBIS)
        ADD_DEFINITIONS(-DfcSupportVorbis)
        TARGET_LINK_LIBRARIES(fccore ${OGG_LIBRARY} ${VORBIS_LIBRARIES})
    ENDIF()
    IF(FC_ENABLE_OPUS)
        ADD_DEFINITIONS(-DfcSupportOpus)
        TARGET_LINK_LIBRARIES(fccore ${OPUS_LIBRARY})
    ENDIF()
ENDIF()
IF(FC_ENABLE_MP4)
    ADD_DEFINITIONS(-DfcSupportMP4)
    IF(FC_ENABLE_FAAC)
        ADD_DEFINITIONS(-DfcSupportAAC_FAAC)
    ENDIF()
    IF(FC_ENABLE_OPENH264)
        ADD_DEFINITIONS(-DfcSupportH264_OpenH264)
    ENDIF()
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
        TARGET_LINK_LIBRARIES(fcTests fccore pthread)
    ENDIF()

    # deploy
    ADD_CUSTOM_TARGET(deploy ALL
        COMMAND rm -rf ${PLUGINS_DIR}/$<TARGET_FILE_NAME:fccore>
        COMMAND cp $<TARGET_FILE:fccore> ${PLUGINS_DIR}
        DEPENDS fccore
    )
ENDIF()
