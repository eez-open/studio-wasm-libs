#include <emscripten.h>
#include <stdlib.h>

#include "lz4.h"
#include "lz4hc.h"
#include "lzma/LzmaEnc.h"

#define EM_PORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE

extern "C" EM_PORT_API(int) encodeBound(int inputSize) {
    return LZ4_compressBound(inputSize);
}

extern "C" EM_PORT_API(int) encodeBlockHC(const char* src, char* dst, int srcSize, int dstCapacity, int compressionLevel) {
    return LZ4_compress_HC(src, dst, srcSize, dstCapacity, compressionLevel);
}

extern "C" EM_PORT_API(int) encodeBlock(const char* src, char* dst, int srcSize, int dstCapacity) {
    return LZ4_compress_default(src, dst, srcSize, dstCapacity);
}

static void *lzmaAlloc(ISzAllocPtr, size_t size) {
    return malloc(size);
}

static void lzmaFree(ISzAllocPtr, void *address) {
    free(address);
}

static const ISzAlloc g_lzmaAlloc = { lzmaAlloc, lzmaFree };

extern "C" EM_PORT_API(int) encodeLzmaBound(int inputSize) {
    return LZMA_PROPS_SIZE + inputSize + inputSize / 3 + 128;
}

// Output: LZMA properties (LZMA_PROPS_SIZE bytes) followed by the LZMA stream
// without the end marker, the decoder must know the decompressed size.
// Returns the output size, or 0 on error.
extern "C" EM_PORT_API(int) encodeLzma(const unsigned char* src, unsigned char* dst, int srcSize, int dstCapacity) {
    if (srcSize < 0 || dstCapacity < LZMA_PROPS_SIZE) {
        return 0;
    }

    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    props.level = 9;
    props.dictSize = 1 << 12;
    while (props.dictSize < (UInt32)srcSize && props.dictSize < (1u << 26)) {
        props.dictSize <<= 1;
    }
    // assets are mostly 16 and 32 bit values aligned to 4 bytes
    props.lc = 0;
    props.lp = 2;
    props.pb = 2;
    props.fb = 273;
    props.writeEndMark = 0;
    props.numThreads = 1;

    SizeT propsSize = LZMA_PROPS_SIZE;
    SizeT destLen = dstCapacity - LZMA_PROPS_SIZE;
    SRes res = LzmaEncode(dst + LZMA_PROPS_SIZE, &destLen, src, srcSize, &props,
        dst, &propsSize, 0, nullptr, &g_lzmaAlloc, &g_lzmaAlloc);
    if (res != SZ_OK || propsSize != LZMA_PROPS_SIZE) {
        return 0;
    }

    return (int)(LZMA_PROPS_SIZE + destLen);
}
