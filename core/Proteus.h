/////////////////////////////////////////////////
// Proteus.h 6.0  Copyright (c) 1997-2007 Bruce Long
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

//#include "..\ProteusCore 10.0\stop4786warning.h"   // Compensates for an annoying MS error.
#include <map>
#include <queue>
#include <iostream>
#include <string.h>
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
    tType=3, tUnknown=0, tUInt=1, tString=2, tList=3,
    isFirst=0x01000000, isLast=0x02000000, isTop=0x04000000, isBottom=0x8000000,
    noAlts=0, hasAlts=0x10000000, noMoreAlts=0x20000000, isTentative=0x40000000, isVirtual=0x80000000
    };

struct infon;
struct infNode {infon* item; infon* slot; ptrdiff_t idFlags; infNode* next; infNode(infon* itm=0, ptrdiff_t f=0):item(itm),idFlags(f){};};
enum {WorkType=0xf, MergeIdent=0, ProcessAlternatives=1, CountSize=2, SetComplete=3, EliminateAlts=4, NodeDoneFlag=8, NoMatch=16,isRawFlag=32};

struct infon {
    infon(ptrdiff_t f=0,infon* s=0, infon*v=0,infNode*ID=0,infon*s1=0,infon*s2=0,infon*n=0):
        flags(f), size(s), value(v), next(n), pred(0), spec1(s1), spec2(s2), wrkList(ID) {prev=0; top=0;};
    ptrdiff_t  flags;
    ptrdiff_t wSize; // get rid if this. disallow strings and lists in "size"
    infon *size;        // The *-term; perhaps just a number of states
    infon *value;       // Summand List
    infon *next, *prev, *top, *pred;
    infon *spec1, *spec2;   // Used to store indexes, functions args, etc.
    infNode* wrkList;
};

struct Qitem{infon* item; infon* firstID; ptrdiff_t IDStatus; ptrdiff_t level;
     Qitem(infon* i=0,infon* f=0,ptrdiff_t s=0,ptrdiff_t l=0):item(i),firstID(f),IDStatus(s),level(l){};};
typedef std::queue<Qitem> infQ;

extern infon* World;
extern std::map<stng,infon*> tag2Ptr;
extern std::map<infon*,stng> ptr2Tag;
struct agent {
    agent(){world=World;};
    int compute(infon* i);
    int doWorkList(infon* ci, infon* CIfol, int asAlt=0);
    infon* normalize(infon* i, infon* firstID=0, bool doShortNorm=false);
    infon *world, context;
};
std::string printHTMLHeader(std::string ItemToNorm);
std::string printHTMLFooter(std::string ErrorMsg);
std::string printInfon(infon* i, infon* CI=0);
std::string printPure (infon* i, ptrdiff_t f, ptrdiff_t wSize, infon* CI=0);
const int bufmax=1024*32;
struct QParser{
    QParser(std::istream& _stream):stream(_stream){};
    infon* parse(); // if there is an error it is returned in buf as a char* string.
    ptrdiff_t ReadPureInfon(char &tag, infon** i, ptrdiff_t* flags, infon** s2);
    infon* ReadInfon(int noIDs=0);
    void scanPast(char* str);
    void RmvWSC ();
    char peek();
    std::istream& stream;
    std::string s;
    char buf[bufmax];
    int line;  // linenumber
    infon* ti; // top infon
};
//extern std::fstream log;
#define OUT(msg) /*{std::cout<< msg;}*/
#define DEB(msg) /*{std::cout<< msg << "\n";}*/
#define getInt(inf, num, sign) {/*normalize(inf);  */           \
  ptrdiff_t f=inf->flags;                           \
  if((f&tType)==tUInt && !(f&fUnknown)) num=(ptrdiff_t)inf->value; else num=0;     \
  sign=f&fInvert;}

#define insertID(list, itm, flag) {IDp=(*list); (*list)=new infNode(itm,flag); \
    if(IDp){(*list)->next=IDp->next; IDp->next=(*list);} else (*list)->next=(*list); }

#define StartTerm(varIn,varOut,tag) { EOT_##tag=0;           \
while(varIn &&(varIn->flags&fConcat)) {varIn=varIn->value;}\
if (varIn==0) EOT_##tag=2;      \
else if ((varIn->flags&tType)!=tList) {EOT_##tag=3;}  \
else {varOut=varIn->value; if (varOut==0) EOT_##tag=4;}} \

#define LastTerm(varIn,varOut,tag) { EOT_##tag=0;     \
while(varIn && (varIn->flags&fConcat)) {varIn=varIn->value->prev;}\
if (varIn==0) EOT_##tag=1;               \
else if((varIn->flags&tType)!=tList) EOT_##tag=1;     \
else {varOut=varIn->value->prev; if (varOut==0) EOT_##tag=1;}}

#define getNextTerm(p,tag) { gnt1##tag:                         \
    if(p->flags&isBottom)                                           \
        if (p->flags&isLast){                                 \
          infon* parent=(p->flags&isFirst)?p->top:p->top->top; \
           if(parent==0){EOT_##tag=true; p=p->top;}    \
           else if(!(parent->flags&fConcat)||(p->next!=parent->value)) {EOT_##tag=true; p=p->top;} \
            else {p=parent; goto gnt1##tag;}          \
            }                                                  \
        else {EOT_##tag=true;} /*Bottom but not end, make subscription*/    \
    else {                                                   \
        p=p->next;                                            \
gnt2##tag:                                                  \
      if (p==0) EOT_##tag=true;                             \
      else if (p&&(p->flags&fConcat)) \
            if (p->value==0) {goto gnt1##tag;}                  \
            else {p=p->value; goto gnt2##tag;}                  \
   }}

#define getPrevTerm(p,tag) { gpt1##tag:                                 \
    if(p->flags&isTop)                                             \
        if (p->flags&isFirst)                                 \
            if (p!=p->first->size){EOT_##tag=true; p=p->prev;}       \
            else {p=p->first->first; goto gpt1##tag;}         \
        else {EOT_##tag=true;} /*Bottom but not end, make subscription*/    \
    else {                                                   \
        p=p->prev;                                            \
gpt2##tag:  if ((((p->flags&mFlags1)>>8)&rType)==rList)     \
            if (p->size==0) {goto gpt1##tag;}                  \
            else {p=p->size->prev; goto gpt2##tag;}            \
        else if(p==0) EOT_##tag=true;                         \
   }}

#endif
