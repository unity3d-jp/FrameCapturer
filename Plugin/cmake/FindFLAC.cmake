set(LIBRARY_PATHS
    /usr/lib
    /usr/local/lib
    ${FLAC_DIR}/lib
)

find_path(FLAC_INCLUDE_DIR
    FLAC/all.h
    PATHS ${FLAC_DIR}/include
)

find_library(FLAC_LIBRARY
    NAMES FLAC
    PATHS ${LIBRARY_PATHS}
)

mark_as_advanced(FLAC_INCLUDE_DIR)
mark_as_advanced(FLAC_LIBRARY)

find_package_handle_standard_args("FLAC"
    DEFAULT_MSG
    FLAC_LIBRARY
    FLAC_INCLUDE_DIR
)
