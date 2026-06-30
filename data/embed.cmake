message("input: ${INPUT}")
message("output: ${OUTPUT}")

file(READ "${INPUT}" CONTENT HEX)

string(LENGTH "${CONTENT}" HEX_LENGTH)
math(EXPR BYTE_COUNT "${HEX_LENGTH} / 2")

string(REGEX REPLACE "(..)" "0x\\1," CONTENT "${CONTENT}")
string(REGEX REPLACE ",$" "" CONTENT "${CONTENT}")

file(WRITE "${OUTPUT}" "const unsigned char ${NAME}[] = {${CONTENT}};\nconst unsigned int ${NAME}_size = ${BYTE_COUNT};\n")