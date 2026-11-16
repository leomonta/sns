#!/bin/bash


while [ 1 ]
do

	printf "miniVector.h stringRef\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_StringRef.h -t StringRef '#include "StringRef.h"'
	printf "miniVector.c stringRef\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_StringRef.c -t StringRef

	printf "miniMap.h stringRef\n"
	templetizer -i template/include/MiniMap.h -o include/MiniMap_StringRef_StringRef.h -t StringRef StringRef '#include "StringRef.h"'
	printf "miniMap.c stringRef\n"
	templetizer -i template/src/MiniMap.c -o src/MiniMap_StringRef_StringRef.c -t StringRef StringRef

	printf "miniVector.h StringOwn\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_StringOwn.h -t StringOwn '#include "StringRef.h"'
	printf "miniVector.c StringOwn\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_StringOwn.c -t StringOwn

	printf "miniVector.h HTTPResponseOption\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_HTTPResponseOption.h -t HTTPResponseOption '#include "constants.h"'
	printf "miniVector.c HTTPResponseOption\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_HTTPResponseOption.c -t HTTPResponseOption

	printf "miniMap.h HTTPResponseOption StringOwn\n"
	templetizer -i template/include/MiniMap.h -o include/MiniMap_HTTPResponseOption_StringOwn.h -t HTTPResponseOption StringOwn '#include "constants.h"'
	printf "miniMap.c HTTPResponseOption StringOwn\n"
	templetizer -i template/src/MiniMap.c -o src/MiniMap_HTTPResponseOption_StringOwn.c -t HTTPResponseOption StringOwn

	printf "RingBuffer.h StringOwn\n"
	templetizer -i template/include/RingBuffer.h -o include/RingBuffer_ResolverData.h -t ResolverData '#include "ResolverData.h"'
	printf "RingBuffer.c StringOwn\n"
	templetizer -i template/src/RingBuffer.c -o src/RingBuffer_ResolverData.c -t ResolverData

	printf "MiniVector.h MessageProcessor\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_MessageProcessor.h -t MessageProcessor '#include "HttpMessage.h"'
	printf "MiniVector.c MessageProcessor\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_MessageProcessor.c -t MessageProcessor

	printf "miniMap.h StringRef MessageProcessor\n"
	templetizer -i template/include/MiniMap.h -o include/MiniMap_StringRef_MessageProcessor.h -t StringRef MessageProcessor ''
	printf "miniMap.c StringRef MessageProcessor\n"
	templetizer -i template/src/MiniMap.c -o src/MiniMap_StringRef_MessageProcessor.c -t StringRef MessageProcessor

	printf "\n"

	sleep 5

done
