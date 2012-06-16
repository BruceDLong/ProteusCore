/////////////////////////////////////////////////
// Proteus.h 6.0  Copyright (c) 1997-2011 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _Proteus
#define _Proteus

#include <map>
#include <queue>
#include <iostream>
#include <string.h>
#include <stddef.h>
#include <unicode/locid.h>
#include <boost/intrusive_ptr.hpp>
#include <gmp.h>
#include <gmpxx.h>

#include "../libfixmath/libfixmath/fixmath.h"

typedef ptrdiff_t UInt;

struct stng {char* S; int L; stng(char* s=0, int l=0):S(s),L(l){};};

#define stngCpy(s,ch) {s.L=strlen(ch); s.S=new char[s.L+1]; memcpy(s.S,ch,s.L+1);}
#define lstngCpy(s,ch,len) {s.L=len; s.S=new char[s.L+1]; memcpy(s.S,ch,s.L); s.S[s.L]=0;}
#define stngApnd(s,ch,len) {int l=s.L+len;char*tmp=new char[l+1];memcpy(tmp,s.S,s.L);memcpy(tmp+s.L,ch,len);delete s.S;s.S=tmp;s.L=l;tmp[l]=0;}
inline void caseDown(stng* s) {for(int i=0;i<s->L;++i) if(isupper(s->S[i])) s->S[i]=tolower(s->S[i]);}
inline bool operator< (const stng& a, const stng& b)   {if(a.L==b.L) return (memcmp(a.S,b.S,a.L)< 0); else return a.L<b.L;}
inline bool operator> (const stng& a, const stng& b)   {if(a.L==b.L) return (memcmp(a.S,b.S,a.L)> 0); else return a.L>b.L;}
inline bool operator==(const stng& a, const stng& b)   {if(a.L==b.L) return (memcmp(a.S,b.S,a.L)==0); else return false;}
inline bool operator==(const stng& a, const char* b)   {std::cout <<"IN==\n"; return (strcmp(a.S, b)==0);}

struct dblPtr {char* key1; void* key2; dblPtr(char* k1=0, void* k2=0):key1(k1), key2(k2){};};
inline bool operator< (const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2<b.key2); else return (a.key1<b.key1);}
inline bool operator> (const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2>b.key2); else return (a.key1>b.key1);}
inline bool operator==(const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2==b.key2); else return false;}

enum masks {mMode=0x0800, isNormed=0x1000, asDesc=0x2000, toExec=0x4000, sizeIndef=0x100, mListPos=0xff000000, goSize=16};
enum vals {
    tType=3, tUnknown=0, tNum=1, tString=2, tList=3,   notLast=(0x04<<goSize),
    mFormat=0x7<<2, FUnknown=1<<2, fLiteral=2<<2, FConcat=3<<2, fFloat=4<<2, fRational=5<<2,  // Format of size and value fields. 0,6,7 not yet used.
    fLoop=0x20, fInvert=0x40, fIncomplete=0x80, fInvalid=0x100,
    dDecimal=1<<8, dHex=2<<8, dBinary=3<<8,

    isFirst=0x01000000, isLast=0x02000000, isTop=0x04000000, isBottom=0x8000000,
    noAlts=0, hasAlts=0x10000000, noMoreAlts=0x20000000, isTentative=0x40000000, isVirtual=0x80000000
    };
enum Intersections {iNone=0, iToWorld,iToCtxt,iToArgs,iToVars,iToPath,iToPathH,iTagUse,iTagDef,iHardFunc,
                   iGetSize,iGetType,iAssocNxt,iGetLast,iGetFirst,iGetMiddle};
enum seeds {mSeed=0x30, sNone=0x00, sUseAsFirst=0x10, sUseAsList=0x20, sUseAsLast=0x30};
enum wMasks {mFindMode = 0x0f, mIsTopRefNode = 0x1000, mIsHeadOfGetLast=0x2000, mAsProxie=0x4000, mAssoc=0x8000};
enum colonFlags {c1Left=0x100, c2Left=0x200, c1Right=0x400, c2Right=0x800};
enum normState {mnStates=0x1f0000, nsListInited=0x10000, nsNormBegan=0x20000, nsPreNormed=0x40000, nsWorkListDone=0x80000, nsNormComplete=0x100000, nsBottomNotLast=0x200000};

#define SetBits(item, mask, val) {(item) &= ~(mask); (item)|=(val);}
#define SizeIsUnknown(inf)    (((inf)->pFlag&(mFormat<<goSize))==(FUnknown<<goSize))
#define SizeIsKnown(inf)      (((inf)->pFlag&(mFormat<<goSize))!=(FUnknown<<goSize))
#define SizeIsConcat(inf)     (((inf)->pFlag&(mFormat<<goSize))==(FConcat<<goSize))
#define ValueIsUnknown(inf)   (((inf)->pFlag&mFormat)==FUnknown)
#define ValueIsKnown(inf)     (((inf)->pFlag&mFormat)!=FUnknown)
#define ValueIsConcat(inf)    (((inf)->pFlag&mFormat)==FConcat)
#define FormatIsUnknown(flag) (((flag)&mFormat)==FUnknown)
#define FormatIsKnown(flag)   (((flag)&mFormat)!=FUnknown)
#define FormatIsConcat(flag)  (((flag)&mFormat)==FConcat)
#define FormatIs(flag, fmt)   (((flag)&mFormat)==(fmt))
#define IsSimpleUnknownNum(inf) ((((inf)->pFlag&(((mFormat+tType)<<goSize)+(mFormat+tType)))==((FUnknown+tNum)<<goSize)+(FUnknown+tNum)) && (((inf)->wFlag&mFindMode)==iNone))
#define SetValueFormat(inf, fmt) SetBits((inf)->pFlag, (mFormat), (fmt))
#define SetSizeFormat(inf, fmt) SetBits((inf)->pFlag, (mFormat<<goSize), (fmt<<goSize))
#define PureIsInListMode(pure) (1)//(((((pure).flags&tType)==tList) && (((pure).flags&mFormat)>=fLiteral)) || ((((pure).flags&tType)!=tUnknown) && (((pure).flags&mFormat)>=FConcat)))

struct infon;

#include <unicode/unistr.h>
struct Tag {std::string tag, locale, pronunciation, norm; infon* definition;};
inline bool operator< (const Tag& a, const Tag& b) {int c=strcmp(a.norm.c_str(), b.norm.c_str()); if(c==0) return (a.locale.substr(0,2)<b.locale.substr(0,2)); else return (c<0);}
inline bool operator==(const Tag& a, const Tag& b) {if(a.norm==b.norm) return (a.locale.substr(0,2)==b.locale.substr(0,2)); else return false;}
extern std::map<Tag,infon*> tag2Ptr;
extern std::map<infon*,Tag> ptr2Tag;

struct infNode {infon* item; infon* slot; UInt idFlags; infNode* next; infNode(infon* itm=0, UInt f=0):item(itm),idFlags(f){};};
enum {WorkType=0xf, MergeIdent=0, ProcessAlternatives=1, InitSearchList=2, SetComplete=3, NodeDoneFlag=8, NoMatch=16,isRawFlag=32, skipFollower=64, mLooseType=128};

struct infonData :mpq_class {
    UInt refCnt;
    infonData(char* numStr, int base):mpq_class(numStr,base), refCnt(1){};
    infonData(char* str);
    infonData(UInt num):mpq_class(num), refCnt(1){};
};

struct pureInfon {
    UInt flagsZ;
    mpq_class offset;
    union{infonData* dataHead; infon* listHead; infon* proxie;};

    //operator UInt() { if((flags&(tType+mFormat))==(tNum+fLiteral)) return 5; else return -1; }
    operator std::string();
    void setValUI(const UInt &num);
    std::string toString(int base=10);
//    pureInfon(UInt flag=0, infonData* Head=0):flags(flag), dataHead(Head){};
    pureInfon(char* str, int base=0);
    pureInfon(UInt num=0) {flagsZ=tNum+fLiteral; dataHead=new infonData(num);};
  //  pureInfon(pureInfon* pInf){if(pInf){flags=pInf->flags; dataHead=pInf->dataHead; offset=pInf->offset; if (dataHead) dataHead->refCnt++;} else {flags=0; dataHead=0; offset=0;}}
    ~pureInfon(){if(dataHead) {if(--dataHead->refCnt == 0) delete (dataHead); }}
};

struct infon {
    infon(UInt pf=0, UInt wf=0, infonData* s=0, infonData* v=0, infNode*ID=0,infon*s1=0,infon*s2=0,infon*n=0):
        pFlag(pf), wFlag(wf), next(n), pred(0), spec1(s1), spec2(s2), wrkList(ID)
        {prev=0; top=0; top2=0; type=0; pos=0; size.dataHead=s; value.dataHead=v;};
    infon* isntLast(); // 0=this is the last one. >0 = pointer to predecessor of the next one.
    mpz_class& getSize();
    UInt pFlag, wFlag;
    uint64_t pos;
    UInt wSize; // get rid if this. disallow strings and lists in "size"
    pureInfon size;        // The *-term; number of states, chars or items
    pureInfon value;       // Summand
    infon *next, *prev, *top, *top2, *pred;
    infon *spec1, *spec2;   // Used to store indexes, functions args, etc.
    infNode* wrkList;
    Tag* type;
};

inline int infonSizeCmp(infon* left, infon* right) { // -1: L<R,  0: L=R, 1: L>R. Infons must have fLiteral, numeric sizes
    UInt leftType=left->pFlag&tType, rightType=right->pFlag&tType;
    if(leftType==rightType) return cmp(left->getSize(), right->getSize());
    else throw "TODO: make infonSizeCmp compare strings to numbers, etc.";
}

struct Qitem;
typedef boost::intrusive_ptr<Qitem> QitemPtr;
struct Qitem{infon *item, *CI, *CIfol; infon* firstID; UInt IDStatus; UInt level; int refCnt, bufCnt; QitemPtr parent; int override, doShortNorm, nxtLvl, whatNext;
     Qitem(infon* i=0,infon* f=0,UInt s=0,UInt l=0, int BufCnt=0, QitemPtr Prnt=0):item(i),firstID(f),IDStatus(s),level(l),bufCnt(BufCnt),parent(Prnt){CI=CIfol=0;refCnt=0;};};
inline void intrusive_ptr_add_ref(Qitem* qp){++qp->refCnt;}
inline void intrusive_ptr_release(Qitem* qp){if(--qp->refCnt == 0) delete qp;}
typedef std::queue<QitemPtr> infQ;
typedef std::map<infon*, infon*> PtrMap;

struct agent {
    agent(infon* World=0, bool (*isHF)(std::string)=0, int (*eval)(infon*, agent*)=0){world=World; isHardFunc=isHF; autoEval=eval;};
    int StartTerm(infon* varIn, infon** varOut);
    int LastTerm(infon* varIn, infon** varOut);
    int getNextTerm(infon** p);
    int getPrevTerm(infon** p);
    int getNextNormal(infon** p);
    int gIntNxt(infon** ItmPtr);
    fix16_t gRealNxt(infon** ItmPtr);
    char* gStrNxt(infon** ItmPtr, char*txtBuff);
    infon* gListNxt(infon** ItmPtr);
    infon* append(infon* i, infon* list);
    int checkTypeMatch(Tag* LType, Tag* RType);
    int compute(infon* i);
    int doWorkList(infon* ci, infon* CIfol, int asAlt=0);
    void preNormalize(infon* CI, Qitem *cn);
    infon* normalize(infon* i, infon* firstID=0);
   infon* Normalize(infon* i, infon* firstID=0);
    infon *world, context;
    icu::Locale locale;
    void* utilField; // Field for application specific use.
    void deepCopy(infon* from, infon* to, PtrMap* ptrs=0, int flags=0);
    int loadInfon(const char* filename, infon** inf, bool normIt=true);

    int fetch_NodesNormalForm(QitemPtr cn);
    void pushNextInfon(infon* CI, QitemPtr cn, infQ &ItmQ);
    private:
        bool (*isHardFunc)(std::string);
        int (*autoEval)(infon*, agent*);
        std::map<dblPtr,UInt> alts;
        void InitList(infon* item);
        infon* copyList(infon* from, int flags);
        void processVirtual(infon* v);
        int getFollower(infon** lval, infon* i);
        void AddSizeAlternate(infon* Lval, infon* Rval, infon* Pred, UInt Size, infon* Last, UInt Flags);
        void addIDs(infon* Lvals, infon* Rvals, UInt flags, int asAlt);
};

// From InfonIO.cpp
std::string printInfon(infon* i, infon* CI=0);
std::string printPure (infonData* i, UInt f, UInt wSize, infon* CI=0);
const int bufmax=1024*32;
struct QParser{
    QParser(std::istream& _stream):stream(_stream){};
    infon* parse(); // if there is an error it is returned in buf as a char* string.
    UInt ReadPureInfon(infonData** i, UInt* pFlag, UInt *flags2, infon** s2);
    infon* ReadInfon(int noIDs=0);
    char streamGet();
    void scanPast(char* str);
    bool chkStr(const char* tok);
    const char* nxtTokN(int n, ...);
    void RmvWSC ();
    char peek(); // Returns next char.
    char Peek(); // Returns next char after whitespace.
    std::istream& stream;
    std::string s;
    char buf[bufmax];
    char nTok; // First character of last token
    int line;  // linenumber, position in text
    std::string textParsed;
    icu::Locale locale;
    infon* ti; // top infon
};

enum WorkItemResults {DoNothing, BypassDeadEnd, DoNext, DoNextBounded};

//extern std::fstream log;
#define OUT(msg) {std::cout<< msg;}
#define Debug(msg) /*{std::cout<< msg << "\n";}*/
#define isEq(L,R) (L && R && strcmp(L,R)==0)

inline void getInt(infon* inf, int* num, int* sign) {
  UInt f=inf->pFlag;
//UNDO:  if((f&tType)==tNum && FormatIs(f, fLiteral)) (*num)=(UInt)inf->value; else (*num)=0;
  *sign=f&fInvert;
 }

inline double getReal(infon* inf) {
    UInt f=inf->pFlag;
    if((f&tType)==tNum && FormatIsKnown(f)) {
//UNDO:        if(f&tReal) return ((f&fInvert)? (-(fix16_t)inf->value): (fix16_t)inf->value);
 //UNDO:       else return fix16_from_int((f&fInvert)?(-(int)inf->value):((int)inf->value));
    } return 0;
}

inline void getStng(infon* i, stng* str) {
    if((i->pFlag&tType)==tString && ValueIsKnown(i)) {
 //UNDO:       str->S=(char*)i->value; str->L=(UInt)i->size;
    }
}

#define prependID(list, node){infNode *IDp=(*list); if(IDp){node->next=IDp->next; IDp->next=node;} else {(*list)=node; node->next=node;}}
#define appendID(list, node) {infNode *IDp=(*list); (*list)=node; if(IDp){(*list)->next=IDp->next; IDp->next=(*list);} else (*list)->next=(*list);}
#define insertID(list, itm, flag) {appendID(list, new infNode(itm,flag));}

#endif
