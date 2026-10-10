# Writes the number the app shows, as the moment it was made. Run by `make build-number` when a
# release is being cut - never by a build, so that every package cut from one stamp carries it.
# in Polish time (CET/CEST) wherever it is cut - the CI's runners are on UTC
SET(ENV{TZ} "Europe/Warsaw")
STRING(TIMESTAMP BUILD_NUMBER "%Y%m%d-%H%M")
FILE(WRITE "${OUT}" "${BUILD_NUMBER}")
MESSAGE(STATUS "Build number: ${BUILD_NUMBER}")
