#!/bin/bash

printf "miniVector.h stringRef\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_StringRef.h -t StringRef '#include "StringRef.h"'
printf "miniVector.c stringRef\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_StringRef.c -t StringRef

printf "miniVector.h stringView\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_StringView.h -t StringView '#include "StringRef.h"'
printf "miniVector.c stringView\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_StringView.c -t StringView

printf "miniMap.h stringRef\n"
templetizer -i template/include/MiniMap.h -o include/generated/MiniMap_StringRef_StringRef.h -t StringRef StringRef '#include "StringRef.h"'
printf "miniMap.c stringRef\n"
templetizer -i template/src/MiniMap.c -o src/generated/MiniMap_StringRef_StringRef.c -t StringRef StringRef

printf "miniVector.h StringOwn\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_StringOwn.h -t StringOwn '#include "StringRef.h"'
printf "miniVector.c StringOwn\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_StringOwn.c -t StringOwn

printf "miniVector.h HTTPResponseOption\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_HTTPResponseOption.h -t HTTPResponseOption '#include "constants.h"'
printf "miniVector.c HTTPResponseOption\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_HTTPResponseOption.c -t HTTPResponseOption

printf "miniMap.h HTTPResponseOption StringOwn\n"
templetizer -i template/include/MiniMap.h -o include/generated/MiniMap_HTTPResponseOption_StringOwn.h -t HTTPResponseOption StringOwn '#include "constants.h"'
printf "miniMap.c HTTPResponseOption StringOwn\n"
templetizer -i template/src/MiniMap.c -o src/generated/MiniMap_HTTPResponseOption_StringOwn.c -t HTTPResponseOption StringOwn

printf "RingBuffer.h ResolverData\n"
templetizer -i template/include/RingBuffer.h -o include/generated/RingBuffer_ResolverData.h -t ResolverData '#include "ResolverData.h"'
printf "RingBuffer.c ResolverData\n"
templetizer -i template/src/RingBuffer.c -o src/generated/RingBuffer_ResolverData.c -t ResolverData

printf "MiniVector.h MessageProcessor\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_MessageProcessor.h -t MessageProcessor '#include "HttpMessage.h"'
printf "MiniVector.c MessageProcessor\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_MessageProcessor.c -t MessageProcessor

printf "miniMap.h StringRef MessageProcessor\n"
templetizer -i template/include/MiniMap.h -o include/generated/MiniMap_StringRef_MessageProcessor.h -t StringRef MessageProcessor ''
printf "miniMap.c StringRef MessageProcessor\n"
templetizer -i template/src/MiniMap.c -o src/generated/MiniMap_StringRef_MessageProcessor.c -t StringRef MessageProcessor

printf "miniVector.h regex_t\n"
templetizer -i template/include/MiniVector.h -o include/generated/MiniVector_regex_t.h -t regex_t '#include "regex.h"'
printf "miniVector.c regex_t\n"
templetizer -i template/src/MiniVector.c -o src/generated/MiniVector_regex_t.c -t regex_t

printf "miniMap.h regex_t MessageProcessor\n"
templetizer -i template/include/MiniMap.h -o include/generated/MiniMap_regex_t_MessageProcessor.h -t regex_t MessageProcessor ''
printf "miniMap.c regex_t MessageProcessor\n"
templetizer -i template/src/MiniMap.c -o src/generated/MiniMap_regex_t_MessageProcessor.c -t regex_t MessageProcessor
