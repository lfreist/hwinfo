message("input: ${INPUT}")
message("output: ${OUTPUT}")

if (VENDORS_ONLY)
    # vendor lines of pci.ids only ("vvvv  name"): the vendor names without the device database
    file(STRINGS "${INPUT}" LINES ENCODING UTF-8 REGEX "^[0-9a-f][0-9a-f][0-9a-f][0-9a-f]  ")
    list(JOIN LINES "\n" TEXT)
    set(FILTERED "${OUTPUT}.txt")
    file(WRITE "${FILTERED}" "${TEXT}\n")
    set(INPUT "${FILTERED}")
endif ()

file(READ "${INPUT}" CONTENT HEX)

string(LENGTH "${CONTENT}" HEX_LENGTH)
math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")

string(REGEX REPLACE "(..)" "0x\\1," CONTENT "${CONTENT}")
string(REGEX REPLACE ",$" "" CONTENT "${CONTENT}")

file(WRITE "${OUTPUT}" "const unsigned char ${NAME}[] = {${CONTENT}};\nconst unsigned int ${NAME}_size = ${BYTE_COUNT};\n")
