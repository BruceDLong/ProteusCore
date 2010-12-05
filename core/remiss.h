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

//#define BIG_ENDIAN	1
#if defined (BIG_ENDIAN)
#define reendian(A) ((((UInt)(A)&0xff000000)>>24)|(((UInt)(A)&0x00ff0000)>>8)|\
		(((UInt)(A)&0x0000ff00)<<8)|(((UInt)(A)&0x000000ff)<<24))
#define doCopy(mode, to, from) {if(mode==1||mode==2)to=reendian(from);else to=from;}
#else
#define reendian(A) (A)
#define doCopy(mode,to,from) to=from;
#endif

