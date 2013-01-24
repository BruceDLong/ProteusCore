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
#include <list>
#include <queue>
#include <iostream>
#include <string>
#include <stddef.h>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/putil.h>
#include <unicode/ustream.h>
#include <boost/intrusive_ptr.hpp>
#include <gmp.h>
#include <gmpxx.h>

#include "xlater.h"
#include "wordLibrary.h"

using namespace std;

///////////////////  FLAG MASKS, VALUES AND DEFINES  ///////////////////

enum InfonFlags {  // Currently called 'pureInfon'
    mType=3, tUnknown=0, tNum=1, tString=2, tList=3,
    mFormat=0x7<<2, fUnknown=0x4, fLiteral=0x8, fConcat=0x0C, fFloat=0x10, fFract=0x14,  // Format of size and value fields. 0,6,7 not yet used.
    fLoop=0x20, fInvert=0x40, fIncomplete=0x80, fInvalid=0x100,
    dDecimal=1<<8, dHex=2<<8, dBinary=3<<8
    };

enum Intersections {iNone=0, iToWorld,iToCtxt,iToArgs,iToVars,iToPath,iToPathH,iTagUse,iTagDef,iHardFunc=0x9,iGetSize,iGetType,iAssocNxt,iGetLast,iGetFirst,iGetMiddle};
enum {xOptmize1=0x20};
enum seeds {mSeed=0x300, sNone=0x000, sUseAsFirst=0x100, sUseAsList=0x200, sUseAsLast=0x300};
enum wMasks {mFindMode = 0x0f, mIsTopRefNode = 0x1000, mIsHeadOfGetLast=0x2000, mAsProxie=0x4000, mAssoc=0x8000};
enum masks {mMode=0x080000, isNormed=0x200000, asDesc=0x400000, toExec=0x800000, asNot=0x400, sizeIndef=0x800, mListPos=0xff000000};
enum normState {mnStates=0x2f0000, nsListInited=0x10000, nsNormBegan=0x20000, nsPreNormed=0x40000, nsWorkListDone=0x80000, nsBottomNotLast=0x100000, nsNormComplete=0x200000};
enum listPos {isFirst=0x01000000, isLast=0x02000000, isTop=0x04000000, isBottom=0x08000000};
enum wMisc {noAlts=0, hasAlts=0x10000000, noMoreAlts=0x20000000, isTentative=0x40000000, isVirtual=0x80000000};

#define SetBits(item, mask, val) {(item) &= ~(mask); (item)|=(val);}
#define VsFlag(inf)           ((inf)->value.flags)
#define SsFlag(inf)           ((inf)->size.flags)
#define InfIsLoop(inf)        ((inf)->value.flags & fLoop)
#define SizeIsInverted(inf)   ( (inf)->size.flags & fInvert)
#define SizeIsUnknown(inf)    ((((inf)->size.flags & mFormat))==fUnknown)
#define SizeIsKnown(inf)      ((((inf)->size.flags & mFormat))!=fUnknown)
#define SizeIsConcat(inf)     ((((inf)->size.flags & mFormat))==fConcat)
#define ValueIsUnknown(inf)   (((inf)->value.flags & mFormat)==fUnknown)
#define ValueIsKnown(inf)     (((inf)->value.flags & mFormat)!=fUnknown)
#define ValueIsConcat(inf)    (((inf)->value.flags & mFormat)==fConcat)
#define FormatIsUnknown(flag) (((flag)&mFormat)==fUnknown)
#define FormatIsKnown(flag)   (((flag)&mFormat)!=fUnknown)
#define FormatIsConcat(flag)  (((flag)&mFormat)==fConcat)
#define FormatIs(flag, fmt)   (((flag)&mFormat)==(fmt))
#define PureIsUnknownNum(pure) (((pure).flags & (mFormat+mType)) == (fUnknown+tNum))
#define InfIsLiteralNum(inf)  (((inf)->value.flags & (mFormat+mType)) == (fLiteral+tNum))
#define SizeType(inf)         ((inf)->size.flags & mType)
#define InfsType(inf)         ((inf)->value.flags & mType)
#define InfsFormat(inf)       ((inf)->value.flags & mFormat)
#define IsSimpleUnknownNum(inf) (PureIsUnknownNum((inf)->size) && PureIsUnknownNum((inf)->value) && (((inf)->wFlag&mFindMode)==iNone))
#define SetValueFormat(inf, fmt) SetBits((inf)->value.flags, (mFormat), (fmt))
#define SetSizeFormat(inf, fmt) SetBits((inf)->size.flags, (mFormat), (fmt))
#define SetSizeType(inf, ttype) SetBits((inf)->size.flags, (mType), (ttype))
#define SetPureTypeForm(pInf,Ty,Fo) SetBits((pInf).flags, (mFormat+mType), (Ty+Fo))
#define PureIsInListMode(pure) ((pure).listHead) //(((((pure).flags&mType)==tList) && (((pure).flags&mFormat)>=fLiteral)) || ((((pure).flags&mType)!=tUnknown) && (((pure).flags&mFormat)>=fConcat)))
#define PureIsInDataMode(pure) ((pure).dataHead) //( (((pure).flags&mFormat)==fLiteral) && ((((pure).flags&mType)==tNum) || (((pure).flags&mType)==tString) ))

#define InfIsNormed(inf)       ((inf)->wFlag & isNormed)
#define InfAsDesc(inf)         ((inf)->wFlag & asDesc)
#define InfToExec(inf)         ((inf)->wFlag & toExec)
#define InfIsTop(inf)          ((inf)->wFlag & isTop)
#define InfIsBottom(inf)       ((inf)->wFlag & isBottom)
#define InfIsFirst(inf)        ((inf)->wFlag & isFirst)
#define InfIsLast(inf)         ((inf)->wFlag & isLast)
#define InfHasAlts(inf)        ((inf)->wFlag & hasAlts)
#define InfIsVirtual(inf)      ((inf)->wFlag & isVirtual)
#define InfIsTentative(inf)    ((inf)->wFlag & isTentative)
#define InfIsVirtTent(inf)     ((inf)->wFlag & (isVirtual+isTentative))
#define SetIsTent(inf)         (inf)->wFlag |=isTentative
#define SetIsVirt(inf)         (inf)->wFlag |=isVirtual
#define ResetVirt(inf)         (inf)->wFlag &= ~isVirtual
#define ResetTent(inf)         (inf)->wFlag &= ~isTentative
#define ResetVirtTent(inf)     (inf)->wFlag &= ~(isVirtual+isTentative)

extern bool iscsymOrUni (char nTok);
extern bool isTagStart(char nTok);
extern bool tagIsBad(string tag, const char* locale);
extern const icu::Normalizer2 *tagNormer;

extern multimap<infon*,WordSPtr> DefPtr2Tag;
extern sqlite3 *coreDatabase;
xlater* fetchXlater(icu::Locale *locale);
typedef map<string, xlater*> LanguageExtentions;
extern LanguageExtentions langExtentions;
extern int initializeProteusCore(char* resourceDir, char* dbName);
extern void shutdownProteusEngine();
extern int calcScopeScore(string wrdS, string trialS);

///////////////////  INFON RELATED ITEMS  ///////////////////

typedef ptrdiff_t UInt;
typedef mpz_class BigInt;
typedef mpq_class BigFrac;

struct infNode {infon* item; infon* slot; UInt idFlags; infNode* next; infNode(infon* itm=0, UInt f=0):item(itm),idFlags(f){};};
enum {WorkType=0xf, MergeIdent=0, ProcessAlternatives=1, InitSearchList=2, SetComplete=3, NodeDoneFlag=8, NoMatch=16,isRawFlag=32, skipFollower=64, mLooseType=128};
enum colonFlags {c1Left=0x100, c2Left=0x200, c1Right=0x400, c2Right=0x800};

struct infonData :BigFrac {
    UInt refCnt;
    infonData(const char* numStr, int base);
    infonData(const string str);
    infonData(BigFrac num):mpq_class(num), refCnt(0){};
};
typedef boost::intrusive_ptr<infonData> infDataPtr;

struct attrStore:map<string, string>{UInt refCnt;};
typedef boost::intrusive_ptr<attrStore> attrStorePtr;
inline void intrusive_ptr_add_ref(attrStore* p){++p->refCnt;}
inline void intrusive_ptr_release(attrStore* p){if(--p->refCnt == 0) delete p;}

struct infonIndex:map<string, infon*>{UInt refCnt;};
typedef boost::intrusive_ptr<infonIndex> infonIndexPtr;
inline void intrusive_ptr_add_ref(infonIndex* p){++p->refCnt;}
inline void intrusive_ptr_release(infonIndex* p){if(--p->refCnt == 0) delete p;}

struct pureInfon {
    UInt flags;
    BigFrac offset;
    infDataPtr dataHead;
    union{infon* listHead; infon* proxie;};

    string toString(UInt sizeBase);
    pureInfon():flags(0), offset(0), dataHead(0), listHead(0){};
    pureInfon(infDataPtr Head, UInt flag, BigFrac offSet);
    pureInfon(char* str, int base=0);
    pureInfon(BigInt num): offset(0), listHead(0){flags=tNum+fLiteral; dataHead=infDataPtr(new infonData(num));};
    pureInfon& operator=(const string &str);
    pureInfon& operator=(const pureInfon &pInf);
    pureInfon& operator=(const BigFrac &num);
    pureInfon& operator=(const UInt &num);
    ~pureInfon();
};

struct infon {
    infon(UInt wf=0, pureInfon* s=0, pureInfon* v=0, infNode*ID=0,infon*s1=0,infon*s2=0,infon*n=0);
    infon* isntLast(); // 0=this is the last one. >0 = pointer to predecessor of the next one.
    BigInt& getSize();
    bool getInt(BigInt* num);
    bool getReal(double* d);
    bool getStng(string* str);
    infon* findTag(WordS& tag);
    void updateIndex();

    UInt wFlag;
    uint64_t pos;
    UInt wSize; // get rid if this. disallow strings and lists in "size"
    pureInfon size;        // The *-term; number of states, chars or items
    pureInfon value;       // Summand
    infon *next, *prev, *top, *top2, *pred;
    infon *spec1, *spec2;   // Used to store indexes, functions args, etc.
    infNode* wrkList;
    WordSPtr type;
    attrStorePtr attrs; // Misc attributes: <tag, data-string>
    infonIndexPtr index;  // For faster finds.
    ~infon(){};
};
int infValueCmp(infon* A, infon* B);
int infonSizeCmp(infon* left, infon* right); // -1: L<R,  0: L=R, 1: L>R. Infons must have fLiteral, numeric sizes
inline void recover(infon* i){ /*delete i;*/}

inline void intrusive_ptr_add_ref(infonData* ip){++ip->refCnt;}
inline void intrusive_ptr_release(infonData* ip){if(--ip->refCnt == 0) delete ip;}

struct dblPtr {char* key1; void* key2; dblPtr(char* k1=0, void* k2=0):key1(k1), key2(k2){};};
inline bool operator< (const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2<b.key2); else return (a.key1<b.key1);}
inline bool operator> (const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2>b.key2); else return (a.key1>b.key1);}
inline bool operator==(const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2==b.key2); else return false;}

struct Qitem;
typedef boost::intrusive_ptr<Qitem> QitemPtr;
struct Qitem{infon *item, *CI, *CIfol; infon* firstID; UInt IDStatus; UInt level; int refCnt, bufCnt; QitemPtr parent; int override, doShortNorm, nxtLvl, whatNext;
     Qitem(infon* i=0,infon* f=0,UInt s=0,UInt l=0, int BufCnt=0, QitemPtr Prnt=0):item(i),firstID(f),IDStatus(s),level(l),bufCnt(BufCnt),parent(Prnt){CI=CIfol=0;refCnt=0;};};
inline void intrusive_ptr_add_ref(Qitem* qp){++qp->refCnt;}
inline void intrusive_ptr_release(Qitem* qp){if(--qp->refCnt == 0) delete qp;}
typedef queue<QitemPtr> infQ;
typedef map<infon*, infon*> PtrMap;

struct agent {
    agent(infon* World=0, bool (*isHF)(string)=0, int (*eval)(infon*, agent*)=0){world=World; isHardFunc=isHF; autoEval=eval;};
    int StartTerm(infon* varIn, infon** varOut);
    int LastTerm(infon* varIn, infon** varOut);
    int getNextTerm(infon** p);
    int getPrevTerm(infon** p);
    int getNextNormal(infon** p);
    BigInt gIntNxt(infon** ItmPtr);
    double gRealNxt(infon** ItmPtr);
    char* gStrNxt(infon** ItmPtr, char*txtBuff);
    infon* gListNxt(infon** ItmPtr);
    infon* append(infon* i, infon* list);
    int checkTypeMatch(WordSPtr LType, WordSPtr RType);
    int compute(infon* i);
    int doWorkList(infon* ci, infon* CIfol, int asAlt=0);
    void prepWorkList(infon* CI, Qitem *cn);
    infon* normalize(infon* i, infon* firstID=0);
   infon* Normalize(infon* i, infon* firstID=0);
    infon *world, context;
    icu::Locale locale;
    void* utilField; // Field for application specific use.
    void deepCopyPure(pureInfon* from, pureInfon* to, int flags);
    void deepCopy(infon* from, infon* to, PtrMap* ptrs=0, int flags=0);
    int loadInfon(const char* filename, infon** inf, bool normIt=true);

    int fetch_NodesNormalForm(QitemPtr cn);
    void pushNextInfon(infon* CI, QitemPtr cn, infQ &ItmQ);
    private:
        bool (*isHardFunc)(string);
        int (*autoEval)(infon*, agent*);
        map<dblPtr,UInt> alts;
        void InitList(infon* item);
        infon* copyList(infon* from, int flags);
        void processVirtual(infon* v);
        int getFollower(infon** lval, infon* i);
        void AddSizeAlternate(infon* Lval, infon* Rval, infon* Pred, UInt Size, infon* Last, UInt Flags);
        void addIDs(infon* Lvals, infon* Rvals, UInt flags, int asAlt);
};

// From InfonIO.cpp
string printInfon(infon* i, infon* CI=0);
string printPure (pureInfon* i, UInt wSize, infon* CI=0);
void numberFromString(char* buf, pureInfon* pInf, int base=10);
const int bufmax=1024*32;
struct QParser{
    QParser(istream& _stream):stream(_stream){};
    infon* parse(); // if there is an error it is returned in buf as a char* string.
    UInt ReadPureInfon(pureInfon* pInf, UInt* flags, UInt *wFlag, infon** s2, string &scopeID);
    infon* ReadInfon(string &scopeID, int noIDs=0);
    char streamGet();
    void scanPast(char* str);
    bool chkStr(const char* tok);
    xlater* chkLocale(icu::Locale* locale);
    WordSPtr ReadTagChain(icu::Locale* locale, xlater **XL_return, string scopeID);
    const char* nxtTokN(int n, ...);
    void RmvWSC ();
    char peek(); // Returns next char.
    char Peek(); // Returns next char after whitespace.
    istream& stream;
    string s;
    char buf[bufmax];
    char nTok; // First character of last token
    int line;  // linenumber, position in text
    string textParsed;
    agent* agnt;
    infon* ti; // top infon
};

enum WorkItemResults {DoNothing, BypassDeadEnd, DoNext, DoNextBounded};
extern bool try2CatStr(string* s, pureInfon* i, UInt wSize);
//extern fstream log;
#define OUT(msg) {cout<< msg;}
#define Debug(msg) /*{cout<< msg << "\n";}*/
#define ImAt(loc,parm) {cout<<"####### At:"<<(loc)<<"  "<<(parm)<<"\n";}
#define isEq(L,R) (L && R && strcmp(L,R)==0)

#define getTop(item) ((InfIsTop(item)||item->top==0)? item->top : item->top->top)
#define prependID(list, node){infNode *IDp=(*list); if(IDp){(node)->next=IDp->next; IDp->next=node;} else {(*list)=node; (node)->next=node;}}
#define appendID(list, node) {infNode *IDp=(*list); (*list)=node; if(IDp){(*list)->next=IDp->next; IDp->next=(*list);} else (*list)->next=(*list);}
#define insertID(list, itm, flag) {appendID(list, new infNode(itm,flag));}
#define insertIDatTop(list, itm, flag) {prependID(list, new infNode(itm,flag));}
#define cpType(from, to) {if(to->type==0) to->type=from->type;}
#define cpFlags(from, to, mask) {(to)->wFlag=(((to)->wFlag& ~(mask))+(((from)->wFlag)&mask)); cpType(from,to);}
#define copyTo(from, to) {if((from)!=(to)){(to)->size=(from)->size; (to)->value=(from)->value; cpFlags((from),(to),0x00ffffff); (to)->attrs=(from)->attrs; (to)->index=(from)->index;}}

#define PrntLocale(L) {icu::UnicodeString lang,country; cout<<L.getDisplayLanguage(L, lang) <<"-"<< L.getDisplayCountry(L, country) <<" (" << L.getBaseName()<<")\n"; }


#endif
