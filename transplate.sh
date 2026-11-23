#!/bin/bash

rm -r include/generated/*
rm -r src/generated/*

printf "MiniVector.h stringRef\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_StringRef.h -t StringRef '#include "StringRef.h"'
printf "MiniVector.c stringRef\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_StringRef.c -t StringRef

printf "MiniVector.h stringView\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_StringView.h -t StringView '#include "StringRef.h"'
printf "MiniVector.c stringView\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_StringView.c -t StringView

printf "MiniMap.h stringRef\n"
templetizer -i template/include/MiniMap.h -o include/generated/MiniMap_StringRef_StringRef.h -t StringRef StringRef '#include "StringRef.h"'
printf "MiniMap.c stringRef\n"
templetizer -i template/src/MiniMap.c -o src/generated/MiniMap_StringRef_StringRef.c -t StringRef StringRef

printf "MiniVector.h StringOwn\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_StringOwn.h -t StringOwn '#include "StringRef.h"'
printf "MiniVector.c StringOwn\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_StringOwn.c -t StringOwn

printf "MiniVector.h HTTPResponseOption\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_HTTPResponseOption.h -t HTTPResponseOption '#include "constants.h"'
printf "MiniVector.c HTTPResponseOption\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_HTTPResponseOption.c -t HTTPResponseOption

printf "MiniMap.h HTTPResponseOption StringOwn\n"
templetizer -i template/include/MiniMap.h -o include/generated/MiniMap_HTTPResponseOption_StringOwn.h -t HTTPResponseOption StringOwn '#include "constants.h"'
printf "MiniMap.c HTTPResponseOption StringOwn\n"
templetizer -i template/src/MiniMap.c -o src/generated/MiniMap_HTTPResponseOption_StringOwn.c -t HTTPResponseOption StringOwn

printf "RingBuffer.h ResolverData\n"
templetizer -i template/include/RingBuffer.h -o include/generated/RingBuffer_ResolverData.h -t ResolverData '#include "ResolverData.h"'
printf "RingBuffer.c ResolverData\n"
templetizer -i template/src/RingBuffer.c -o src/generated/RingBuffer_ResolverData.c -t ResolverData

printf "MiniVector.h MessageProcessor\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_MessageProcessor.h -t MessageProcessor '#include "HttpMessage.h"'
printf "MiniVector.c MessageProcessor\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_MessageProcessor.c -t MessageProcessor

printf "MiniVector.h regex_t\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_regex_t.h -t regex_t '#include "regex.h"'
printf "MiniVector.c regex_t\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_regex_t.c -t regex_t
