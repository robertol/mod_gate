#ifndef MODJERK_SWAP_ENDIAN
#define MODJERK_SWAP_ENDIAN

// from http://www.gdargaud.net/Hack/SourceCode.html

namespace modjerk {

namespace endian {
extern const int one;
}

// Swap the byte order of a structure
extern void* swap_endian(void* addr, const int size);

} // end namespace modjerk

#define IS_BIGENDIAN    (*(char *)(&modjerk::endian::one)==0)
#define IS_LITTLEENDIAN (*(char *)(&modjerk::endian::one)==1)

#endif
