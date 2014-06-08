#ifndef MODGATE_SWAP_ENDIAN
#define MODGATE_SWAP_ENDIAN

// from http://www.gdargaud.net/Hack/SourceCode.html

namespace modgate {

namespace endian {
extern const int one;
}

// Swap the byte order of a structure
extern void* swap_endian(void* addr, const int size);

} // end namespace modgate

#define IS_BIGENDIAN    (*(char *)(&modgate::endian::one)==0)
#define IS_LITTLEENDIAN (*(char *)(&modgate::endian::one)==1)

#endif
