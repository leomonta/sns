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

	printf "\n"

	sleep 5

done
