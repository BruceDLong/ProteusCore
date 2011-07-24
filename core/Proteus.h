/////////////////////////////////////////////////
// Proteus.h 6.0  Copyright (c) 1997-2011 Bruce Long
/*    This file is part of the "Proteus Engine"

    The Proteus Engine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Proteus Engine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _Proteus
#define _Proteus

#include <map>
#include <queue>
#include <iostream>
#include <string.h>

typedef ptrdiff_t UInt;

struct stng {char* S; int L; stng(char* s=0, int l=0):S(s),L(l){};};

#define stngCpy(s,ch) {s.L=strlen(ch); s.S=new char[s.L+1]; memcpy(s.S,ch,s.L+1);}
#define lstngCpy(s,ch,len) {s.L=len; s.S=new char[s.L+1]; memcpy(s.S,ch,s.L); s.S[s.L]=0;}
#define stngApnd(s,ch,len) {int l=s.L+len;char*tmp=new char[l+1];memcpy(tmp,s.S,s.L);memcpy(tmp+s.L,ch,len);delete s.S;s.S=tmp;s.L=l;tmp[l]=0;}
inline bool operator< (const stng& a, const stng& b)   {if(a.L==b.L) return (memcmp(a.S,b.S,a.L)< 0); else return a.L<b.L;}
inline bool operator> (const stng& a, const stng& b)   {if(a.L==b.L) return (memcmp(a.S,b.S,a.L)> 0); else return a.L>b.L;}
inline bool operator==(const stng& a, const stng& b)   {if(a.L==b.L) return (memcmp(a.S,b.S,a.L)==0); else return false;}

struct dblPtr {char* key1; void* key2; dblPtr(char* k1=0, void* k2=0):key1(k1), key2(k2){};};
inline bool operator< (const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2<b.key2); else return (a.key1<b.key1);}
inline bool operator> (const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2>b.key2); else return (a.key1>b.key1);}
inline bool operator==(const dblPtr& a, const dblPtr& b) {if(a.key1==b.key1) return (a.key2==b.key2); else return false;}

enum masks {mRepMode=0x0700, mMode=0x0800, isNormed=0x1000, asDesc=0x2000, toExec=0x4000, sizeIndef=0x8000, mListPos=0xff000000, goSize=16};
enum vals {toGiven=0, toWorldCtxt=0x0100, toHomePos=0x0200, fromHere=0x0300, asFunc=0x0400, intersect=0x0500, asTag=0x0600, asNone=0x0700,
    fMode=0xf8, fConcat=0x08, fLoop=0x10, fInvert=0x20, fIncomplete=0x40, fUnknown=0x80,
    tType=3, tUnknown=0, tUInt=1, tString=2, tList=3,   matchType=0x04,  notLast=(0x04<<goSize),
    isFirst=0x01000000, isLast=0x02000000, isTop=0x04000000, isBottom=0x8000000,
    noAlts=0, hasAlts=0x10000000, noMoreAlts=0x20000000, isTentative=0x40000000, isVirtual=0x80000000,

    useTag=0x10000 // improve this. Combine tag, none, search, etc.
    };
enum Intersections {iToWorld,iToCtxt,iToArgs,iToVars,iToPath,iToPathH,iTagUse,iTagDef,iUseAsFirst,iUseAsList,iUseAsLast,
                    iGetFirst,iGetMiddle,iGetLast,iGetSize,iGetType,iStartAssoc,iNextAssoc,iHardFunc,iNone};
const UInt mFindMode = 0x1f;  // use this to select the intersection/FindMode from wFlag

const int fUnknowns=fUnknown+(fUnknown<<goSize);

struct infon;
struct assocInfon {infon *VarRef, *nextRef; assocInfon(infon* first=0):VarRef(first),nextRef(0){};};
enum {miscWorldCtxt, miscFindAssociate};

struct infNode {infon* item; infon* slot; UInt idFlags; infNode* next; infNode(infon* itm=0, UInt f=0):item(itm),idFlags(f){};};
enum {WorkType=0xf, MergeIdent=0, ProcessAlternatives=1, CountSize=2, SetComplete=3, NodeDoneFlag=8, NoMatch=16,isRawFlag=32, skipFollower=64};

struct infon {
    infon(UInt f=0, UInt f2=0, infon* s=0, infon*v=0,infNode*ID=0,infon*s1=0,infon*s2=0,infon*n=0):
        pFlag(f), wFlag(f2), size(s), value(v), next(n), pred(0), spec1(s1), spec2(s2), wrkList(ID) {prev=0; top=0; type=0;};
    UInt pFlag, wFlag;
    UInt wSize; // get rid if this. disallow strings and lists in "size"
    infon *size;        // The *-term; perhaps just a number of states
    infon *value;       // Summand List
    infon *next, *prev, *top, *pred;
    infon *spec1, *spec2;   // Used to store indexes, functions args, etc.
    infNode* wrkList;
	stng* type;
};

struct Qitem{infon* item; infon* firstID; UInt IDStatus; UInt level; int bufCnt;
     Qitem(infon* i=0,infon* f=0,UInt s=0,UInt l=0, int BufCnt=0):item(i),firstID(f),IDStatus(s),level(l),bufCnt(BufCnt){};};
typedef std::queue<Qitem> infQ;

extern infon* World;
extern std::map<stng,infon*> tag2Ptr;
extern std::map<infon*,stng> ptr2Tag;
struct agent {
    agent(){world=World;};
    inline int StartTerm(infon* varIn, infon** varOut);
    inline int LastTerm(infon* varIn, infon** varOut);
    inline int getNextTerm(infon** p);
    inline int getPrevTerm(infon** p);
	inline int getNextNormal(infon** p);
	void append(infon* i, infon* list);
    int compute(infon* i);
    int doWorkList(infon* ci, infon* CIfol, int asAlt=0);
    infon* fillBlanks(infon* i, infon* firstID=0, bool doShortNorm=false);
    infon *world, context;
        void deepCopy(infon* from, infon* to, infon* args=0);
    private:
        void InitList(infon* item);
        infNode* copyIdentList(infNode* from);
        infon* copyList(infon* from);
        void processVirtual(infon* v);
        int getFollower(infon** lval, infon* i);
        void addIDs(infon* Lvals, infon* Rvals, int asAlt=0);
};

std::string printInfon(infon* i, infon* CI=0);
std::string printPure (infon* i, UInt f, UInt wSize, infon* CI=0);
const int bufmax=1024*32;
struct QParser{
    QParser(std::istream& _stream):stream(_stream){};
    infon* parse(); // if there is an error it is returned in buf as a char* string.
    UInt ReadPureInfon(char &tag, infon** i, UInt* pFlag, UInt *flags2, infon** s2);
    infon* ReadInfon(int noIDs=0);
	char streamGet();
    void scanPast(char* str);
    bool chkStr(const char* tok);
    const char* lookGet(int n, ...);
    void RmvWSC ();
    char peek();
    std::istream& stream;
    std::string s;
    char buf[bufmax];
    int line, txtPos;  // linenumber, position in text
	std::string textParsed;
    infon* ti; // top infon
};
//extern std::fstream log;
#define OUT(msg) /*{std::cout<< msg;}*/
#define DEB(msg) /*{std::cout<< msg << "\n";}*/
#define getInt(inf, num, sign) {/*fillBlanks(inf);  */           \
  UInt f=inf->pFlag;                           \
  if((f&tType)==tUInt && !(f&fUnknown)) num=(UInt)inf->value; else num=0;     \
  sign=f&fInvert;}

inline void getStng(infon* i, stng* str) {
	if((i->pFlag&tType)==tString && !(i->pFlag&fUnknown)) {
		str->S=(char*)i->value; str->L=(UInt)i->size;
	}
}

#define insertID(list, itm, flag) {IDp=(*list); (*list)=new infNode(itm,flag); \
    if(IDp){(*list)->next=IDp->next; IDp->next=(*list);} else (*list)->next=(*list); }

int  getNextTerm(infon** p);  // forward decl

#endif
