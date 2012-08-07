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

const int ListBuffCutoff=20;  // TODO: make this '2' to show a bug.

typedef map<dblPtr,UInt>::iterator altIter;
TagMap topTag2Ptr;
map<infon*,Tag> ptr2Tag;

#define recAlts(lval, rval) {if(InfsType(rval)==tString) alts[dblPtr((char*)rval->value.dataHead->get_num_mpz_t()->_mp_d,lval)]++;}
#define fetchLastItem(lval, item) {for(lval=item; InfsType(lval)==tList;lval=lval->value->prev);}
#define fetchFirstItem(lval, item) {for(lval=item; InfsTYpe(lval)==tList;lval=lval->value){};}

#include "xlater.h"
#include "XlaterENGLISH.h"
XlaterENGLISH EnglishXLater;

LanguageExtentions langExtentions; // This map stores valid locales and their xlater if available.
void populateLangExtentions(){     // Use this to load available language modules before normalizing any infons.
    int numLocales=0;
    const icu::Locale* locale = icu::Locale::getAvailableLocales(numLocales);
    for(int loc=0; loc<numLocales; ++loc){
        string localeID=locale[loc].getBaseName();
        string localeLanguage=locale[loc].getLanguage();
        if     (localeLanguage=="en") langExtentions[localeID]=&EnglishXLater;   // English
        else if(localeLanguage=="fr") langExtentions[localeID]=0;  // French
        else if(localeLanguage=="de") langExtentions[localeID]=0;  // German
        else if(localeLanguage=="ja") langExtentions[localeID]=0;  // Japanese
        else if(localeLanguage=="es") langExtentions[localeID]=0;  // Spanish
        else if(localeLanguage=="zh") langExtentions[localeID]=0;  // Chinese
        else if(localeLanguage=="ru") langExtentions[localeID]=0;  // Russian
        else if(localeLanguage=="it") langExtentions[localeID]=0;  // Italian
        else if(localeLanguage=="hi") langExtentions[localeID]=0;  // Hindi
        else if(localeLanguage=="he") langExtentions[localeID]=0;  // Hebrew
        else if(localeLanguage=="ar") langExtentions[localeID]=0;  // Arabic
        else if(localeLanguage=="eu") langExtentions[localeID]=0;  // Basque
        else if(localeLanguage=="pt") langExtentions[localeID]=0;  // Portuguese
        else if(localeLanguage=="bn") langExtentions[localeID]=0;  // Bengali
        else langExtentions[localeID]=0;
    }
}

infon* infon::isntLast(){ // 0=this is the last one. >0 = pointer to predecessor of the next one.
    if (!InfIsLast(this)) return this;
    infon* parent=getTop(this); infon* gParent=getTop(parent);
    if(gParent && ValueIsConcat(gParent))
        return parent->isntLast();
    return 0;
}

int agent::loadInfon(const char* filename, infon** inf, bool normIt){
    cout<<"Loading:'"<<filename<<"'..."<<flush;
    fstream InfonIn(filename);
    if(InfonIn.fail()){cout<<"Error: The file "<<filename<<" was not found.\n"<<flush; return 1;}
    QParser T(InfonIn); T.locale=locale;
    *inf=T.parse();
    if (*inf) {cout<<"done.   "<<flush;}
    else {cout<<"Error:"<<T.buf<<"   "<<flush; return 1;}
    if(normIt) {
        alts.clear();
        try{
            cout<<"Normalizing..."<<flush; normalize(*inf); cout << "Normalized."<<flush;
        } catch (char const* errMsg){cout<<errMsg;}
    }
    cout<<"\n";
    return 0;
}

int agent::StartTerm(infon* varIn, infon** varOut) {
  infon* tmp;
  if (varIn==0) return 1;
  if (ValueIsConcat(varIn)){
    if ((*varOut=varIn->value.listHead)==0) return 2;
    do {
      switch(InfsType(varIn)){
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
  if (InfsType(varIn) != tList) {return 3;}
  else {*varOut=varIn->value.listHead; if (*varOut==0) return 4;}
  return 0;
}

int agent::LastTerm(infon* varIn, infon** varOut) {
  if (varIn==0) return 1;
  if (ValueIsConcat(varIn)) {varIn=varIn->value.listHead->prev;}
  else if(InfsType(varIn) != tList)  return 2;
  else {*varOut=varIn->value.listHead->prev; if (*varOut==0) return 3;}
  return 0;
}

int agent::getNextTerm(infon** p) {
  infon *parent, *Gparent=0, *GGparent=0;
  if(InfIsBottom(*p)) {
    parent=(InfIsFirst(*p))?(*p)->top:(*p)->top->top;
    if (InfIsLast(*p)){
      if(parent==0){(*p)=(*p)->next; return 1;}
      if(parent->top) Gparent=(InfIsFirst(parent))?parent->top:parent->top->top;
      if(Gparent && ValueIsConcat(Gparent)) {
        if(Gparent->top) GGparent=(InfIsFirst(Gparent))?Gparent->top:Gparent->top->top;
        do {
          if(getNextTerm(&parent)) return 4;
// TODO: the next line fixes one problem but causes another: nested list-concats.
// test it with:  (   ({} {"X", "Y"} ({} {9,8} {7,6}) {5} ) {1} {{}} {2, 3, 4} )
          if(GGparent && ValueIsConcat(GGparent)) {*p=parent; return 0;}
          switch(InfsType(parent)){
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
    if (InfIsLast(*p)){     // If isLast && Parent is spec1-of-get-last, get parent, apply any idents
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
    if (result==0 && !InfIsNormed(*p)) normalize(*p);
    return result;
}

// TODO: This function is out-of-date but not used yet anyhow.
int agent::getPrevTerm(infon** p) { gpt1:
  if(InfIsTop(*p))
    if (InfIsFirst(*p)) {}
//      if ((*p)!=(*p)->first->size){(*p)=(*p)->prev; return 1;}
//      else {(*p)=(*p)->first->first; goto gpt1;}
          else {return 2;} /*Bottom but not end, make subscription*/
  else {
    (*p)=(*p)->prev;
gpt2: //if ((((VsFlag(*p)&mFlags1)>>8)&rType)==rList)
    if ((*p)->size.listHead==0) {goto gpt1;}
    else {(*p)=(*p)->size.listHead->prev; goto gpt2;}
    if((*p)==0) return 3;
  }
  return 0;
}

//////////// Routines for copying infon data to C formats:
BigInt agent::gIntNxt(infon** ItmPtr){
    BigInt num;
    (*ItmPtr)->getInt(&num);
    getNextTerm(ItmPtr);
    return num; //(sign)?-num:num;
}

double agent::gRealNxt(infon** ItmPtr){
    double ret;
    (*ItmPtr)->getReal(&ret);
    getNextTerm(ItmPtr);
    return ret;
}

char* agent::gStrNxt(infon** ItmPtr, char*txtBuff){
    if(InfsType(*ItmPtr)==tString && ValueIsKnown(*ItmPtr)) {
        UInt strLen=(*ItmPtr)->getSize().get_ui();
        memcpy(txtBuff, (*ItmPtr)->value.toString((*ItmPtr)->getSize().get_ui()).c_str(), strLen);
        txtBuff[strLen]=0;
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
    if(! (list && list->value.listHead && list->value.listHead->prev)) return 0;
    infon *last=list->value.listHead->prev;
    infon *p=last, *q;
    do {  // find the earliest-but-at-end tentative in list
        q=p; p=p->prev; // Shift left in list
    } while(p!=last && InfIsVirtTent(p));
    if(!(q && InfIsVirtTent(q))) return 0;
    if(InfIsVirtual(q)) processVirtual(q);
    copyTo(i, q); if(InfsType(q)==tList && q->size.listHead) q->value.listHead->top=q;
    q->spec1=i->spec1; q->spec2=i->spec2; q->wrkList=i->wrkList;
    ResetVirtTent(q);
    normalize(q);
    return q;
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

infon* agent::copyList(infon* from, int flags, infon* tagCtxt){
    infon* top=0; infon* follower; infon* q, *p=from;
    if(from==0)return 0;
    do {
        q=new infon;
        deepCopy(p,q, 0, flags, tagCtxt); q->pos=p->pos;
        if (top==0) follower=top=q;
        q->prev=follower; q->top=top; follower=follower->next=q;
        p=p->next;
    } while(p!=from);
    top->prev=follower; top->wFlag|=from->wFlag & mListPos;
    follower->next=top;
    return top;
}

void agent::deepCopyPure(pureInfon* from, pureInfon* to, int flags, infon* tagCtxt){
    to->flags=from->flags;
    to->offset=from->offset;
    if(PureIsInListMode(*from)) to->listHead=copyList(from->listHead, flags, tagCtxt);
    else {to->listHead=0; to->dataHead=from->dataHead;}
}

void agent::deepCopy(infon* from, infon* to, PtrMap* ptrs, int flags, infon* tagCtxt){
    UInt fm=from->wFlag&mFindMode; infon* tmp;
    to->wFlag=(from->wFlag&0xffffffff);
    to->tag2Ptr=from->tag2Ptr;
    if(to->type==0) {  // TODO: We should merge types, not just copy over.
        if(from->type){ cout<<"TAG:"<<from->type->tag<<"--"<<tagCtxt<<"\n";
            if(tagCtxt) if(!tagCtxt->findTag(from->type)) throw "Nested tag not found when copying.";
            to->type=from->type;
        }
    }

    deepCopyPure(&from->size, &to->size, flags, tagCtxt);  if(to->size.listHead) to->size.listHead->top=to;
    deepCopyPure(&from->value,&to->value,flags, tagCtxt); if(to->value.listHead)to->value.listHead->top=to;

    if(from->wFlag&mIsHeadOfGetLast) to->top2=(*ptrs)[from->top2];
    if(fm==iToPath || fm==iToPathH || fm==iToArgs || fm==iToVars) {
        to->spec1=from->spec1;
        if (ptrs) to->top=(*ptrs)[from->top];
    } else if((fm==iGetFirst || fm==iGetLast || fm==iGetMiddle) && from->spec1) {
        PtrMap *ptrMap = (ptrs)?ptrs:new PtrMap;
        if ((to->prev) && (tmp=to->prev->spec1) && (tmp->wFlag&mIsHeadOfGetLast) && (tmp=tmp->next)) to->spec1=tmp;
        else {to->spec1=new infon; }
        (*ptrMap)[from]=to;
        deepCopy (from->spec1, to->spec1, ptrMap, flags, tagCtxt);
        if(ptrs==0) delete ptrMap;
        if (flags && from->wFlag&mAssoc) {
            from->wFlag&= ~(mAssoc+mFindMode); from->wFlag|=iAssocNxt;
            SetValueFormat(from, fUnknown); SetSizeFormat(from, fUnknown);
        }
    }

    if ((from->wFlag&mFindMode)==iAssocNxt) {to->spec2=from;}  // Breadcrumbs to later find next associated item.
    else if(!from->spec2) to->spec2=0;
    else {to->spec2=new infon; deepCopy(from->spec2, to->spec2,0,flags,tagCtxt);}

    infNode *p=from->wrkList, *q;  // Merge Identity Lists
    if(p) do {
        q=new infNode; q->idFlags=p->idFlags;
        if((p->item->wFlag&mFindMode)<iAssocNxt && (p->item->wFlag&mFindMode)>iNone) {q->item=new infon; q->idFlags=p->idFlags; deepCopy(p->item,q->item,ptrs,flags,tagCtxt);}
        else {q->item=p->item;}
        prependID(&to->wrkList, q);
        p=p->next;
    } while (p!=from->wrkList);
}

void closeListAtItem(infon* lastItem){ // remove (no-longer valid) items to the right of lastItem in a list.
//cout << "CLOSING: " << printInfon(lastItem) << "\n";
    // TODO: Will this work when a list becomes empty?
    infon *itemAfterLast, *nextItem; UInt count=0;
    for(itemAfterLast=lastItem->next; !InfIsTop(itemAfterLast); itemAfterLast=nextItem){
        nextItem=itemAfterLast->next;
        recover(itemAfterLast);
    }
    lastItem->next=itemAfterLast; itemAfterLast->prev=lastItem; lastItem->wFlag|=(isBottom+isLast);
    // The lines below remove any tentative flags then set parent's size
    do{
        ResetTent(itemAfterLast);
        itemAfterLast=itemAfterLast->next;
        count++;
    } while (!InfIsTop(itemAfterLast));
    itemAfterLast->top->size=(BigFrac)count;
    SetSizeFormat(itemAfterLast, fLiteral); SetSizeType(itemAfterLast,tNum);
}

char isPosLorEorGtoSize(UInt pos, infon* item){
    if(SizeIsUnknown(item)) return '?';
    if(SizeType(item)!=tNum) throw "Size as non integer is probably not useful, so not allowed.";
 /*   if(SizeIsConcat(item)){ // TODO: apparently no tests call this block. related to the Range bug.
        infon* size=item->size;
        //if ((size->wFlag&mFindMode)!=iNone) normalize(size);  // TODO: Verify do we need this?
        if (IsSimpleUnknownNum(size))
            if(size->next!=size && size->next==size->prev && (IsSimpleUnknownNum(size->next))){
                if(pos<(UInt)size->value) return 'L';
                if(pos>(UInt)size->size) return 'G';
                return '?';  // later, if pos is in between two ranges, return 'X'
            }
    }   */
    int cmpNums=cmp(item->getSize(), pos);
    if(cmpNums > 0) return 'L';  // Less
    if(cmpNums == 0) return 'E';  // Equal
    else return 'G';  // Greater
}

void agent::processVirtual(infon* v){
    infon *args=v->spec1, *spec=v->spec2, *parent=getTop(v); int EOT=0;
    char posArea=isPosLorEorGtoSize(v->pos, parent);
    if(posArea=='G'){cout << "EXTRA ITEM ALERT!\n"; closeListAtItem(v); return;}
    UInt tmpFlags=v->wFlag&mListPos;  // TODO B4: move this flag stuff into deepCopy. Clean the following block.
    if (spec){
        if((spec->wFlag&mFindMode)==iAssocNxt) {
            copyTo(spec, v); SetSizeType(v,tNum); SetSizeFormat(v,fUnknown); VsFlag(v)=fUnknown; v->spec1=spec->spec2;
        } else {deepCopy(spec, v,0,1); }
        if(posArea=='?' && v->spec1) posArea= 'N';
    }
    v->wFlag|=tmpFlags;  // TODO B4: move this flag stuff into deepCopy.
    ResetVirt(v);
    if(EOT){ if(posArea=='?'){posArea='E'; closeListAtItem(v);}
        else if(posArea!='E') throw "List was too short";}
    if (posArea=='E') {v->wFlag|=isBottom+isLast; return;}
    infon* tmp= new infon; tmp->pos=(v->pos+1); tmp->spec2=spec; tmp->wFlag=(isBottom+isVirtual);
    VsFlag(tmp)|=fUnknown; SetSizeType(tmp,tNum); SetSizeFormat(tmp,fUnknown); tmp->wFlag|=iNone;
    tmp->top=tmp->next=v->next; v->next=tmp; tmp->prev=v; tmp->next->prev=tmp; tmp->spec1=args;
    v->wFlag&=~isBottom;
    if (posArea=='?'){ SetIsTent(v);}
}

void agent::InitList(infon* item) {
    infon* tmp;
    if(!(item->wFlag&nsListInited) && item->value.listHead && (((tmp=item->value.listHead->prev)->wFlag)&isVirtual)){
        item->wFlag|=nsListInited;
        tmp->spec2=item->spec2;
    //    if(tmp->spec2 && ((tmp->spec2->pFlag&mRepMode)==asFunc)) // Remove this after testing an argument as a function
    //            StartTerm(tmp->spec2->spec1, &tmp->spec1);
        processVirtual(tmp); VsFlag(item)|=tList;
    }
}

int agent::getFollower(infon** lval, infon* i){
    int levels=0;
    if(!i) return 0;
    gnsTop: if(i->next==0) {*lval=0; return levels;}
 /*  infon* size;
     if(InfIsVirtual(i->next) && i->spec2 && i->spec2->prev==(infon*)2){
        i->wFlag|=(isLast+isBottom);
        size=i->next->size-1;
        i->next->next->prev=i; i->next=i->next->next;// TODO when working on mem mngr: i->next is dangling
        i=getTop(i); i->size=size; VsFlag(i) &= ~fLoop; ++levels;
        goto gnsTop;
    }*/
    if(InfIsLast(i) && i->prev){
        i=getTop(i); ++levels;
        if(i) goto gnsTop;
        else {*lval=0; return levels;}
    }
    *lval=i; getNextTerm(lval);
    if(InfIsVirtual(*lval)) processVirtual(*lval);
    return levels;
}

UInt calcItemsPos(infon* i){
    UInt count;
    if (i->pos) return ((UInt)i->pos)-1;
    for(count=1; !InfIsTop(i); i=i->prev)
        ++count;
    return count;
}

void agent::AddSizeAlternate(infon* Lval, infon* Rval, infon* Pred, UInt Size, infon* Last, UInt Flags) {
    infon *copy, *copy2, *LvalFol; pureInfon size(Size); SetBits(size.flags, mFormat, Lval->size.flags&mFormat);
    copy=new infon(Lval->wFlag, &size, &Lval->value ,0, Lval->spec1,Lval->spec2,Lval->next); // TODO B4: verify correct size and val usage
    copy->prev=Lval->prev; copy->top=Lval->top;copy->pred=Pred; copy->type=Lval->type;
    insertID(&Lval->wrkList,copy,ProcessAlternatives|Flags); Lval->wrkList->slot=Last; Lval->wFlag&=~nsWorkListDone;
    getFollower(&LvalFol, Lval);
    if (LvalFol){
        copy2=new infon(LvalFol->wFlag, &LvalFol->size, &LvalFol->value ,0,LvalFol->spec1,LvalFol->spec2,LvalFol->next); // TODO B4: verify correct size and val usage
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
    while(crntAlt && InfIsTentative(crntAlt)){
        getFollower(&crntAlt, getTop(crntAlt));
        RvlLst[altCnt++]=crntAlt;
        if(altCnt>=maxAlternates) throw "Too many nested alternates";
    }
    size=calcItemsPos(Lvals); crntAlt=Lvals; pred=Lvals->pred;
    while(crntAlt){  // Lvals
        for(int i=0; i<altCnt; ++i){ // Rvals
            if (!asAlt && altCnt==1 && crntAlt==Lvals){
                insertID(&Lvals->wrkList,Rvals,flags);
                Lvals->wFlag&=~nsWorkListDone;
                recAlts(Lvals,Rvals);
            } else {
                Rval=RvlLst[i];
                AddSizeAlternate(crntAlt, Rval, pred, size, (prev)?prev->prev:0, flags);
            }
        }
        if(InfIsTentative(crntAlt)) {prev=crntAlt; size=calcItemsPos(crntAlt); crntAlt=getTop(crntAlt); if(crntAlt) crntAlt->wFlag|=hasAlts;}
        else crntAlt=0;
    }
}

infon* getIdentFromPrev(infon* prev){
// TODO: this may only work in certain cases. It should do a recursive search.
    return prev->wrkList->item->wrkList->item;
}

inline infon* getMasterList(infon* item){
    for(infon* findMaster=item; findMaster; findMaster=findMaster->top){
        infon* nxtParent=getTop(findMaster);
       // if(findMaster->next && InfIsTentative(findMaster->next)) return findMaster;
        if(nxtParent && InfIsLoop(nxtParent)) return findMaster;
    }
    return 0;
}

int agent::checkTypeMatch(Tag* LType, Tag* RType){
    return (*LType)==(*RType);
}

int agent::compute(infon* i){
    infon* p=i->value.listHead; BigInt vAcc, sAcc; int count=0;
    if(p) do{
        normalize(p); // TODO: appending inline rather than here would allow streaming.
        if(SizeType(p)==tNum && InfsType(p)==tNum){
            if (SizeIsUnknown(p)) return 0;
            if (SizeIsConcat(p)) compute(p->size.listHead);
            if (SizeType(p)!=tNum) return 0;
            if (ValueIsUnknown(p)) return 0;
            if (InfsFormat(p)>=fConcat) compute(p->value.listHead);
            if (InfsType(p)!=tNum) return 0;
            BigInt val;
            if((p)->value.flags & fInvert) {val=-(*p->value.dataHead);} else {val=(*p->value.dataHead);}
            if(SizeIsInverted(p)){
                if (++count==1){sAcc= *p->size.dataHead; vAcc=val;}
                else {sAcc/= *p->size.dataHead; vAcc=(vAcc/ *p->size.dataHead)+val;}
            } else {
                if (++count==1){sAcc= *p->size.dataHead; vAcc=val;}
                else {sAcc*= *p->size.dataHead; vAcc=(vAcc*(*p->size.dataHead))+val;}
            }//cout<<"V:"<<vAcc.get_str()<<"\n";
        } else return 0;
        p=p->next;
    } while (p!= i->value.listHead);
    i->value=(mpq_class)vAcc; i->size=(mpq_class)sAcc;
   // i->pFlag=(i->pFlag&0xFF00FF00)+(tNum<<goSize)+tNum;
    return 1;
}

void resolve(infon* i, infon* theOne){ //cout<<"RESOLVING";
    infon *prev=0;
    while(i && theOne){
        if(InfIsTentative(theOne)){
            closeListAtItem(theOne); //cout<<"-CLOSED\n";
            return;
        } else {
            i->wFlag&=~hasAlts; copyTo(theOne, i);
            if(InfsType(theOne)==tList) {
                infon* itm=i->value.listHead;
                for (UInt p=i->getSize().get_ui(); p; --p) {
                    ResetTent(itm); itm=itm->next;}
             }
            prev=i; i=i->pred; theOne=theOne->pred;
        }
    } if (theOne){theOne->next=prev->value.listHead; theOne->wFlag|=isLast+isBottom;}
 //   cout<<"-OPENED:"<<printInfon (i)<<"\n";
}

void agent::prepWorkList(infon* CI, Qitem *cn){
    infon *newID; int CIFindMode;
    while ((CIFindMode=(CI->wFlag&mFindMode))){
        newID=0;
        if(InfToExec(CI)) cn->override=1;
        if(InfAsDesc(CI)) {if(cn->override) cn->override=0; else break;}
        switch(CI->wFlag&mSeed){
            case sUseAsFirst: cn->firstID=CI->spec2; cn->IDStatus=1; CI->wFlag&=~mSeed; break;
            case sUseAsList: CI->spec1->wFlag|=toExec;  newID=CI->spec1;
        }
        switch(CIFindMode){
            case iToWorld: copyTo(world, CI); break;
            case iToCtxt:   copyTo(&context, CI); break; // TODO: Search agent::context
            case iToArgs: case iToVars:
                for (newID=CI->top; newID && !(newID->top->wFlag&mIsHeadOfGetLast); newID=newID->top){}
                if(newID && CIFindMode==iToVars) newID=newID->next;
                if(newID) {CI->wFlag|=mAsProxie; CI->value.proxie=newID; newID->wFlag|=isNormed; CI->wFlag&=~mFindMode; newID=0;}
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
                    newID=getTop(newID); if (newID==0 ) {cout << "Too many '\\'s in "<<printInfon(CI)<< '\n';}
                }
                if(CIFindMode==iToPathH) {  // If no '^', move to first item in list.
                    if(!InfIsTop(newID)) {newID=newID->top; }
                    if (newID==0) cout<<"Zero TOP in "<< printInfon(CI)<<'\n';
                    else if(!InfIsFirst(newID)) {newID=0; cout<<"Top but not First in "<< printInfon(CI)<<'\n';}
                }
                if(newID) {CI->wFlag|=mAsProxie; CI->value.proxie=newID; newID->wFlag|=isNormed; CI->wFlag&=~mFindMode; newID=0;}
                cn->doShortNorm=true;
            } break;
            case iTagDef: break; // Reserved
            case iTagUse: {
                if(CI->type == 0) throw ("A tag was null which is a bug");
                // OUT("Recalling: "<<CI->type->tag<<":"<<CI->type->locale);
                infon* found=CI->findTag(CI->type);
                if (found) {
                    bool asNotFlag=((CI->wFlag&asNot)==asNot);
                    UInt tmpFlags=CI->wFlag&mListPos; deepCopy(found,CI,0,0,CI->type->tagCtxt); CI->wFlag|=tmpFlags; // TODO B4: move this flag stuff into deepCopy.
                    if(CI->wFlag&asNot) asNotFlag = !asNotFlag;
                    SetBits(CI->wFlag, asNot, (asNotFlag)?asNot:0);
                    deTagWrkList(CI);
                } else{OUT("\nBad tag:'"<<CI->type->tag<<"'\n");throw("A tag was used but never defined");}
                break;}
            case iGetFirst:  StartTerm (CI, &newID); break;
            case iGetMiddle: break; // TODO: iGetMiddle
            case iGetLast:
                if (SizeIsInverted(CI->spec1)){ // TODO: This block is a hack to make simple backward references work. Fix for full back-parsing.
                    newID=CI;
                    for(UInt i=(UInt)CI->spec1->getSize().get_ui(); i>0; --i){newID=newID->prev;}
                    {insertID(&CI->wrkList, newID,0); if(ValueIsUnknown(CI)) {CI->size=newID->size;} cpFlags(newID, CI,~mListPos);}
                    SetValueFormat(CI, fUnknown);
                } else{
                    normalize(CI->spec1, cn->firstID);
                    if( ! CI->spec1->isntLast ()) CI->wFlag|=(isBottom+isLast);
                    if(InfHasAlts(CI->spec1)) {  // migrate alternates from spec1 to CI... Later, build this into LastTerm.
                        infNode *wrkNode=CI->spec1->wrkList; infon* item=0;
                        if(wrkNode)do{
                            wrkNode=wrkNode->next; item=wrkNode->item;
                            if(((wrkNode->idFlags&(ProcessAlternatives+NoMatch))==ProcessAlternatives)&& (item->wFlag & mIsHeadOfGetLast)){
                                if(*wrkNode->item->size.dataHead == 0){} // TODO: handle the null alternative
                                else {insertID(&CI->wrkList,wrkNode->slot,ProcessAlternatives+isRawFlag);}
                            }
                        }while (wrkNode!=CI->spec1->wrkList);
                    }
                    else LastTerm(CI->spec1, &newID);
                }
                SetValueFormat(CI, fUnknown);
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
                    ResetTent(masterItem);
                }
                if(!newID->isntLast()){
                    closeListAtItem((masterItem)?masterItem : getMasterList(CI));
                }
            }
            insertID(&CI->wrkList, newID,(CIFindMode>=iAssocNxt)?skipFollower:0);
            VsFlag(CI)&=~mType; CI->wFlag&=~mFindMode;
        }
    }
}

enum rejectModes {rAccept=0, rReject, rNullable, rInvertable};

int agent::doWorkList(infon* ci, infon* CIfol, int asAlt){
    infNode *wrkNode=ci->wrkList; infon *item, *IDfol, *tmp, *theOne=0; Qitem cn;
    UInt altCount=0, level, tempRes, isIndef=0, result=DoNothing, f, looseType, noNewContent=true;
    if(CIfol && !CIfol->pred) CIfol->pred=ci;
    if(wrkNode)do{
        wrkNode=wrkNode->next; item=wrkNode->item;
        bool cpySize=0, cpyValue=0, resetCIsTentative=0, linkCIFol=false, invertAcceptance=(ci->wFlag&asNot); int reject=rAccept;
        if (wrkNode->idFlags&skipFollower) CIfol=0;
        switch (wrkNode->idFlags&WorkType){
        case InitSearchList: // E.g., {[....]|...}::={3 4 5 6}
            tmp=new infon();
            tmp->top2=ci; tmp->value.listHead=ci->value.listHead->spec1;
            {tmp->size=ci->size; cpFlags(ci,tmp,0xff0000);} VsFlag(tmp)|=(fLoop+fUnknown+tList); // TODO: Should fUnknown really be here?
            insertID(&tmp->wrkList, item, MergeIdent);  // Appending the {3 4 5 6}
            tmp->value.listHead->next = tmp->value.listHead->prev = tmp->value.listHead;
            tmp->value.listHead->wFlag|=(isTop+isBottom+isFirst+isVirtual);
            tmp->value.listHead->top=tmp;
            insertID(&tmp->value.listHead->wrkList, item->value.listHead, MergeIdent);
            processVirtual (tmp->value.listHead);
            tmp->value.listHead->next->size=(mpq_class)2;
            result=DoNext; noNewContent=false;
            break;
        case ProcessAlternatives:
            if (altCount>=1) break; // Don't keep looking after found
            if(wrkNode->idFlags&isRawFlag){
              tmp=new infon(ci->wFlag,&ci->size, &ci->value,0,ci->spec1,ci->spec2,ci->next);
              tmp->prev=ci->prev; tmp->top=ci->top; tmp->type=ci->type; tmp->pos=ci->pos;
              insertID(&tmp->wrkList, item,0);
              wrkNode->item=item=tmp;
              wrkNode->idFlags^=isRawFlag;
            }
            wrkNode->idFlags|=NodeDoneFlag;
            cn.doShortNorm=0; cn.override=0;
            prepWorkList(item, &cn);
            tempRes=doWorkList(item, CIfol,1); if(result<tempRes) result=tempRes;
            if (tempRes==BypassDeadEnd) {wrkNode->idFlags|=NoMatch; break;}
            altCount++;  theOne=item;  noNewContent=false;
            break;
        case (ProcessAlternatives+NodeDoneFlag):
            altCount++;  theOne=item; result=DoNext; noNewContent=false;
            break;
        case MergeIdent:
            wrkNode->idFlags|=SetComplete; IDfol=0;
            if(!InfIsNormed(item) && ((item->wFlag&mFindMode)!=iNone || (item->wrkList && !InfIsNormed(item)))) { // Norm item. esp %W, %//^
                if(item->top==0) {item->top=ci->top;}
                QitemPtr Qi(new Qitem(item));
                fetch_NodesNormalForm(Qi);
                if(Qi->whatNext!=DoNextBounded) noNewContent=false;
                if (item->wFlag&mAsProxie) item=item->value.proxie;
            }
            UInt CIsType=InfsType(ci), ItemsType=InfsType(item);
            if(CIsType==tUnknown && !invertAcceptance) {
                VsFlag(ci)|=(InfsType(item)+fUnknown); SetSizeType(ci, tNum); ci->wFlag|=sizeIndef;
                isIndef=1; ci->size=item->size;
                if (SizeIsKnown(item)) SetSizeFormat(ci, item->size.flags & mFormat);
                CIsType=InfsType(ci);
            } else isIndef=0;
            if (ValueIsConcat(item)) cout << "WARNING: Trying to merge a concatenation.\n";
            if (!(looseType=(wrkNode->idFlags&mLooseType))) { // TODO: More rigorously verify and test strict/loose typing system.
                if (ItemsType && CIsType!=tList && CIsType!=ItemsType){reject=rInvertable;}
                else if((ci->type && !(ci->wFlag&iHardFunc)) && (item->type && !checkTypeMatch(ci->type,item->type))){reject=rInvertable;}
            }
            int infTypes=CIsType+4*ItemsType;
            if (!reject) switch(infTypes){
                case tUnknown+4*tUnknown: case tNum+4*tUnknown: case tString+4*tUnknown:
                case tNum+4*tString:
                case tString+4*tNum:
                case tList+4*tList:
                case tString+4*tString:
                case tNum+4*tNum:
                    result=DoNext;
                    linkCIFol=true;
                    if (!isIndef && ci->wFlag&sizeIndef && ((infTypes== tNum+4*tNum) || (infTypes== tString+4*tString ))){
                        if (infValueCmp(item, ci)==0) cpySize=true; // Override a previous alt's size if size was not definite.
                        else {reject=rNullable; break;}
                    }
                    else if(ItemsType!=tUnknown){
                        if (ItemsType==tList) InitList(item);
                        // MERGE SIZES
                        if(!looseType &&  SizeIsKnown(item)){
                            if(SizeIsUnknown(ci)) {cpySize=true;}
                            else if (infonSizeCmp(ci,item)!=0 &&  (infTypes!= tList+4*tList) && (infTypes!= tString+4*tString)){
                                reject=rInvertable; cout<<"Sizes contradict\n"; break;
                            }
                        }
                        // MERGE VALUES
                        if(ValueIsKnown(item)){
                            if ((infTypes== tNum+4*tNum) || (infTypes== tString+4*tString )){
                                if(!cpySize && SizeIsKnown(item) && SizeIsKnown(ci) && infonSizeCmp(ci,item)>0) {reject=rNullable; break;}
                                if(ValueIsUnknown(ci)) {cpyValue=true;}
                                else if (((cpySize)?infValueCmp(item, ci):infValueCmp(ci, item))!=0) {reject=rInvertable; break;}
                            }
                            else if (infTypes== tList+4*tList){
                                linkCIFol=false;
                                if(ci->value.listHead) {
                                    if(!InfIsTentative(item->value.listHead)) ResetTent(ci->value.listHead); // See note below ("This is used...")
                                    if(looseType) addIDs(ci->value.listHead, item->value.listHead, looseType, asAlt);
                                    else{
                                        insertID(&ci->value.listHead->wrkList,item->value.listHead,0);
                                        ci->value.listHead->wFlag&=~nsWorkListDone;
                                    }
                                } else {ci->value=item->value; cpFlags(item,ci,0xff);if(SizeIsKnown(ci) && *ci->size.dataHead==0){IDfol=item;} else item->value.listHead->top=ci;}
                            }
                        }
                        if(true){
                            // This is used when we are merging two lists and the 'item' is not tentative but ci is.
                            // There are likely cases where we want to put something other than 'true' in the condition above.
                            // We could also make all these changes when the list-heads are merged but what about alternatives?
                            // Until there is a problem we do it this way:
                            if(!InfIsTentative(item)) resetCIsTentative=true;
                        }
                    }
                    isIndef=0;

                break;
                case tString+4*tList:
                case tNum+4*tList:   InitList(item); addIDs(ci,item->value.listHead,looseType, asAlt); result=DoNext;break; // this should probably be insertID not addIDs. Check it out.

                case tList+4*tUnknown:
                case tList+4*tString:
                case tList+4*tNum:
                    if(ci->value.listHead){addIDs(ci->value.listHead, item, looseType, asAlt);}
                    else{copyTo(item, ci);}
                    result=DoNext;
            }
            if (invertAcceptance) {
                if(reject==rInvertable){reject=rAccept; result=DoNext; linkCIFol=true;}
                else {reject=rNullable; linkCIFol=false;}
            } else if (!reject){ // Not inverted acceptance
                if (cpySize)  ci->size=item->size;
                if (cpyValue) ci->value=item->value; //  do we ever need to also copy wFlags and type fields?
                if (resetCIsTentative) ResetTent(ci);
            }
            if(linkCIFol && CIfol){
                if(infonSizeCmp(ci,item)==0 || (infTypes!= tString+4*tString)){
                    level=((IDfol)?0:getFollower(&IDfol, item));
                    if(IDfol){if(level==0) addIDs(CIfol, IDfol, looseType, asAlt); else reject=rReject;}
                    else  if( (infTypes!= tList+4*tList)) {// temporary hack but it works ok.
                        if ((tmp=ci->isntLast()) && InfIsTentative(tmp->next)) {reject=rNullable;}
                        if((tmp=getMasterList(ci) )){closeListAtItem(tmp); if(!reject) reject=rReject; }
                    }
                } else if((infTypes== tString+4*tString) && infonSizeCmp(ci,item)<0){
                    BigInt cSize=ci->getSize();
                    pureInfon pSize(item->getSize()-cSize);
                    pureInfon pVal(item->value.dataHead, item->value.flags, item->value.offset+cSize);
                    tmp=new infon(item->wFlag, &pSize, &pVal,0,0,item->next);
                    addIDs(CIfol, tmp, looseType, asAlt);
                }
            }
            if (reject){
                result=BypassDeadEnd;
                if(reject >= rNullable){
                    infon* CA=getTop(ci); if (CA) {AddSizeAlternate(CA, item, 0, ((UInt)ci->next->pos)-1, ci, looseType); }
                }
            }
            break;
        }
    }while (wrkNode!=ci->wrkList); else {result=DoNext;} //result=(ci->next && InfIsTentative(ci))?DoNextBounded:DoNext;
    if(altCount==1){
        for (f=1, tmp=getTop(ci); tmp!=0; tmp=getTop(tmp)) // Check ancestors for alts
           if (InfHasAlts(tmp)) {f=0; break;}
        if(f) resolve(ci, theOne);
    }
    if(result==DoNext && noNewContent && InfIsTentative(ci)) result=DoNextBounded;
    ci->wFlag|=isNormed; ci->wFlag&=~(sizeIndef);
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
        if((VsFlag(CI)&(mFormat+mType))==(fConcat+tNum)){
            compute(CI); if(cn->CIfol && !InfIsLast(CI)){pushCIsFollower;}
        }else if(!(InfAsDesc(CI)&&!cn->override)&&((CI->value.listHead&&(InfsType(CI)==tList))||ValueIsConcat(CI)) && !InfIsNormed(CI->value.listHead)){
            ItmQ.push(QitemPtr(new Qitem(CI->value.listHead,cn->firstID,((cn->IDStatus==1) & !ValueIsConcat(CI))?2:cn->IDStatus,cn->level+1,0,cn))); // Push CI->value
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
        prepWorkList(CI, &*cn);
        if (cn->doShortNorm) return 0;
        if((CI->wFlag&mFindMode)==0 && cn->IDStatus==2)
            {cn->IDStatus=0; insertID(&CI->wrkList,cn->firstID,0);}
        if(InfsType(CI)==tList){InitList(CI);}
        cn->nxtLvl=getFollower(&cn->CIfol, CI);
        if(InfAsDesc(CI) && !cn->override) cn->whatNext=DoNext;
        else cn->whatNext=doWorkList(CI, cn->CIfol);
        pushNextInfon(CI, cn, ItmQ);
    }
    return (InfIsNormed(i))?i:0;
};

int agent::fetch_NodesNormalForm(QitemPtr cn){
        int result=0; cn->CI=cn->item;
        if(cn->item->wFlag&nsBottomNotLast) return 0;
        if(!(cn->item->wFlag&nsNormBegan)){cn->CI=cn->item; cn->CIfol=0; cn->doShortNorm=0; cn->item->wFlag|=nsNormBegan;}
        if(!(cn->CI->wFlag&nsPreNormed)){
            result=0; prepWorkList(cn->CI, &*cn); // NOWDO: make prepWorkList return something
            if(result>0) return result;
            cn->CI->wFlag|=nsPreNormed;
            if (cn->doShortNorm) cn->CI->wFlag|=nsWorkListDone;
            if((cn->CI->wFlag&mFindMode)==0 && cn->IDStatus==2)
                {cn->IDStatus=0; insertID(&cn->CI->wrkList,cn->firstID,0);}
        }

        if(!(cn->CI->wFlag&nsWorkListDone)){
            if(!cn->CIfol) cn->nxtLvl=getFollower(&cn->CIfol, cn->CI);
            if(InfAsDesc(cn->CI) && !cn->override) {cn->whatNext=DoNext;}
            else {
                if(InfsType(cn->CI)==tList){InitList(cn->CI);}
                cn->whatNext=doWorkList(cn->CI, cn->CIfol);
            }
           //cn->CI->wFlag|=nsWorkListDone;
        }
        // NOWDO: Notify-and-remove subscribers
    return 0;
}

infon* agent::normalize(infon* i, infon* firstID){
    infon* parent;
    if (i==0) return 0;
    QitemPtr Qi(new Qitem(i,firstID,(firstID)?1:0,0));
    infQ ItmQ; ItmQ.push(Qi);
    while (!ItmQ.empty()){
        QitemPtr cn=ItmQ.front(); ItmQ.pop(); infon* CI=cn->item;
        fetch_NodesNormalForm(cn);
        if((CI != i) && (parent=getTop(CI)) && (InfsType(parent)==tNum) && (InfsFormat(parent)==fConcat) && !InfIsTop(CI) && InfIsLiteralNum(CI) && InfIsLiteralNum(CI->prev) ){
            {}// combine with prev...
            // if(InfIsTop(Currnt) && InfIsBottom(Crrnt)) {/* Remove one layer */}
        }
        //wait?
        pushNextInfon(CI, cn, ItmQ);
    }
    return 0;
}

