#!/usr/bin/env bash

printf "Checking sources for code style\n"
SOURCE_FILES=()
find ./src/ ./test/ ./include/ -regextype egrep -regex '.*\.(h|c)(pp|xx)?$' -print0 > sourcelist
while IFS=  read -r -d $'\0'; do
    SOURCE_FILES+=("$REPLY")
done < sourcelist

ERROR=0
for source in "${SOURCE_FILES[@]}"; do
	clang-format -output-replacements-xml "$source" | grep "<replacement " &> /dev/null
	HAS_WRONG_FILES=$?
	if [ $HAS_WRONG_FILES -ne 1 ]; then
		printf "Checking %s: \x1b[31mFAILED!\x1b[m\n" "$source"
		ERROR=1
  else
		printf "Checking %s: \x1b[32mPASSED\x1b[m\n" "$source"
	fi
done

rm sourcelist

if [ $ERROR -eq 0 ]; then
	printf "\x1b[32mCongratulations! All sources match the code style\x1b[m\n"
else
  printf "\x1b[31mclang-format discovered style issues in at least one file.\x1b[m\n"
	exit 1
fi