set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${VORBIS_DIR}/lib
)

find_path(VORBIS_INCLUDE_DIR
    vorbis/codec.h
    PATHS ${VORBIS_DIR}/include
)

find_library(VORBIS_LIBRARY
    NAMES vorbis
    PATHS ${LIBRARY_PATHS}
)
find_library(VORBISENC_LIBRARY
    NAMES vorbisenc
    PATHS ${LIBRARY_PATHS}
)
find_library(VORBISFILE_LIBRARY
    NAMES vorbisfile
    PATHS ${LIBRARY_PATHS}
)

foreach(VORBIS_LIB
    vorbis
    vorbisenc
    vorbisfile
    )
    find_library(${VORBIS_LIB}_LIBRARY
        ${VORBIS_LIB}
        PATHS ${LIBRARY_PATHS}
    )
    mark_as_advanced(${VORBIS_LIB}_LIBRARY)
    if(${VORBIS_LIB}_LIBRARY)
        list(APPEND VORBIS_LIBRARIES ${${VORBIS_LIB}_LIBRARY})
    endif()
endforeach(VORBIS_LIB)

mark_as_advanced(VORBIS_INCLUDE_DIR)

find_package_handle_standard_args("VORBIS"
    DEFAULT_MSG
    VORBIS_LIBRARY
    VORBIS_INCLUDE_DIR
)
