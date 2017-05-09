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

mark_as_advanced(VORBIS_INCLUDE_DIR)
mark_as_advanced(VORBIS_LIBRARY)

find_package_handle_standard_args("VORBIS"
    DEFAULT_MSG
    VORBIS_LIBRARY
    VORBIS_INCLUDE_DIR
)
