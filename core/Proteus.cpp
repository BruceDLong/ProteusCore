///////////////////////////////////////////////////
// Proteus.cpp 10.0  Copyright (c) 1997-2011 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <fstream>
#include "Proteus.h"
#include "remiss.h"

const int ListBuffCutoff=20;

typedef std::map<dblPtr,UInt>::iterator altIter;

#define recAlts(lval, rval) {if((rval->pFlag&tType)==tString) alts[dblPtr((char*)rval->value,lval)]++;}
#define getTop(item) (((item->pFlag&isTop)||item->top==0)? item->top : item->top->top)
#define fetchLastItem(lval, item) {for(lval=item;(lval->pFlag&tType)==tList;lval=lval->value->prev);}
#define fetchFirstItem(lval, item) {for(lval=item;(lval->pFlag&tType)==tList;lval=lval->value){};}
#define cpType(from, to) {if(to->type==0) to->type=from->type;}
#define cpFlags(from, to, mask) {to->pFlag=(to->pFlag& ~(mask))+((from)->pFlag&mask); to->wFlag=from->wFlag; cpType(from,to);}
#define copyTo(from, to) {if(from!=to){to->size=(from)->size; to->value=(from)->value; cpFlags((from),to,0x00ffffff);}}
#define AddSizeAlternate(Lval, Rval, Pred, Size, Last) { infon *copy, *copy2, *LvalFol;    \
        copy=new infon(Lval->pFlag,Lval->wFlag,(infon*)(Size),Lval->value,0,Lval->spec1,Lval->spec2,Lval->next); \
        copy->prev=Lval->prev; copy->top=Lval->top;copy->pred=Pred; \
        insertID(&Lval->wrkList,copy,ProcessAlternatives); Lval->wrkList->slot=Last;\
        getFollower(&LvalFol, Lval);                  \
        if (LvalFol){\
            copy2=new infon(LvalFol->pFlag,LvalFol->wFlag,LvalFol->size,LvalFol->value,0,LvalFol->spec1,LvalFol->spec2,LvalFol->next);\
            copy2->pred=copy; insertID(&copy2->wrkList,Rval,0); insertID(&LvalFol->wrkList,copy2,ProcessAlternatives);}}

infon* infon::isntLast(){ // 0=this is the last one. >0 = pointer to predecessor of the next one.
    if (!(pFlag&isLast)) return this;
    infon* parent=getTop(this); infon* gParent=getTop(parent);
    if(gParent && (gParent->pFlag&fConcat))
        return parent->isntLast();
    return 0;
}

int agent::loadInfon(const char* filename, infon** inf, bool normIt){
    std::cout<<"Loading:'"<<filename<<"'...";
    std::fstream InfonIn(filename);
    if(InfonIn.fail()){std::cout<<"Error: The file "<<filename<<" was not found.\n"; return 1;}
    QParser T(InfonIn);
    *inf=T.parse();
    if (*inf) {std::cout<<"done.\n";}
    else {std::cout<<"Error:"<<T.buf<<"\n"; return 1;}
    if(normIt) {
        alts.clear();
        try{
            normalize(*inf); // cout << "Normalizd\n";
        } catch (char const* errMsg){std::cout<<errMsg<<"\n";}
    }
    return 0;
}

int agent::StartTerm(infon* varIn, infon** varOut) {
  infon* tmp;
  if (varIn==0) return 1;
  int varInFlags=varIn->pFlag;
  if (varInFlags&fConcat){
    if ((*varOut=varIn->value)==0) return 2;
    do {
      switch(varInFlags&tType){
        case tUnknown: *varOut=0; return -1;
        case tNum: case tString: return 0;
        case tList: if(StartTerm(*varOut, &tmp)==0) {
          *varOut=tmp;
          return 0;
          }
        }
      if (getNextTerm(varOut)) return 4;
    } while(1);
  }
  if ((varIn->pFlag&tType)!=tList) {return 3;}
  else {*varOut=varIn->value; if (*varOut==0) return 4;}
  return 0;
}

int agent::LastTerm(infon* varIn, infon** varOut) {
  if (varIn==0) return 1;
  if (varIn->pFlag&fConcat) {varIn=varIn->value->prev;}
  else if((varIn->pFlag&tType)!=tList)  return 2;
  else {*varOut=varIn->value->prev; if (*varOut==0) return 3;}
  return 0;
}

int agent::getNextTerm(infon** p) {
  infon *parent, *Gparent=0, *GGparent=0;
  if((*p)->pFlag&isBottom) {
    if ((*p)->pFlag&isLast){
      parent=((*p)->pFlag&isFirst)?(*p)->top:(*p)->top->top;  // TODO B4: This should be getTop, but test it.
      if(parent==0){(*p)=(*p)->next; return 1;}
      if(parent->top) Gparent=(parent->pFlag&isFirst)?parent->top:parent->top->top;
      if(Gparent && (Gparent->pFlag&fConcat)) {
        if(Gparent->top) GGparent=(Gparent->pFlag&isFirst)?Gparent->top:Gparent->top->top;
        do {
          if(getNextTerm(&parent)) return 4;
// TODO: the next line fixes one problem but causes another: nested list-concats.
// test it with:  (   ({} {"X", "Y"} ({} {9,8} {7,6}) {5} ) {1} {{}} {2, 3, 4} )
          if(GGparent && (GGparent->pFlag&fConcat)) {*p=parent; return 0;}
//    normalize(parent);
          switch(parent->pFlag&tType){
            case tUnknown: (*p)=0; return -1;
            case tNum: case tString: throw("Item in list concat is not a list.");
            case tList: if(!StartTerm(parent, p)) return 0;
            }
        } while (1);
      }
      return 2;
    } else {std::cout <<"BOTTOM\n"; return 5;} /*Bottom but not end, make subscription*/
  }else {
    (*p)=(*p)->next;
    if ((*p)==0) {return 3; }
    if ((*p)->pFlag&isLast){     // if isLast && Parent is spec1-of-get-last, get parent, apply any idents
        parent=getTop((*p));
        if(parent->wFlag&mIsHeadOfGetLast) {
            parent=parent->top2; // This is a different use for 'prev'. It should be OK.
            infNode *j=parent->wrkList, *k;  // Merge Identity Lists
            if(j) do {
                k=new infNode; k->item=new infon; k->idFlags=j->idFlags;
                deepCopy (j->item, k->item);
                appendID(&(*p)->wrkList, k);
                j=j->next;
            } while (j!=parent->wrkList);
        }
    }
  }
  return 0;
}

int agent::getNextNormal(infon** p){
    int result=getNextTerm (p);
    if (result==0 && !((*p)->pFlag&isNormed)) normalize(*p);
    return result;
}

// TODO: This function is out-of-date but not used yet anyhow.
int agent::getPrevTerm(infon** p) { gpt1:
  if((*p)->pFlag&isTop)
    if ((*p)->pFlag&isFirst) {}
//      if ((*p)!=(*p)->first->size){(*p)=(*p)->prev; return 1;}
//      else {(*p)=(*p)->first->first; goto gpt1;}
          else {return 2;} /*Bottom but not end, make subscription*/
  else {
    (*p)=(*p)->prev;
gpt2: //if (((((*p)->pFlag&mFlags1)>>8)&rType)==rList)
    if ((*p)->size==0) {goto gpt1;}
    else {(*p)=(*p)->size->prev; goto gpt2;}
    if((*p)==0) return 3;
  }
  return 0;
}

//////////// Routines for copying infon data to C formats:
int agent::gIntNxt(infon** ItmPtr){
    UInt num,sign;
    getInt(*ItmPtr,&num,&sign);
    getNextTerm(ItmPtr);
    return (sign)?-num:num;
}

fix16_t agent::gRealNxt(infon** ItmPtr){
    fix16_t ret=getReal(*ItmPtr);
    getNextTerm(ItmPtr);
    return ret;
}

char* agent::gStrNxt(infon** ItmPtr, char*txtBuff){
    if(((*ItmPtr)->pFlag&tType)==tString && !((*ItmPtr)->pFlag&fUnknown)) {
        memcpy(txtBuff, (*ItmPtr)->value, (UInt)(*ItmPtr)->size);
        txtBuff[(UInt)((*ItmPtr)->size)]=0;
    }
    getNextTerm(ItmPtr);
    return txtBuff;
}

infon* agent::gListNxt(infon** ItmPtr){
    infon* ret=*ItmPtr;
    getNextTerm(ItmPtr);
    return ret;
}

///////////// Routines for copying, appending, etc.
void agent::append(infon* i, infon* list){ // appends an item to the end of a list
    i->next=i->top=list->value; i->prev=i->next->prev; i->next->prev=i;
    i->prev->pFlag^=isBottom; i->pFlag|=isBottom;
//  signalSubscriptions();
}

infon* agent::copyList(infon* from, int flags){
    infon* top=0; infon* follower; infon* p=from;
    if(from==0)return 0;
    do {
        infon* q=new infon;
        deepCopy(p,q, 0, 0, flags);
        if (top==0) follower=top=q;
        q->prev=follower; q->top=top; follower=follower->next=q;
        p=p->next;
    } while(p!=from);
    top->prev=follower; top->pFlag|=from->pFlag&mListPos;
    follower->next=top;
    return top;
}

infon* getVeryTop(infon* i){
    infon* j=i;
    while(i!=0) {j=i; i=getTop(i);}
    return j;
}

void deTagWrkList(infon* i){ // After a tag is derefed, move any c1Left wrkList items to i->spec1.
    // TODO B4: make sure that if the right side (i.e., q->item) is a tag that the c1Right flag get applied if needed.
    if((i->wFlag&mFindMode)==iTagUse) return;
    infNode *q, *p=i->wrkList;
    if(p) do {
        q=p->next;
        if(q->idFlags&c1Left){
            q->idFlags-=c1Left;
            p->next=q->next;
            appendID(&i->spec1->wrkList, q);
            if(p==q) {i->wrkList=0; break;}
        }
        p=p->next;
    } while (p!=i->wrkList);
}

void agent::deepCopy(infon* from, infon* to, infon* args, PtrMap* ptrs, int flags){
    UInt fm=from->wFlag&mFindMode; infon* tmp;
    to->pFlag=(from->pFlag&0x0fffffff)/*|(to->pFlag&0xff000000)*/; to->wFlag=from->wFlag;
    if(to->type==0) to->type=from->type; // TODO: We should merge types, not just copy over.

    if(((from->pFlag>>goSize)&tType)==tList || ((from->pFlag>>goSize)&fConcat)){to->size=copyList(from->size, flags); if(to->size)to->size->top=to;}
    else to->size=from->size;

    if((from->pFlag&tType)==tList || ((from->pFlag)&fConcat)){to->value=copyList(from->value, flags); if(to->value)to->value->top=to;}
    else to->value=from->value;
    if(from->wFlag&mIsHeadOfGetLast) to->top2=(*ptrs)[from->top2]; // The 'mIsHeadOfGetLast' use of prev
    if(fm==iToPath || fm==iToPathH || fm==iToArgs || fm==iToVars) {
        to->spec1=from->spec1;
        if (ptrs) to->top=(*ptrs)[from->top];
    } else if((fm==iGetFirst || fm==iGetLast || fm==iGetMiddle) && from->spec1) {
        PtrMap *ptrMap = (ptrs)?ptrs:new PtrMap;
        if ((to->prev) && (tmp=to->prev->spec1) && (tmp->wFlag&mIsHeadOfGetLast) && (tmp=tmp->next)) to->spec1=tmp;
        else to->spec1=new infon;
        (*ptrMap)[from]=to;
        deepCopy (from->spec1, to->spec1, 0, ptrMap, flags);  // In Old List Copy this was copyTo, not DeepCopy!!!!!!!!!!!!!
        if(ptrs==0) delete ptrMap;
        if (flags && from->wFlag&mAssoc) {// std::cout<<"Initializing ASSOC\n";
            from->wFlag&= ~(mAssoc+mFindMode); from->wFlag|=iAssocNxt; from->pFlag|=(fUnknown+(fUnknown<<goSize));}
    } else if((UInt)(from->spec1)<=20) to->spec1=from->spec1; // TODO B4: Is this needed?
    else if(fm<(UInt)asFunc){ // Get next item in repeated indexing
        to->spec1=new infon; copyTo(from->spec1,to->spec1); // copy head of list being searched.
        if(to->prev && to->prev!=to){ // If we aren't the first virtual in the growing list...
            infon* nextNode=to->prev->spec2;   // The prev fields, here, are storing ptr to where the previous search left off.
            if (nextNode && nextNode->next==0 && (UInt)(nextNode->prev)>1){ // 1 means seed this; we're just starting a search.
                to->spec1->value=nextNode->prev;  // to->spec1->value now points to where search should continue.
                to->spec1->size=(infon*)((UInt)to->prev->spec1->size - (UInt)nextNode->size); // We've moved, so size is less.
            }
        }}
    else {to->spec1=new infon; deepCopy((args)?args:from->spec1,to->spec1);}

    if ((from->wFlag&mFindMode)==iAssocNxt)
        {to->spec2=from;}  // Breadcrumbs to later find next associated item.
    else if(!from->spec2) to->spec2=0;
    else {to->spec2=new infon; deepCopy(from->spec2, to->spec2,0,0,flags);}

    infNode *p=from->wrkList, *q;  // Merge Identity Lists
    if(p) do {
        q=new infNode; q->item=new infon; q->idFlags=p->idFlags;
        if((p->item->pFlag&mFindMode)==iNone) {q->item=new infon; q->idFlags=p->idFlags; deepCopy (p->item, q->item, 0, ptrs);}
        else {q->item=p->item;}
        prependID(&to->wrkList, q);
        p=p->next;
    } while (p!=from->wrkList);
}

void recover(infon* i){ delete i;}

void closeListAtItem(infon* lastItem){ // remove (no-longer valid) items to the right of lastItem in a list.
std::cout << "CLOSING: " << printInfon(lastItem) << "\n";
    // TODO: Will this work when a list becomes empty?
    infon *itemAfterLast, *nextItem; UInt count=0;
    for(itemAfterLast=lastItem->next; !(itemAfterLast->pFlag&isTop); itemAfterLast=nextItem){
        nextItem=itemAfterLast->next;
        recover(itemAfterLast);
    }
    lastItem->next=itemAfterLast; itemAfterLast->prev=lastItem; lastItem->pFlag|=(isBottom+isLast);
    // The lines below remove any tentative flag then set parent's size
    do{
        itemAfterLast->pFlag &=~ isTentative;
        itemAfterLast=itemAfterLast->next;
        count++;
    } while (!(itemAfterLast->pFlag&isTop));
    itemAfterLast->top->size=(infon*)count;
    itemAfterLast->pFlag &=~(fUnknown<<goSize);
}

const int simpleUnknownUint=((fUnknown+tNum)<<goSize) + fUnknown+tNum+asNone;
char isPosLorEorGtoSize(UInt pos, infon* item){
    if(item->pFlag&(fUnknown<<goSize)) return '?';
    if(((item->pFlag>>goSize)&tType)!=tNum) throw "Size as non integer is probably not useful, so not allowed.";
    if(item->pFlag&(fConcat<<goSize)){
    infon* size=item->size;
        //if ((size->pFlag&mRepMode)!=asNone) normalize(size);
        if ((size->pFlag&simpleUnknownUint)==simpleUnknownUint)
            if(size->next!=size && size->next==size->prev && ((size->next->pFlag&simpleUnknownUint)==simpleUnknownUint)){
                    if(pos<(UInt)size->value) return 'L';
                        if(pos>(UInt)size->size) return 'G';
                    return '?';  // later, if pos is in between two ranges, return 'X'
                }
    }
    if(pos<(UInt)item->size) return 'L';  // Less
    if(pos==(UInt)item->size) return 'E';  // Equal
    else return 'G';  // Greater
}

void agent::processVirtual(infon* v){
    infon *args=v->spec1, *spec=v->spec2, *parent=getTop(v); int EOT=0; UInt vSize=(UInt)v->size;
    char posArea=(v->pFlag&(fUnknown<<goSize))?'?':isPosLorEorGtoSize(vSize, parent);
    if(posArea=='G'){std::cout << "EXTRA ITEM ALERT!\n"; closeListAtItem(v); return;}
    UInt tmpFlags=v->pFlag&0xff000000;  // TODO B4: move this flag stuff into deepCopy. Clean the following block.
    if (spec){
        if((spec->wFlag&mFindMode)==iAssocNxt) {
            copyTo(spec, v); v->pFlag=((fUnknown+tNum)<<goSize)+fUnknown; v->spec1=spec->spec2;}
        else {deepCopy(spec, v,0,0,1); }
        if(posArea=='?' && v->spec1) posArea= 'N';
    }
    v->pFlag|=tmpFlags;  // TODO B4: move this flag stuff into deepCopy.
    v->pFlag&=~isVirtual;
    if(EOT){ if(posArea=='?'){posArea='E'; closeListAtItem(v);}
        else if(posArea!='E') throw "List was too short";}
    if (posArea=='E') {v->pFlag|=isBottom+isLast; return;}
    infon* tmp= new infon;  tmp->size=(infon*)(vSize+1); tmp->spec2=spec;
    tmp->pFlag|=fUnknown+isBottom+isVirtual+(tNum<<goSize); tmp->wFlag|=iNone;
    tmp->top=tmp->next=v->next; v->next=tmp; tmp->prev=v; tmp->next->prev=tmp; tmp->spec1=args;
    v->pFlag&=~isBottom;
    if (posArea=='?'){ v->pFlag|=isTentative;}
}

void agent::InitList(infon* item) { // TODO: fix: what if this is called twice?
    infon* tmp;
    if(item->value && (((tmp=item->value->prev)->pFlag)&isVirtual)){
        tmp->spec2=item->spec2;
        if(tmp->spec2 && ((tmp->spec2->pFlag&mRepMode)==asFunc))
                StartTerm(tmp->spec2->spec1, &tmp->spec1);
        processVirtual(tmp); item->pFlag|=tList;
    }
}

int agent::getFollower(infon** lval, infon* i){
    int levels=0;
    gnsTop: if(i->next==0) {*lval=0; return levels;}
 /*  infon* size;
     if((i->next->pFlag&isVirtual) && i->spec2 && i->spec2->prev==(infon*)2){
        i->pFlag|=(isLast+isBottom);
        size=i->next->size-1;
        i->next->next->prev=i; i->next=i->next->next;// TODO when working on mem mngr: i->next is dangling
        i=getTop(i); i->size=size; i->pFlag&= ~fLoop; ++levels;
        goto gnsTop;
    }*/
    if(i->pFlag&isLast && i->prev){
        i=getTop(i); ++levels;
        if(i) goto gnsTop;
        else {*lval=0; return levels;}
    }
    *lval=i; getNextTerm(lval);
    if((*lval)->pFlag&isVirtual) processVirtual(*lval);
    return levels;
}

void agent::addIDs(infon* Lvals, infon* Rvals, int asAlt){
    const int maxAlternates=100;
    if(Lvals==Rvals) return;
    infon* RvlLst[maxAlternates]; infon* crntAlt=Rvals; infon *pred, *Rval, *prev=0; UInt size;
    int altCnt=1; RvlLst[0]=crntAlt;
    while(crntAlt && (crntAlt->pFlag&isTentative)){
        getFollower(&crntAlt, getTop(crntAlt));
        RvlLst[altCnt++]=crntAlt;
        if(altCnt>=maxAlternates) throw "Too many nested alternates";
    }
    size=((UInt)Lvals->next->size)-2; crntAlt=Lvals; pred=Lvals->pred;
    while(crntAlt){  // Lvals
        for(int i=0; i<altCnt; ++i){ // Rvals
            if (!asAlt && altCnt==1 && crntAlt==Lvals){
                insertID(&Lvals->wrkList,Rvals,0); recAlts(Lvals,Rvals);
            } else {
                Rval=RvlLst[i];
                AddSizeAlternate(crntAlt, Rval, pred, size, (prev)?prev->prev:0);
            }
        }
        if(crntAlt->pFlag&isTentative) {prev=crntAlt; size=((UInt)crntAlt->next->size)-2; crntAlt=getTop(crntAlt); if(crntAlt) crntAlt->pFlag|=hasAlts;}
        else crntAlt=0;
    }
}

infon* getIdentFromPrev(infon* prev){
// TODO: this may only work in certain cases. It should do a recursive search.
    return prev->wrkList->item->wrkList->item;
}

inline int infonSizeCmp(infon* left, infon* right) { // -1: L<R,  0: L=R, 1: L>R. Infons must have known, numeric sizes
    UInt leftType=left->pFlag&tType, rightType=right->pFlag&tType;
    if(leftType==rightType)
        if (left->size==right->size) return 0;
        if (left->size<right->size) return -1; else return 1;
}

inline infon* getMasterList(infon* item){
    for(infon* findMaster=item; findMaster; findMaster=findMaster->top){
        infon* nxtParent=getTop(findMaster);
       // if(findMaster->next && (findMaster->next->pFlag&isTentative)) return findMaster;
        if(nxtParent && (nxtParent->pFlag&fLoop)) return findMaster;
    }
    return 0;
}

int agent::compute(infon* i){
    infon* p=i->value; int vAcc, sAcc, count=0;
    if(p) do{
        normalize(p); // TODO: appending inline rather than here would allow streaming.
        if((p->pFlag&((tType<<goSize)+tType))==((tNum<<goSize)+tNum)){
            if (p->pFlag&(fUnknown<<goSize)) return 0;
            if (p->pFlag&fConcat) compute(p->size);
            if ((p->pFlag&(tType<<goSize))!=(tNum<<goSize)) return 0;
            if (p->pFlag&fUnknown) return 0;
            if ((p->pFlag&tType)==tList) compute(p->value);
            if ((p->pFlag&tType)!=tNum) return 0;
            int val=(p->pFlag&fInvert)?-(UInt)p->value:(UInt)p->value;
            if(p->pFlag&(fInvert<<goSize)){
                if (++count==1){sAcc=(UInt)p->size; vAcc=val;}
                    else {sAcc/=(UInt)p->size; vAcc=(vAcc/(UInt)p->size)+val;}
                } else {
                if (++count==1){sAcc=(UInt)p->size; vAcc=val;}
                    else {sAcc*=(UInt)p->size; vAcc=(vAcc*(UInt)p->size)+val;}
                    }
            } else return 0;
        p=p->next;
    } while (p!=i->value);
    i->value=(infon*)vAcc; i->size=(infon*)sAcc;
    i->pFlag=(i->pFlag&0xFF00FF00)+(tNum<<goSize)+tNum;
    return 1;
}

void resolve(infon* i, infon* theOne){ std::cout<<"RESOLVING";
    infon *prev=0;
    while(i && theOne){
        if(theOne->pFlag&isTentative){
            closeListAtItem(theOne); std::cout<<"-CLOSED\n";
            return;
        } else {
            i->size=theOne->size; i->value=theOne->value; i->pFlag&=~hasAlts; cpType(theOne,i);
            if((theOne->pFlag&tType)==tList) {infon* itm=i->value; for (UInt p=(UInt)i->size; p; --p) {itm->pFlag&=~isTentative; itm=itm->next;}}
            prev=i; i=i->pred; theOne=theOne->pred;
        }
    } if (theOne){theOne->next=prev->value; theOne->pFlag|=isLast+isBottom;}
    std::cout<<"-OPENED:"<<printInfon (i)<<"\n";
}

#define SetBypassDeadEnd() {result=BypassDeadEnd; infon* CA=getTop(ci); if (CA) {std::cout<< printInfon(CA) <<'>'; AddSizeAlternate(CA, item, 0, ((UInt)ci->next->size)-1, ci); }}

enum WorkItemResults {DoNothing, BypassDeadEnd, DoNext, DoNextIf};
int agent::doWorkList(infon* ci, infon* CIfol, int asAlt){
    infNode *wrkNode=ci->wrkList; infon *item, *IDfol, *tmp, *theOne=0;
    UInt altCount=0, cSize, tempRes, isIndef=0, result=DoNothing, f, matchType;
    if(CIfol && !CIfol->pred) CIfol->pred=ci;
    if(wrkNode)do{
        wrkNode=wrkNode->next; item=wrkNode->item; IDfol=(infon*)1;
        if (wrkNode->idFlags&skipFollower) CIfol=0;
        switch (wrkNode->idFlags&WorkType){
        case InitSearchList: // e.g., {[....]|...}::={3 4 5 6}
            tmp=new infon();
            tmp->top2=ci; tmp->value=ci->value->spec1;
            {tmp->size=ci->size; cpFlags(ci,tmp,0xff0000);} tmp->pFlag|=(fLoop+fUnknown+tList);
            insertID(&tmp->wrkList, item, MergeIdent);  // apending the {3 4 5 6}
            tmp->value->next = tmp->value->prev = tmp->value;
            tmp->value->pFlag|=(isTop+isBottom+isFirst+isVirtual);
            tmp->value->top=tmp;
            insertID(&tmp->value->wrkList, item->value, MergeIdent);
            processVirtual (tmp->value); tmp->value->next->size=(infon*)2;
            result=DoNext;
            break;
        case ProcessAlternatives:
            if (altCount>=1) break; // don't keep looking after found
            if(wrkNode->idFlags&isRawFlag){
              tmp=new infon(ci->pFlag,ci->wFlag,ci->size,ci->value,0,ci->spec1,ci->spec2,ci->next);
              tmp->prev=ci->prev; tmp->top=ci->top; tmp->type=ci->type;
              insertID(&tmp->wrkList, item,0);
              wrkNode->item=item=tmp;
              wrkNode->idFlags^=isRawFlag;
            }
            wrkNode->idFlags|=NodeDoneFlag;
            tempRes=doWorkList(item, CIfol,1); if(result<tempRes) result=tempRes;
            if (tempRes==BypassDeadEnd) {wrkNode->idFlags|=NoMatch; break;}
        case (ProcessAlternatives+NodeDoneFlag):
            altCount++;  theOne=item;
            break;
        case MergeIdent:
            wrkNode->idFlags|=SetComplete;
            UInt fm=item->wFlag&mFindMode;
            UInt CIsType=ci->pFlag&tType, ItemsType=item->pFlag&tType;
            matchType=wrkNode->idFlags&mMatchType;
            if(!(item->pFlag&isNormed) && (fm!=iNone || (item->wrkList && !(item->pFlag&isNormed)))) {
                if(item->top==0) {item->top=ci->top;}
                normalize(item);
                if (item->wFlag&mAsProxie) {item=item->value;fm=item->wFlag&mFindMode;}
                ItemsType=item->pFlag&tType;
            }
            if(CIsType==tUnknown) {
                ci->pFlag|=((item->pFlag&tType)+(tNum<<goSize)+sizeIndef); isIndef=1;
                ci->size=item->size;if (!(item->pFlag&(fUnknown<<goSize))) ci->pFlag&=~(fUnknown<<goSize);
                CIsType=ci->pFlag&tType;
            } else isIndef=0;
            if (item->pFlag&fConcat) std::cout << "WARNING: Trying to merge a concatenation.\n";
            if (matchType) {
                if (CIsType!= ItemsType){result=BypassDeadEnd; std::cout << "Non-matching base types\n";}
                else if(ci->type && (item->type==0 || !(ci->type==item->type))){result=BypassDeadEnd; std::cout << "Non-matching model types\n";}
            }else switch(CIsType+4*ItemsType){
                case tUnknown+4*tUnknown: case tNum+4*tUnknown: case tString+4*tUnknown:
                case tList+4*tList:
                case tString+4*tString:
                case tNum+4*tNum:
                    result=DoNext;
                    if (!isIndef && ci->pFlag&sizeIndef && ((CIsType+4*ItemsType)== tString+4*tString)){ // When alts are evaluated (e.g., aaron, aron) use this secton.
                        if(infonSizeCmp(ci,item)<0) {result=BypassDeadEnd; break;}
                        if (memcmp(ci->value, item->value, (UInt)item->size)!=0)  {result=BypassDeadEnd;break;}
                        ci->size=item->size;
                        if(CIfol){
                            getFollower(&IDfol, item);
                            if(IDfol) addIDs(CIfol, IDfol, asAlt); else {SetBypassDeadEnd(); break;}
                        }
                        break;
                    }
                    if(ItemsType!=tUnknown){
                        if (ItemsType==tList) InitList(item);
                        UInt flagMask=0;
                        // MERGE SIZES
                        if(!(item->pFlag&(fUnknown<<goSize))){ // if item's size is known, copy size // TODO: What if SU, US, etc.?
                            if(ci->pFlag&(fUnknown<<goSize)) {ci->size=item->size;  flagMask|=(0xff<<goSize); }  // TODO: Verify this flag mask is correct.
                            else if (infonSizeCmp(ci,item)!=0 &&  ((CIsType+4*ItemsType)!= tList+4*tList) && ((CIsType+4*ItemsType)!= tString+4*tString)){
                                SetBypassDeadEnd(); std::cout<<"Sizes contradict\n"; break;
                            }
                        }

                        // MERGE VALUES
                        if(!(item->pFlag&fUnknown)){

                            if ((CIsType+4*ItemsType)== tNum+4*tNum){
                                if(ci->pFlag&fUnknown) {ci->value=item->value; flagMask|=0xff;}
                                else if (ci->value!=item->value){SetBypassDeadEnd(); std::cout<<"Values contradict\n"; break;}
                            }

                            else if ((CIsType+4*ItemsType)== tList+4*tList){
                                if(ci->value) {addIDs(ci->value, item->value, asAlt); flagMask=0;} // TODO: when flags are correct in Merge Size, remove flagMask=0 here.
                                else{ci->value=item->value; cpFlags(item,ci,0xff);item->value->top=ci;}
                            }

                            else if ((CIsType+4*ItemsType)== tString+4*tString){
                                if(infonSizeCmp(ci,item)>0) {SetBypassDeadEnd(); break;}
                                if(ci->pFlag&fUnknown){ci->value=item->value; ci->pFlag&=~fUnknown;}
                                else if (memcmp(ci->value, item->value,  (UInt)ci->size)!=0) {SetBypassDeadEnd(); break;}
                            }
                        }
                        if(flagMask) {cpFlags(item,ci,(flagMask+0xff00));}
                    }
                    isIndef=0;
                    if(CIfol){
                        if(infonSizeCmp(ci,item)==0 || ((CIsType+4*ItemsType)!= tString+4*tString)){
                            int level=getFollower(&IDfol, item);
                            if(IDfol){if(level==0) addIDs(CIfol, IDfol, asAlt); else result=BypassDeadEnd;}
                            else  if( ((CIsType+4*ItemsType)!= tList+4*tList)) {// temporaty hack
                                if ((tmp=ci->isntLast()) && (tmp->next->pFlag&isTentative)) {
                                    SetBypassDeadEnd();
                                }
                                if((tmp=getMasterList(ci) )){closeListAtItem(tmp); result=BypassDeadEnd; }
                            }
                        } else if(((CIsType+4*ItemsType)== tString+4*tString) && infonSizeCmp(ci,item)<0){
                            cSize=(UInt)ci->size;
                            tmp=new infon(item->pFlag,item->wFlag,(infon*)((UInt)item->size-cSize),(infon*)((UInt)item->value+cSize),0,0,item->next);
                            addIDs(CIfol, tmp, asAlt);
                        }
                    }
                break;
                case tNum+4*tString: result=DoNext; break;
                case tString+4*tNum: result=DoNext;break;
                case tString+4*tList: InitList(item);  result=DoNext; break;
                case tNum+4*tList:   InitList(item); addIDs(ci,item->value,asAlt); result=DoNext;break; // This should probably be insertID not addIDs. Check it out.
                case tList+4*tUnknown:
                case tList+4*tString:
                case tList+4*tNum:
                    if(ci->value){addIDs(ci->value, item, asAlt);}
                    else{copyTo(item, ci);}
                    result=DoNext;
            }
            break;
        }
    }while (wrkNode!=ci->wrkList); else result=(ci->next && (ci->pFlag&isTentative))?DoNextIf:DoNext;
    if(altCount==1){
            for (f=1, tmp=getTop(ci); tmp!=0; tmp=getTop(tmp)) // check ancestors for alts
               if (tmp->pFlag&hasAlts) {f=0; break;}
            if(f) resolve(ci, theOne);
    }
    ci->pFlag|=isNormed; ci->pFlag&=~(sizeIndef+isTentative);
    return result;
}

void agent::preNormalize(infon* CI, Qitem *cn){
    infon *newID; int CIFindMode;
    while ((CIFindMode=(CI->wFlag&mFindMode))){
        newID=0;
        if(CI->pFlag&toExec) cn->override=1;
        if(CI->pFlag&asDesc) {if(cn->override) cn->override=0; else break;}
        switch(CI->wFlag&mSeed){
            case sUseAsFirst: cn->firstID=CI->spec2; cn->IDStatus=1; CI->wFlag&=~mSeed; break;
            case sUseAsList: CI->spec1->pFlag|=toExec;  newID=CI->spec1;
        }
        switch(CIFindMode){
            case iToWorld: copyTo(world, CI); break;
            case iToCtxt:   copyTo(&context, CI); break; // TODO: Search agent::context
            case iToArgs: case iToVars:
                for (newID=CI->top; newID && !(newID->top->wFlag&mIsHeadOfGetLast); newID=newID->top){}
                if(newID && CIFindMode==iToVars) newID=newID->next;
                if(newID) {CI->wFlag|=mAsProxie; CI->value=newID; newID->pFlag|=isNormed; CI->wFlag&=~mFindMode; newID=0;}
                cn->doShortNorm=true;
                break;
            case iAssocNxt:
                newID=CI->spec2->spec2;
                getNextNormal(&newID);
                break;
            case iToPath: //  Handle ^, \^, \\^, \\\^, etc.
            case iToPathH:{  //  Handle \, \\, \\\, etc. Path with 'Home'
                newID=CI->top;
                for(int s=1; s<(UInt)CI->spec1; ++s) {  // for  backslashes-1 go to parent
                    newID=getTop(newID); if (newID==0 ) {std::cout << "Too many '\\'s in "<<printInfon(CI)<< '\n';}
                }
                if(CIFindMode==iToPathH) {  // If no '^', move to first item in list.
                    if(!(newID->pFlag&isTop)) {newID=newID->top; }
                    if (newID==0) std::cout<<"Zero TOP in "<< printInfon(CI)<<'\n';
                    if(!(newID->pFlag&isFirst)) {newID=0; std::cout<<"Top but not First in "<< printInfon(CI)<<'\n';}
                }
                if(newID) {CI->wFlag|=mAsProxie; CI->value=newID; newID->pFlag|=isNormed; CI->wFlag&=~mFindMode; newID=0;}
                cn->doShortNorm=true;
            } break;
            case iTagDef: {
                caseDown(CI->type); //std::cout<<"Defining:'"<<(char*)CI->type->S<<"'\n";
                std::map<stng,infon*>::iterator tagPtr=tag2Ptr.find(*CI->type);
                if (tagPtr==tag2Ptr.end()) {tag2Ptr[*CI->type]=CI->wrkList->item; CI->wrkList=0;}
                else{throw("A tag is being redefined, which isn't allowed");}
                CI->wrkList=0; CI->wFlag=0;
                break;}
            case iTagUse: {
                caseDown(CI->type); //OUT("Recalling: "<<(char*)CI->type->S)
                std::map<stng,infon*>::iterator tagPtr=tag2Ptr.find(*CI->type);
                if (tagPtr!=tag2Ptr.end()) {UInt tmpFlags=CI->pFlag&0xff000000; deepCopy(tagPtr->second,CI); CI->pFlag|=tmpFlags; deTagWrkList(CI);} // TODO B4: move this flag stuff into deepCopy.
                else{OUT("\nBad tag:'"<<(char*)(CI->type->S)<<"'\n");throw("A tag was used but never defined");}
                break;}
            case iGetFirst:      StartTerm (CI, &newID); break;
            case iGetMiddle:  break; // TODO: iGetMiddle
            case iGetLast:
                if ((CI->spec1->pFlag)&(fInvert<<goSize)){ // TODO: This block is a hack to make simple backward references work. Fix for full back-parsing.
                    newID=CI;
                    for(UInt i=(UInt)CI->spec1->size; i>0; --i){newID=newID->prev;}
                    {insertID(&CI->wrkList, newID,0); if(CI->pFlag&fUnknown) {CI->size=newID->size;} cpFlags(newID, CI,0x00ffffff);}
                    CI->pFlag|=fUnknown;
                } else{
                    normalize(CI->spec1, cn->firstID);
                    if( ! CI->spec1->isntLast ()) CI->pFlag|=(isBottom+isLast);
                    if(CI->spec1->pFlag&hasAlts) {  // migrate alternates from spec1 to CI... Later, build this into LastTerm.
                        infNode *wrkNode=CI->spec1->wrkList; infon* item=0;
                        if(wrkNode)do{
                            wrkNode=wrkNode->next; item=wrkNode->item;
                            if(((wrkNode->idFlags&(ProcessAlternatives+NoMatch))==ProcessAlternatives)&& (item->wFlag&0x2000)){
                                if(wrkNode->item->size==0){} // TODO: handle the null alternative
                                else {insertID(&CI->wrkList,wrkNode->slot,ProcessAlternatives+isRawFlag);}
                            }
                        }while (wrkNode!=CI->spec1->wrkList);
                    }
                    else LastTerm(CI->spec1, &newID);
                }
                CI->pFlag|=fUnknown;
                break;
            case iGetSize:     break; // TODO: iGetSize
            case iGetType:     break; // TODO: iGetType
            case iHardFunc: autoEval(CI, this); cn->firstID=CI->spec2; break;
            case iNone: default: throw "Invalid Find Mode";
        }
        if(newID){
            if(CI->wFlag&mAssoc)
                {newID=newID->wrkList->item; CIFindMode=iAssocNxt;} // Transition assoc modes
            if(CIFindMode==iAssocNxt){
                CI->spec2->spec2=newID; // Remember this so later we can fetch the next assoc.
                infon* masterItem = 0;
                if(!(getTop(newID)->pFlag&(fUnknown<<goSize))) {
                    masterItem=getMasterList(CI);
                    masterItem->pFlag &= ~isTentative;
                }
                if(!newID->isntLast()){
                    closeListAtItem((masterItem)?masterItem : getMasterList(CI));
                }
            }
            insertID(&CI->wrkList, newID,(CIFindMode>=iAssocNxt)?skipFollower:0);
            CI->pFlag&=~tType; CI->wFlag&=~mFindMode;
        }
    }
}

#define pushCIsFollower {int lvl=cn.level-cn.nxtLvl; if(lvl>0) ItmQ.push(Qitem(cn.CIfol,0,0,lvl,cn.bufCnt));}

infon* agent::Normalize(infon* i, infon* firstID){
    infon *CI;
    if (i==0) return 0;
    infQ ItmQ; ItmQ.push(Qitem(i,firstID,(firstID)?1:0,0));
    while (!ItmQ.empty()){
        Qitem cn=ItmQ.front(); ItmQ.pop(); CI=cn.item; cn.doShortNorm=0;
        preNormalize(CI, &cn);
        if (cn.doShortNorm) return 0;
        if((CI->wFlag&mFindMode)==0 && cn.IDStatus==2)
            {cn.IDStatus=0; insertID(&CI->wrkList,cn.firstID,0);}
        if((CI->pFlag&tType)==tList){InitList(CI);}
        cn.nxtLvl=getFollower(&cn.CIfol, CI);
        if((CI->pFlag&asDesc) && !cn.override) cn.whatNext=DoNext;//{pushCIsFollower; continue;}
        else cn.whatNext=doWorkList(CI, cn.CIfol);
        switch (cn.whatNext) {
        case DoNextIf: if(++cn.bufCnt>=ListBuffCutoff){cn.nxtLvl=getFollower(&cn.CIfol,getTop(CI))+1; pushCIsFollower; break;}
        case DoNext:
            if((CI->pFlag&(fConcat+tType))==(fConcat+tNum)){
                compute(CI); if(cn.CIfol && !(CI->pFlag&isLast)){pushCIsFollower;}
            }else if(!((CI->pFlag&asDesc)&&!cn.override)&&((CI->value&&((CI->pFlag&tType)==tList))||(CI->pFlag&fConcat)) && !(CI->value->pFlag&isNormed)){
                ItmQ.push(Qitem(CI->value,cn.firstID,((cn.IDStatus==1)&!(CI->pFlag&fConcat))?2:cn.IDStatus,cn.level+1)); // push CI->value
            }else if (cn.CIfol){pushCIsFollower;}
            break;
        case BypassDeadEnd: {cn.nxtLvl=getFollower(&cn.CIfol,getTop(CI))+1; pushCIsFollower;} break;
        case DoNothing: break;
        }
    }
    return (i->pFlag&isNormed)?i:0;
};

fetchData* initFetch(fetchData *data){
    fetchData* FVar=0;
/*    if(!data){
        FVar=new fetchData;
        FVar->state1=FVar->state2=0;
    }
    FVar->refcount++;*/
    return FVar;
}

void releaseFetch(fetchData *data){
 //   data->refcount--;
  //  if(data->refcount==0) delete(data);
}

int agent::fetch_NodesNormalForm(int operation, Qitem &cn){
    switch(operation){
    case opRequest: cn.fetchVars=initFetch((fetchData*)&cn); break;
    case opTry:{
        cn.CI=cn.item; cn.doShortNorm=0;
        preNormalize(cn.CI, &cn);
        if (cn.doShortNorm) return 0;
        if((cn.CI->wFlag&mFindMode)==0 && cn.IDStatus==2)
            {cn.IDStatus=0; insertID(&cn.CI->wrkList,cn.firstID,0);}
        if((cn.CI->pFlag&tType)==tList){InitList(cn.CI);}
        cn.nxtLvl=getFollower(&cn.CIfol, cn.CI);
        if((cn.CI->pFlag&asDesc) && !cn.override) {cn.whatNext=DoNext;}
        else cn.whatNext=doWorkList(cn.CI, cn.CIfol);
        } break;
    case opTakeHint:
    case opPing:
    case opRefcount:
    case opHearError:
    case opThanks: releaseFetch((fetchData*)&cn);
    default:;
    }
    return 0;
}

int agent::fetch_NormalForm(int operation, normData *data){
    switch(operation){
    case opRequest: {data->fetchVars=initFetch((fetchData*)data); data->level=0; /* check for item==0 */ break;}
    case opTry:{
        infon *CI;
        if (data->item==0) return 0;
        infQ ItmQ; ItmQ.push(Qitem(data->item,data->firstID,(data->firstID)?1:0,0));
        while (!ItmQ.empty()){
            Qitem cn=ItmQ.front(); ItmQ.pop(); CI=cn.item;
            fetch_NodesNormalForm(opRequest, cn);
            fetch_NodesNormalForm(opTry, cn);
            //wait?
            fetch_NodesNormalForm(opThanks, cn);
            switch (cn.whatNext) {
            case DoNextIf: if(++cn.bufCnt>=ListBuffCutoff){cn.nxtLvl=getFollower(&cn.CIfol,getTop(CI))+1; pushCIsFollower; break;}
            case DoNext:
                if((CI->pFlag&(fConcat+tType))==(fConcat+tNum)){
                    compute(CI); if(cn.CIfol && !(CI->pFlag&isLast)){pushCIsFollower;}
                }else if(!((CI->pFlag&asDesc)&&!cn.override)&&((CI->value&&((CI->pFlag&tType)==tList))||(CI->pFlag&fConcat)) && !(CI->value->pFlag&isNormed)){
                    ItmQ.push(Qitem(CI->value,cn.firstID,((cn.IDStatus==1)&!(CI->pFlag&fConcat))?2:cn.IDStatus,cn.level+1)); // push CI->value
                }else if (cn.CIfol){pushCIsFollower;}
                break;
            case BypassDeadEnd: {cn.nxtLvl=getFollower(&cn.CIfol,getTop(CI))+1; pushCIsFollower;} break;
            case DoNothing: break;
            }
        }
    } break;
    case opTakeHint:
    case opPing:
    case opRefcount:
    case opHearError:
    case opThanks: releaseFetch((fetchData*)data);
    default:;
    }
    return 0;
}

infon* agent::normalize(infon* i, infon* firstID){
    normData *data=new normData;
    data->item=i; data->firstID=firstID;
    fetch_NormalForm(opRequest, data);
    fetch_NormalForm(opTry, data);
    //wait?
    fetch_NormalForm(opThanks, data);
    delete(data);
    return (i->pFlag&isNormed)?i:0;
}

