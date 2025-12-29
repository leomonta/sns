#/bin/bash


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

	printf "miniVector.h u_char\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_u_char.h -t u_char 'typedef unsigned char u_char;'
	printf "miniVector.c u_char\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_u_char.c -t u_char

	printf "miniVector.h StringOwn\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_StringOwn.h -t StringOwn '#include "StringRef.h"'
	printf "miniVector.c StringOwn\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_StringOwn.c -t StringOwn

	printf "miniMap.h u_char StringOwn\n"
	templetizer -i template/include/MiniMap.h -o include/MiniMap_u_char_StringOwn.h -t u_char StringOwn '#include "StringRef.h"'
	printf "miniMap.c u_char StringOwn\n"
	templetizer -i template/src/MiniMap.c -o src/MiniMap_u_char_StringOwn.c -t u_char StringOwn

	printf "RingBuffer.h StringOwn\n"
	templetizer -i template/include/RingBuffer.h -o include/RingBuffer_ResolverData.h -t ResolverData '#include "methods.h"'
	printf "RingBuffer.c StringOwn\n"
	templetizer -i template/src/RingBuffer.c -o src/RingBuffer_ResolverData.c -t ResolverData

	printf "MiniVector.h uint32_t\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_uint32_t.h -t uint32_t '#include <stdint.h>'
	printf "MiniVector.c uint32_t\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_uint32_t.c -t uint32_t

	printf "MiniVector.h MessageProcessor\n"
	templetizer -i template/include/MiniVector.h -o include/MiniVector_MessageProcessor.h -t MessageProcessor '#include "HttpMessage.h"'
	printf "MiniVector.c MessageProcessor\n"
	templetizer -i template/src/MiniVector.c -o src/MiniVector_MessageProcessor.c -t MessageProcessor

	printf "miniMap.h uint32_t MessageProcessor\n"
	templetizer -i template/include/MiniMap.h -o include/MiniMap_uint32_t_MessageProcessor.h -t uint32_t MessageProcessor ''
	printf "miniMap.c uint32_t MessageProcessor\n"
	templetizer -i template/src/MiniMap.c -o src/MiniMap_uint32_t_MessageProcessor.c -t uint32_t MessageProcessor

	printf "\n"

	sleep 5

done
