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

const int ListBuffCutoff=2;

typedef std::map<dblPtr,UInt>::iterator altIter;
std::map<Tag,infon*> tag2Ptr;
std::map<infon*,Tag> ptr2Tag;

#define recAlts(lval, rval) {if((rval->pFlag&tType)==tString) alts[dblPtr((char*)rval->value,lval)]++;}
#define getTop(item) (((item->pFlag&isTop)||item->top==0)? item->top : item->top->top)
#define fetchLastItem(lval, item) {for(lval=item;(lval->pFlag&tType)==tList;lval=lval->value->prev);}
#define fetchFirstItem(lval, item) {for(lval=item;(lval->pFlag&tType)==tList;lval=lval->value){};}
#define cpType(from, to) {if(to->type==0) to->type=from->type;}
#define cpFlags(from, to, mask) {to->pFlag=(to->pFlag& ~(mask))+((from)->pFlag&mask); to->wFlag=from->wFlag; cpType(from,to);}
#define copyTo(from, to) {if(from!=to){to->size=(from)->size; to->value=(from)->value; cpFlags((from),to,0x00ffffff);}}

infon* infon::isntLast(){ // 0=this is the last one. >0 = pointer to predecessor of the next one.
    if (!(pFlag&isLast)) return this;
    infon* parent=getTop(this); infon* gParent=getTop(parent);
    if(gParent && ValueIsConcat(gParent))
        return parent->isntLast();
    return 0;
}

int agent::loadInfon(const char* filename, infon** inf, bool normIt){
    std::cout<<"Loading:'"<<filename<<"'..."<<std::flush;
    std::fstream InfonIn(filename);
    if(InfonIn.fail()){std::cout<<"Error: The file "<<filename<<" was not found.\n"<<std::flush; return 1;}
    QParser T(InfonIn); T.locale=locale;
    *inf=T.parse();
    if (*inf) {std::cout<<"done.   "<<std::flush;}
    else {std::cout<<"Error:"<<T.buf<<"   "<<std::flush; return 1;}
    if(normIt) {
        alts.clear();
        try{
            std::cout<<"Normalizing..."<<std::flush; normalize(*inf); std::cout << "Normalized."<<std::flush;
        } catch (char const* errMsg){std::cout<<errMsg;}
    }
    std::cout<<"\n";
    return 0;
}

int agent::StartTerm(infon* varIn, infon** varOut) {
  infon* tmp;
  if (varIn==0) return 1;
  int varInFlags=varIn->pFlag;
  if (FormatIsConcat(varInFlags)){
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
  if (ValueIsConcat(varIn)) {varIn=varIn->value->prev;}
  else if((varIn->pFlag&tType)!=tList)  return 2;
  else {*varOut=varIn->value->prev; if (*varOut==0) return 3;}
  return 0;
}

int agent::getNextTerm(infon** p) {
  infon *parent, *Gparent=0, *GGparent=0;
  if((*p)->pFlag&isBottom) {
    parent=((*p)->pFlag&isFirst)?(*p)->top:(*p)->top->top;
    if ((*p)->pFlag&isLast){
      if(parent==0){(*p)=(*p)->next; return 1;}
      if(parent->top) Gparent=(parent->pFlag&isFirst)?parent->top:parent->top->top;
      if(Gparent && ValueIsConcat(Gparent)) {
        if(Gparent->top) GGparent=(Gparent->pFlag&isFirst)?Gparent->top:Gparent->top->top;
        do {
          if(getNextTerm(&parent)) return 4;
// TODO: the next line fixes one problem but causes another: nested list-concats.
// test it with:  (   ({} {"X", "Y"} ({} {9,8} {7,6}) {5} ) {1} {{}} {2, 3, 4} )
          if(GGparent && ValueIsConcat(GGparent)) {*p=parent; return 0;}
          switch(parent->pFlag&tType){
            case tUnknown: (*p)=0; return -1;
            case tNum: case tString: throw("Item in list concat is not a list.");
            case tList: if(!StartTerm(parent, p)) return 0;
            }
        } while (1);
      }
      return 2;
    } else {infon* slug=new infon; slug->wFlag|=nsBottomNotLast; append(slug, parent); (*p)=slug;} /*Bottom but not end, make slug*/
  }else {
    (*p)=(*p)->next;
    if ((*p)==0) {return 3;}
    if ((*p)->pFlag&isLast){     // If isLast && Parent is spec1-of-get-last, get parent, apply any idents
        parent=getTop((*p));
        if(parent->wFlag&mIsHeadOfGetLast) {
            parent=parent->top2;
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
    if(((*ItmPtr)->pFlag&tType)==tString && ValueIsKnown(*ItmPtr)) {
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
infon* agent::append(infon* i, infon* list){ // appends an item to the end of a tentative list
    if(! (list && list->value && list->value->prev)) return 0;
    infon *last=list->value->prev;
    infon *p=last, *q;
    do {  // find the earliest-but-at-end tentative in list
        q=p; p=p->prev; // Shift left in list
    } while(p!=last && (p->pFlag&(isVirtual+isTentative)));
    if(!(q && (q->pFlag&(isVirtual+isTentative)))) return 0;
    if(q->pFlag&isVirtual) processVirtual(q);
    copyTo(i, q); if(((q->pFlag&tType)==tList) && q->size) q->value->top=q;
    q->spec1=i->spec1; q->spec2=i->spec2; q->wrkList=i->wrkList;
    q->pFlag&= ~(isVirtual+isTentative);
    normalize(q);
    return q;
}

infon* agent::copyList(infon* from, int flags){
    infon* top=0; infon* follower; infon* q, *p=from;
    if(from==0)return 0;
    do {
        q=new infon;
        deepCopy(p,q, 0, flags); q->pos=p->pos;
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
            i->type=0;  // This removes the reference to the old type to avoid a false strict-type mismatch.
            q->idFlags-=c1Left;
            p->next=q->next;
            appendID(&i->spec1->wrkList, q);
            if(p==q) {i->wrkList=0; break;}
        }
        p=p->next;
    } while (p!=i->wrkList);
}

void agent::deepCopy(infon* from, infon* to, PtrMap* ptrs, int flags){
    UInt fm=from->wFlag&mFindMode; infon* tmp;
    to->pFlag=(from->pFlag&0x0fffffff)/*|(to->pFlag&0xff000000)*/; to->wFlag=from->wFlag;
    if(to->type==0) to->type=from->type; // TODO: We should merge types, not just copy over.

    if(((from->pFlag>>goSize)&tType)==tList || SizeIsConcat(from)){to->size=copyList(from->size, flags); if(to->size)to->size->top=to;}
    else to->size=from->size;

    if((from->pFlag&tType)==tList || ValueIsConcat(from)){to->value=copyList(from->value, flags); if(to->value)to->value->top=to;}
    else to->value=from->value;
    if(from->wFlag&mIsHeadOfGetLast) to->top2=(*ptrs)[from->top2];
    if(fm==iToPath || fm==iToPathH || fm==iToArgs || fm==iToVars) {
        to->spec1=from->spec1;
        if (ptrs) to->top=(*ptrs)[from->top];
    } else if((fm==iGetFirst || fm==iGetLast || fm==iGetMiddle) && from->spec1) {
        PtrMap *ptrMap = (ptrs)?ptrs:new PtrMap;
        if ((to->prev) && (tmp=to->prev->spec1) && (tmp->wFlag&mIsHeadOfGetLast) && (tmp=tmp->next)) to->spec1=tmp;
        else to->spec1=new infon;
        (*ptrMap)[from]=to;
        deepCopy (from->spec1, to->spec1, ptrMap, flags);
        if(ptrs==0) delete ptrMap;
        if (flags && from->wFlag&mAssoc) {// std::cout<<"Initializing ASSOC\n";
            from->wFlag&= ~(mAssoc+mFindMode); from->wFlag|=iAssocNxt;
            from->pFlag&= ~(mFormat+(mFormat<<goSize)); from->pFlag|=(FUnknown+(FUnknown<<goSize));
        }
    }

    if ((from->wFlag&mFindMode)==iAssocNxt)
        {to->spec2=from;}  // Breadcrumbs to later find next associated item.
    else if(!from->spec2) to->spec2=0;
    else {to->spec2=new infon; deepCopy(from->spec2, to->spec2,0,flags);}

    infNode *p=from->wrkList, *q;  // Merge Identity Lists
    if(p) do {
        q=new infNode; q->idFlags=p->idFlags;
        if((p->item->wFlag&mFindMode)==iNone || true) {q->item=new infon; q->idFlags=p->idFlags; deepCopy (p->item, q->item, ptrs);}
        else {q->item=p->item;}
        prependID(&to->wrkList, q);
        p=p->next;
    } while (p!=from->wrkList);
}

void recover(infon* i){ delete i;}

void closeListAtItem(infon* lastItem){ // remove (no-longer valid) items to the right of lastItem in a list.
//std::cout << "CLOSING: " << printInfon(lastItem) << "\n";
    // TODO: Will this work when a list becomes empty?
    infon *itemAfterLast, *nextItem; UInt count=0;
    for(itemAfterLast=lastItem->next; !(itemAfterLast->pFlag&isTop); itemAfterLast=nextItem){
        nextItem=itemAfterLast->next;
        recover(itemAfterLast);
    }
    lastItem->next=itemAfterLast; itemAfterLast->prev=lastItem; lastItem->pFlag|=(isBottom+isLast);
    // The lines below remove any tentative flags then set parent's size
    do{
        itemAfterLast->pFlag &=~ isTentative;
        itemAfterLast=itemAfterLast->next;
        count++;
    } while (!(itemAfterLast->pFlag&isTop));
    itemAfterLast->top->size=(infon*)count;
    SetSizeFormat(itemAfterLast, fLiteral); itemAfterLast->pFlag|=(tNum<<goSize);
}

char isPosLorEorGtoSize(UInt pos, infon* item){
    if(SizeIsUnknown(item)) return '?';
    if(((item->pFlag>>goSize)&tType)!=tNum) throw "Size as non integer is probably not useful, so not allowed.";
    if(SizeIsConcat(item)){ // TODO: apparently no tests call this block. related to the Range bug.
        infon* size=item->size;
        //if ((size->wFlag&mFindMode)!=iNone) normalize(size);  // TODO: Verify do we need this?
        if (IsSimpleUnknownNum(size))
            if(size->next!=size && size->next==size->prev && (IsSimpleUnknownNum(size->next))){
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
    infon *args=v->spec1, *spec=v->spec2, *parent=getTop(v); int EOT=0;
    char posArea=(SizeIsUnknown(v))?'?':isPosLorEorGtoSize(v->pos, parent);
    if(posArea=='G'){std::cout << "EXTRA ITEM ALERT!\n"; closeListAtItem(v); return;}
    UInt tmpFlags=v->pFlag&mListPos;  // TODO B4: move this flag stuff into deepCopy. Clean the following block.
    if (spec){
        if((spec->wFlag&mFindMode)==iAssocNxt) {
            copyTo(spec, v); v->pFlag=((FUnknown+tNum)<<goSize)+FUnknown; v->spec1=spec->spec2;}
        else {deepCopy(spec, v,0,1); }
        if(posArea=='?' && v->spec1) posArea= 'N';
    }
    v->pFlag|=tmpFlags;  // TODO B4: move this flag stuff into deepCopy.
    v->pFlag&=~isVirtual;
    if(EOT){ if(posArea=='?'){posArea='E'; closeListAtItem(v);}
        else if(posArea!='E') throw "List was too short";}
    if (posArea=='E') {v->pFlag|=isBottom+isLast; return;}
    infon* tmp= new infon; tmp->pos=(v->pos+1); tmp->spec2=spec;
    tmp->pFlag|=FUnknown+isBottom+isVirtual+(tNum<<goSize); tmp->wFlag|=iNone;
    tmp->top=tmp->next=v->next; v->next=tmp; tmp->prev=v; tmp->next->prev=tmp; tmp->spec1=args;
    v->pFlag&=~isBottom;
    if (posArea=='?'){ v->pFlag|=isTentative;}
}

void agent::InitList(infon* item) {
    infon* tmp;
    if(!(item->wFlag&nsListInited) && item->value && (((tmp=item->value->prev)->pFlag)&isVirtual)){
//std::cout<<"INITLIST ("<<item<<")\n";
        item->wFlag|=nsListInited;
        tmp->spec2=item->spec2;
    //    if(tmp->spec2 && ((tmp->spec2->pFlag&mRepMode)==asFunc)) // Remove this after testing an argument as a function
    //            StartTerm(tmp->spec2->spec1, &tmp->spec1);
        processVirtual(tmp); item->pFlag|=tList;
    }
}

int agent::getFollower(infon** lval, infon* i){
    int levels=0;
    if(!i) return 0;
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

UInt calcItemsPos(infon* i){
    UInt count;
    if (i->pos) return ((UInt)i->pos)-1;
    for(count=1; !(i->pFlag&isTop); i=i->prev)
        ++count;
    return count;
}

void agent::AddSizeAlternate(infon* Lval, infon* Rval, infon* Pred, UInt Size, infon* Last, UInt Flags) {
    infon *copy, *copy2, *LvalFol;
    copy=new infon(Lval->pFlag,Lval->wFlag,(infon*)(Size),Lval->value,0,Lval->spec1,Lval->spec2,Lval->next);
    copy->prev=Lval->prev; copy->top=Lval->top;copy->pred=Pred; copy->type=Lval->type;
    insertID(&Lval->wrkList,copy,ProcessAlternatives|Flags); Lval->wrkList->slot=Last; Lval->wFlag&=~nsWorkListDone;
    getFollower(&LvalFol, Lval);
    if (LvalFol){
        copy2=new infon(LvalFol->pFlag,LvalFol->wFlag,LvalFol->size,LvalFol->value,0,LvalFol->spec1,LvalFol->spec2,LvalFol->next);
        copy2->top=LvalFol->top; copy2->prev=LvalFol->prev; copy2->type=LvalFol->type; copy2->pred=copy;
        insertID(&copy2->wrkList,Rval,Flags); insertID(&LvalFol->wrkList,copy2,ProcessAlternatives|Flags); LvalFol->wFlag&=~nsWorkListDone;
    }
}

void agent::addIDs(infon* Lvals, infon* Rvals, UInt flags, int asAlt){
    const int maxAlternates=100;
    if(Lvals==Rvals) return;
    if(!(flags&mLooseType)) {insertID(&Lvals->wrkList,Rvals,flags); Lvals->wFlag&=~nsWorkListDone; return;}
    infon* RvlLst[maxAlternates]; infon* crntAlt=Rvals; infon *pred, *Rval, *prev=0; UInt size;
    int altCnt=1; RvlLst[0]=crntAlt;
    while(crntAlt && (crntAlt->pFlag&isTentative)){
        getFollower(&crntAlt, getTop(crntAlt));
        RvlLst[altCnt++]=crntAlt;
        if(altCnt>=maxAlternates) throw "Too many nested alternates";
    }
    size=calcItemsPos(Lvals); crntAlt=Lvals; pred=Lvals->pred;
    while(crntAlt){  // Lvals
        for(int i=0; i<altCnt; ++i){ // Rvals
            if (!asAlt && altCnt==1 && crntAlt==Lvals){
                insertID(&Lvals->wrkList,Rvals,flags); Lvals->wFlag&=~nsWorkListDone; recAlts(Lvals,Rvals);
            } else {
                Rval=RvlLst[i];
                AddSizeAlternate(crntAlt, Rval, pred, size, (prev)?prev->prev:0, flags);
            }
        }
        if(crntAlt->pFlag&isTentative) {prev=crntAlt; size=calcItemsPos(crntAlt); crntAlt=getTop(crntAlt); if(crntAlt) crntAlt->pFlag|=hasAlts;}
        else crntAlt=0;
    }
}

infon* getIdentFromPrev(infon* prev){
// TODO: this may only work in certain cases. It should do a recursive search.
    return prev->wrkList->item->wrkList->item;
}

inline int infonSizeCmp(infon* left, infon* right) { // -1: L<R,  0: L=R, 1: L>R. Infons must have fLiteral, numeric sizes
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

int agent::checkTypeMatch(Tag* LType, Tag* RType){
    return (*LType)==(*RType);
}

int agent::compute(infon* i){
    infon* p=i->value; int vAcc, sAcc, count=0;
    if(p) do{
        normalize(p); // TODO: appending inline rather than here would allow streaming.
        if((p->pFlag&((tType<<goSize)+tType))==((tNum<<goSize)+tNum)){
            if (SizeIsUnknown(p)) return 0;
            if (ValueIsConcat(p)) compute(p->size);
            if ((p->pFlag&(tType<<goSize))!=(tNum<<goSize)) return 0;
            if (ValueIsUnknown(p)) return 0;
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

void resolve(infon* i, infon* theOne){ //std::cout<<"RESOLVING";
    infon *prev=0;
    while(i && theOne){
        if(theOne->pFlag&isTentative){
            closeListAtItem(theOne); //std::cout<<"-CLOSED\n";
            return;
        } else {
            i->pFlag&=~hasAlts; copyTo(theOne, i);
            if((theOne->pFlag&tType)==tList) {infon* itm=i->value; for (UInt p=(UInt)i->size; p; --p) {itm->pFlag&=~isTentative; itm=itm->next;}}
            prev=i; i=i->pred; theOne=theOne->pred;
        }
    } if (theOne){theOne->next=prev->value; theOne->pFlag|=isLast+isBottom;}
 //   std::cout<<"-OPENED:"<<printInfon (i)<<"\n";
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
                    else if(!(newID->pFlag&isFirst)) {newID=0; std::cout<<"Top but not First in "<< printInfon(CI)<<'\n';}
                }
                if(newID) {CI->wFlag|=mAsProxie; CI->value=newID; newID->pFlag|=isNormed; CI->wFlag&=~mFindMode; newID=0;}
                cn->doShortNorm=true;
            } break;
            case iTagDef: break; // Reserved
            case iTagUse: {
                if(CI->type == 0) throw ("A tag was null which is a bug");
                // OUT("Recalling: "<<CI->type->tag<<":"<<CI->type->locale);
                std::map<Tag,infon*>::iterator tagPtr=tag2Ptr.find(*CI->type);
                if (tagPtr!=tag2Ptr.end()) {UInt tmpFlags=CI->pFlag&mListPos; deepCopy(tagPtr->second,CI); CI->pFlag|=tmpFlags; deTagWrkList(CI);} // TODO B4: move this flag stuff into deepCopy.
                else{OUT("\nBad tag:'"<<CI->type->tag<<"'\n");throw("A tag was used but never defined");}
                break;}
            case iGetFirst:      StartTerm (CI, &newID); break;
            case iGetMiddle:  break; // TODO: iGetMiddle
            case iGetLast:
                if ((CI->spec1->pFlag)&(fInvert<<goSize)){ // TODO: This block is a hack to make simple backward references work. Fix for full back-parsing.
                    newID=CI;
                    for(UInt i=(UInt)CI->spec1->size; i>0; --i){newID=newID->prev;}
                    {insertID(&CI->wrkList, newID,0); if(ValueIsUnknown(CI)) {CI->size=newID->size;} cpFlags(newID, CI,~mListPos);}
                    SetValueFormat(CI, FUnknown);
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
                SetValueFormat(CI, FUnknown);
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
                if(SizeIsKnown(getTop(newID))) {
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

#define SetBypassDeadEnd() {result=BypassDeadEnd; infon* CA=getTop(ci); if (CA) {AddSizeAlternate(CA, item, 0, ((UInt)ci->next->pos)-1, ci, looseType); }}

int agent::doWorkList(infon* ci, infon* CIfol, int asAlt){
    infNode *wrkNode=ci->wrkList; infon *item, *IDfol, *tmp, *theOne=0; Qitem cn;
    UInt altCount=0, level, cSize, tempRes, isIndef=0, result=DoNothing, f, looseType, noNewContent=true;
    if(CIfol && !CIfol->pred) CIfol->pred=ci;
    if(wrkNode)do{
        wrkNode=wrkNode->next; item=wrkNode->item;
        if (wrkNode->idFlags&skipFollower) CIfol=0;
        switch (wrkNode->idFlags&WorkType){
        case InitSearchList: // e.g., {[....]|...}::={3 4 5 6}
            tmp=new infon();
            tmp->top2=ci; tmp->value=ci->value->spec1;
            {tmp->size=ci->size; cpFlags(ci,tmp,0xff0000);} tmp->pFlag|=(fLoop+FUnknown+tList); // TODO: Should FUnknown really be here?
            insertID(&tmp->wrkList, item, MergeIdent);  // appending the {3 4 5 6}
            tmp->value->next = tmp->value->prev = tmp->value;
            tmp->value->pFlag|=(isTop+isBottom+isFirst+isVirtual);
            tmp->value->top=tmp;
            insertID(&tmp->value->wrkList, item->value, MergeIdent);
            processVirtual (tmp->value); tmp->value->next->size=(infon*)2;
            result=DoNext; noNewContent=false;
            break;
        case ProcessAlternatives:
            if (altCount>=1) break; // don't keep looking after found
            if(wrkNode->idFlags&isRawFlag){
              tmp=new infon(ci->pFlag,ci->wFlag,ci->size,ci->value,0,ci->spec1,ci->spec2,ci->next);
              tmp->prev=ci->prev; tmp->top=ci->top; tmp->type=ci->type; tmp->pos=ci->pos;
              insertID(&tmp->wrkList, item,0);
              wrkNode->item=item=tmp;
              wrkNode->idFlags^=isRawFlag;
            }
            wrkNode->idFlags|=NodeDoneFlag;
            cn.doShortNorm=0; cn.override=0;
            preNormalize(item, &cn);
            tempRes=doWorkList(item, CIfol,1); if(result<tempRes) result=tempRes;
            if (tempRes==BypassDeadEnd) {wrkNode->idFlags|=NoMatch; break;}
            altCount++;  theOne=item;  noNewContent=false;
            break;
        case (ProcessAlternatives+NodeDoneFlag):
            altCount++;  theOne=item; result=DoNext; noNewContent=false;
            break;
        case MergeIdent:
            wrkNode->idFlags|=SetComplete; IDfol=0;
            if(!(item->pFlag&isNormed) && ((item->wFlag&mFindMode)!=iNone || (item->wrkList && !(item->pFlag&isNormed)))) { // Norm item. esp %W, %//^
                if(item->top==0) {item->top=ci->top;}
                QitemPtr Qi(new Qitem(item));
                fetch_NodesNormalForm(Qi);
                if(Qi->whatNext!=DoNextBounded) noNewContent=false;
                if (item->wFlag&mAsProxie) item=item->value;
            }
            UInt CIsType=ci->pFlag&tType, ItemsType=item->pFlag&tType;
            if(CIsType==tUnknown) {
                ci->pFlag|=((item->pFlag&tType)+(tNum<<goSize)+sizeIndef);
                isIndef=1; ci->size=item->size;
                if (SizeIsKnown(item)) SetSizeFormat(ci, item->pFlag&(mFormat<<goSize));
                CIsType=ci->pFlag&tType;
            } else isIndef=0;
            if (ValueIsConcat(item)) std::cout << "WARNING: Trying to merge a concatenation.\n";
            if (!(looseType=(wrkNode->idFlags&mLooseType))) { // TODO: More rigorously verify and test strict/loose typing system.
                if (ItemsType && CIsType!=tList && CIsType!=ItemsType){SetBypassDeadEnd(); continue;}
                else if((ci->type && !(ci->wFlag&iHardFunc)) && (item->type && !checkTypeMatch(ci->type,item->type))){SetBypassDeadEnd(); continue;}
            }
            int infTypes=CIsType+4*ItemsType;
            switch(infTypes){ // TODO: Make size and value changes atomic. Currently, size may be set then value fails.
                case tUnknown+4*tUnknown: case tNum+4*tUnknown: case tString+4*tUnknown:
                case tList+4*tList:
                case tString+4*tString:
                case tNum+4*tNum:
                    result=DoNext;
                    // TODO: Verify and comment the following 'if' section.
                    if (!isIndef && ci->pFlag&sizeIndef && (infTypes==tString+4*tString)){ // When alts are evaluated (e.g., aaron, aron) use this secton.
                        if(infonSizeCmp(ci,item)<0) {SetBypassDeadEnd(); break;}
                        if (memcmp(ci->value, item->value, (UInt)item->size)!=0)  {SetBypassDeadEnd(); break;}
                        ci->size=item->size;
                        if(CIfol){
                            getFollower(&IDfol, item);
                            if(IDfol) addIDs(CIfol, IDfol, looseType, asAlt); else {SetBypassDeadEnd(); break;}
                        }
                        break;
                    }
                    if(ItemsType!=tUnknown){
                        if (ItemsType==tList) InitList(item);
                        UInt flagMask=0; infon* oldCiSize=ci->size;
                        // MERGE SIZES
                        if(!looseType &&  SizeIsKnown(item)){ // If item's size is known, we copy size
                            if(SizeIsUnknown(ci)) {ci->size=item->size;  flagMask|=(0xff<<goSize); }
                            else if (infonSizeCmp(ci,item)!=0 &&  (infTypes!= tList+4*tList) && (infTypes!= tString+4*tString)){
                                SetBypassDeadEnd(); std::cout<<"Sizes contradict\n"; break;
                            }
                        }
                        // MERGE VALUES
                        if(ValueIsKnown(item)){

                            if (infTypes== tNum+4*tNum){
                                if(ValueIsUnknown(ci)) {ci->value=item->value; flagMask|=0xff;}
                                else if (ci->value!=item->value){ci->size=oldCiSize; SetBypassDeadEnd(); std::cout<<"Values contradict\n"; break;}
                                // TODO: Allow for nums with non-matching sizes, as with strings. Clump with: concat, long nums/strings, arithmetic, etc.
                            }

                            else if (infTypes== tString+4*tString){
                                if(infonSizeCmp(ci,item)>0) {ci->size=oldCiSize; SetBypassDeadEnd(); break;} // TODO: Someday, allow this.
                                if(ValueIsUnknown(ci)){ci->value=item->value; SetValueFormat(ci, item->pFlag&mFormat);}
                                else if (memcmp(ci->value, item->value,  (UInt)ci->size)!=0) {ci->size=oldCiSize; SetBypassDeadEnd(); break;}
                            }

                            else if (infTypes== tList+4*tList){
                                if(ci->value) {
                                    if(!(item->value->pFlag&isTentative)) ci->value->pFlag&= ~isTentative; // See note below ("This is used...")
                                    if(looseType) addIDs(ci->value, item->value, looseType, asAlt);
                                    else{
                                        insertID(&ci->value->wrkList,item->value,0);
                                        ci->value->wFlag&=~nsWorkListDone;
                                    }
                                } else {ci->value=item->value; cpFlags(item,ci,0xff);if(SizeIsKnown(ci) && ci->size==0){IDfol=item;} else item->value->top=ci;}
                            }
                        }
                        if(flagMask) {cpFlags(item,ci,(flagMask+0xff00));}
                        if(true){
                            // This is used when we are merging two lists and the 'item' is not tentative but ci is.
                            // There are likely cases where we want to put something other than 'true' in the condition above.
                            // We could also make all these changes when the list-heads are merged but what about alternatives?
                            // Until there is a problem we do it this way:
                            if(!(item->pFlag&isTentative)) ci->pFlag&= ~isTentative;
                        }
                    }
                    isIndef=0;
                    if(CIfol){
                        if(infonSizeCmp(ci,item)==0 || (infTypes!= tString+4*tString)){
                            level=((IDfol)?0:getFollower(&IDfol, item));
                            if(IDfol){if(level==0) addIDs(CIfol, IDfol, looseType, asAlt); else result=BypassDeadEnd;}
                            else  if( (infTypes!= tList+4*tList)) {// temporary hack but it works ok.
                                if ((tmp=ci->isntLast()) && (tmp->next->pFlag&isTentative)) {SetBypassDeadEnd();}
                                if((tmp=getMasterList(ci) )){closeListAtItem(tmp); result=BypassDeadEnd; }
                            }
                        } else if((infTypes== tString+4*tString) && infonSizeCmp(ci,item)<0){
                            cSize=(UInt)ci->size;
                            tmp=new infon(item->pFlag,item->wFlag,(infon*)((UInt)item->size-cSize),(infon*)((UInt)item->value+cSize),0,0,item->next);
                            addIDs(CIfol, tmp, looseType, asAlt);
                        }
                    }
                break;
                case tNum+4*tString: result=DoNext; break;
                case tString+4*tNum: result=DoNext;break;
                case tString+4*tList: InitList(item);  result=DoNext; break;
                case tNum+4*tList:   InitList(item); addIDs(ci,item->value,looseType, asAlt); result=DoNext;break; // This should probably be insertID not addIDs. Check it out.
                case tList+4*tUnknown:
                case tList+4*tString:
                case tList+4*tNum:
                    if(ci->value){addIDs(ci->value, item, looseType, asAlt);}
                    else{copyTo(item, ci);}
                    result=DoNext;
            }
            break;
        }
    }while (wrkNode!=ci->wrkList); else {result=DoNext;} //result=(ci->next && (ci->pFlag&isTentative))?DoNextBounded:DoNext;
    if(altCount==1){
            for (f=1, tmp=getTop(ci); tmp!=0; tmp=getTop(tmp)) // check ancestors for alts
               if (tmp->pFlag&hasAlts) {f=0; break;}
            if(f) resolve(ci, theOne);
    }
    if(result==DoNext && noNewContent && (ci->pFlag&isTentative)) result=DoNextBounded;
    ci->pFlag|=isNormed; ci->pFlag&=~(sizeIndef);//+isTentative);
    return result;
}

#define pushCIsFollower {int lvl=cn->level-cn->nxtLvl; if(lvl>0) ItmQ.push(QitemPtr(new Qitem(cn->CIfol,0,0,lvl,cn->bufCnt,cn->parent)));}

void agent::pushNextInfon(infon* CI, QitemPtr cn, infQ &ItmQ){
    switch (cn->whatNext) {
    case DoNextBounded:
        if(++cn->bufCnt>=ListBuffCutoff){
            cn->nxtLvl=getFollower(&cn->CIfol,getTop(CI))+1;
            for(int c=0;c<cn->nxtLvl; c++) {cn->bufCnt=cn->parent->bufCnt; cn->parent=cn->parent->parent;}
            pushCIsFollower;
            break;
        }
    case DoNext:
        if(cn->whatNext==DoNext) cn->bufCnt=0;
        if((CI->pFlag&(mFormat+tType))==(FConcat+tNum)){
            compute(CI); if(cn->CIfol && !(CI->pFlag&isLast)){pushCIsFollower;}
        }else if(!((CI->pFlag&asDesc)&&!cn->override)&&((CI->value&&((CI->pFlag&tType)==tList))||ValueIsConcat(CI)) && !(CI->value->pFlag&isNormed)){
            ItmQ.push(QitemPtr(new Qitem(CI->value,cn->firstID,((cn->IDStatus==1) & !ValueIsConcat(CI))?2:cn->IDStatus,cn->level+1,0,cn))); // Push CI->value
        }else if (cn->CIfol){pushCIsFollower;}
        break;
    case BypassDeadEnd: {cn->nxtLvl=getFollower(&cn->CIfol,getTop(CI))+1; pushCIsFollower;} break;
    case DoNothing: break;
    }
}

infon* agent::Normalize(infon* i, infon* firstID){
    infon *CI;
    if (i==0) return 0;
    infQ ItmQ; ItmQ.push(QitemPtr(new Qitem(i,firstID,(firstID)?1:0,0)));
    while (!ItmQ.empty()){
        QitemPtr cn=ItmQ.front(); ItmQ.pop(); CI=cn->item; cn->doShortNorm=0;
        if(CI->wFlag&nsBottomNotLast) return 0;
        preNormalize(CI, &*cn);
        if (cn->doShortNorm) return 0;
        if((CI->wFlag&mFindMode)==0 && cn->IDStatus==2)
            {cn->IDStatus=0; insertID(&CI->wrkList,cn->firstID,0);}
        if((CI->pFlag&tType)==tList){InitList(CI);}
        cn->nxtLvl=getFollower(&cn->CIfol, CI);
        if((CI->pFlag&asDesc) && !cn->override) cn->whatNext=DoNext;//{pushCIsFollower; continue;}
        else cn->whatNext=doWorkList(CI, cn->CIfol);
        pushNextInfon(CI, cn, ItmQ);
    }
    return (i->pFlag&isNormed)?i:0;
};

int agent::fetch_NodesNormalForm(QitemPtr cn){
        int result=0; cn->CI=cn->item;
        if(cn->item->wFlag&nsBottomNotLast) return 0;
        if(!(cn->item->wFlag&nsNormBegan)){cn->CI=cn->item; cn->CIfol=0; cn->doShortNorm=0; cn->item->wFlag|=nsNormBegan;}
        if(!(cn->CI->wFlag&nsPreNormed)){
            result=0; preNormalize(cn->CI, &*cn); // NOWDO: make preNorm return something
            if(result>0) return result;
            cn->CI->wFlag|=nsPreNormed;
            if (cn->doShortNorm) cn->CI->wFlag|=nsWorkListDone;
            if((cn->CI->wFlag&mFindMode)==0 && cn->IDStatus==2)
                {cn->IDStatus=0; insertID(&cn->CI->wrkList,cn->firstID,0);}
        }

        if(!(cn->CI->wFlag&nsWorkListDone)){
            if(!cn->CIfol) cn->nxtLvl=getFollower(&cn->CIfol, cn->CI);
            if((cn->CI->pFlag&asDesc) && !cn->override) {cn->whatNext=DoNext;}
            else {
                if((cn->CI->pFlag&tType)==tList){InitList(cn->CI);}
                cn->whatNext=doWorkList(cn->CI, cn->CIfol);
            }
           //cn->CI->wFlag|=nsWorkListDone;
        }
        // NOWDO: Notify-and-remove subscribers
    return 0;
}

infon* agent::normalize(infon* i, infon* firstID){
        if (i==0) return 0;
        QitemPtr Qi(new Qitem(i,firstID,(firstID)?1:0,0));
        infQ ItmQ; ItmQ.push(Qi);
        while (!ItmQ.empty()){
            QitemPtr cn=ItmQ.front(); ItmQ.pop(); infon* CI=cn->item;
            fetch_NodesNormalForm(cn);
            //wait?
            pushNextInfon(CI, cn, ItmQ);
        }
    return 0;
}

