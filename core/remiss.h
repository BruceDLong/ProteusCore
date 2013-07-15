#ifndef _remiss
#define _remiss
#include <cctype>

// fix g++ deficiencies
inline char* itoa(unsigned int n, char str[]){
  int len=0; int i;
  do {
    str[len++]='0'+n%10;
    n/=10;
  } while(n>0);
  str[len]=0;
  for (i=0; i<len/2; i++){
    char tmp=str[i];
    str[i]=str[len-i-1];
    str[len-i-1]=tmp;
  }
  return str;
}

inline int iscsym(int ch){
  if (isalnum(ch)) return 1;
  else return (ch=='_');
}


///////////////////////// handle endian-ness
/*#if defined (__GLIBC__)
# include <endian.h>
# if (__BYTE_ORDER == __LITTLE_ENDIAN)
#  define LITTLE_ENDIAN
# elif (__BYTE_ORDER == __BIG_ENDIAN)
#  define BIG_ENDIAN
# elif (__BYTE_ORDER == __PDP_ENDIAN)
#  define PDP_ENDIAN
# else
#  error Machine endianness unknown.
#else
# error GLIBC is not being used; manually define endian-ness of target machine.
#endif
*/
//#define BIG_ENDIAN    1
#if defined (BIG_ENDIAN)
#define reendian32(x)   __builtin_bswap32 (x) /* ((x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) |(x<<24)) */
#define reendian64(ull) __builtin_bswap64(ull) /* ((ull >> 56) | ((ull<<40) & 0x00FF000000000000) | ((ull<<24) & 0x0000FF0000000000) | ((ull<<8) & 0x000000FF00000000) | \
                         //((ull>>8) & 0x00000000FF000000) | ((ull>>24) & 0x0000000000FF0000) | ((ull>>40) & 0x000000000000FF00) | (ull << 56))  */
#define doCopy(mode, to, from) {if(mode==1||mode==2)to=reendian(from);else to=from;}
#else
#define reendian32(A) (A)
#define reendian64(A) (A)
#define doCopy(mode,to,from) to=from;
#endif


#endif
