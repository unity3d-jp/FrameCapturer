if(ENABLE_ISPC)
    setup_ispc()
    set(FCISPC_OUTDIR ${CMAKE_CURRENT_BINARY_DIR}/ISPC)
    set(FCISPC_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Foundation/ConvertKernel.ispc"
    )
    add_ispc_targets(SOURCES ${FCISPC_SOURCES} OUTDIR ${FCISPC_OUTDIR})
    set(FCISPC_OUTPUTS ${_ispc_outputs})
endif()

file(GLOB FC_SOURCES
    *.cpp
    Foundation/*.cpp
    Encoder/Audio/*.cpp
    Encoder/Image/*.cpp
    Encoder/MP4/*.cpp
    Encoder/WebM/*.cpp
    GraphicsDevice/*.cpp
    *.h
    Foundation/*.h
    Encoder/Audio/*.h
    Encoder/Image/*.h
    Encoder/MP4/*.h
    Encoder/WebM/*.h
    GraphicsDevice/*.h
)
set(PLUGINS_DIR "${CMAKE_SOURCE_DIR}/../FrameCapturer/Assets/UTJ/FrameCapturer/Plugins/x86_64")
add_plugin(fccore SOURCES ${FC_SOURCES} ${FCISPC_OUTPUTS} PLUGINS_DIR ${PLUGINS_DIR})

target_include_directories(
    fccore
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/external
    ${OPENEXR_INCLUDE_DIRS}
    ${YUV_INCLUDE_DIR}
    ${WEBM_INCLUDE_DIR}
    ${OGG_INCLUDE_DIR}
    ${VORBIS_INCLUDE_DIR}
    ${OPUS_INCLUDE_DIR}
    ${FLAC_INCLUDE_DIR}
)
install(TARGETS fccore DESTINATION .)

target_link_libraries(fccore
    pthread z ${OPENEXR_Half_LIBRARY} ${YUV_LIBRARY}
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(fccore dl)
endif()

if(ENABLE_ISPC)
    add_definitions(-DfcEnableISPC)
    target_include_directories(fccore PRIVATE ${FCISPC_OUTDIR})
endif()

if(FC_ENABLE_GIF)
    add_definitions(-DfcSupportGIF)
endif()

if(FC_ENABLE_PNG)
    add_definitions(-DfcSupportPNG)
    target_link_libraries(fccore ${PNG_LIBRARY})
endif()

if(FC_ENABLE_EXR)
    add_definitions(-DfcSupportEXR)
    target_link_libraries(fccore ${OPENEXR_LIBRARIES})
endif()

if(FC_ENABLE_WEBM)
    add_definitions(-DfcSupportWebM)
    target_link_libraries(fccore ${WEBM_LIBRARY})
    if(FC_ENABLE_VPX)
        add_definitions(-DfcSupportVPX)
        target_link_libraries(fccore ${VPX_LIBRARY})
    endif()
    if(FC_ENABLE_OPUS)
        add_definitions(-DfcSupportOpus)
        target_link_libraries(fccore ${OPUS_LIBRARY})
    endif()
endif()

if(FC_ENABLE_MP4)
    add_definitions(-DfcSupportMP4)
    if(FC_ENABLE_FAAC)
        add_definitions(-DfcSupportAAC_FAAC)
    endif()
    if(FC_ENABLE_OPENH264)
        add_definitions(-DfcSupportH264_OpenH264)
    endif()
endif()
if(FC_ENABLE_WAVE)
    add_definitions(-DfcSupportWave)
endif()

if(FC_ENABLE_VORBIS)
    add_definitions(-DfcSupportVorbis)
    target_link_libraries(fccore ${OGG_LIBRARY} ${VORBIS_LIBRARIES})
    if(FC_ENABLE_FLAC)
        add_definitions(-DfcSupportFlac)
        target_link_libraries(fccore ${FLAC_LIBRARY})
    endif()
endif()
