# cmake/replace_in_file.cmake
# Portable text replacement in a file (no sed dependency).
# Usage: cmake -D TARGET_FILE=<path> -D OLD=<old> -D NEW=<new> -P replace_in_file.cmake

file(READ "${TARGET_FILE}" CONTENT)
string(REPLACE "${OLD}" "${NEW}" CONTENT "${CONTENT}")
file(WRITE "${TARGET_FILE}" "${CONTENT}")
