///////////////////////////////////////////////////
// Proteus.cpp 10.0  Copyright (c) 1997-2011 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <strstream>
#include <iostream>
#include <fstream>
#include <sqlite3.h>
#include "Proteus.h"
#include "remiss.h"

const int ListBuffCutoff=20;  // TODO: make this '2' to show a bug.

typedef map<dblPtr,UInt>::iterator altIter;
multimap<infon*,WordSPtr> DefPtr2Tag;  // TODO: Verify that smartpointer in container here will be OK.

#include "xlater.h"
#include "XlaterENGLISH.h"
XlaterENGLISH EnglishXLater;
sqlite3 *coreDatabase;
xlater* fetchXlater(icu::Locale *locale){
    LanguageExtentions::iterator lang = langExtentions.find(locale->getBaseName());
    if (lang != langExtentions.end()) return lang->second;
    return 0;
}

LanguageExtentions langExtentions; // This map stores valid locales and their xlater if available.
int initializeProteusCore(char* resourceDir, char* dbName){     // Use this to load available language modules before normalizing any infons.
    // Connect to Proteus Database
    string dbPath=resourceDir; dbPath+="/"; dbPath+=dbName;
    int rc = sqlite3_open(dbPath.c_str(), &coreDatabase);
        if( rc ){
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(coreDatabase));
            sqlite3_close(coreDatabase);
            return(1);
        }
    // Initialize Language modules.
    u_setDataDirectory(resourceDir);
    EnglishXLater.loadLanguageData(coreDatabase);
    int numLocales;
    const icu::Locale* locale = icu::Locale::getAvailableLocales(numLocales);
    for(int loc=0; loc<numLocales; ++loc){
        string localeID=locale[loc].getBaseName();
        string localeLanguage=locale[loc].getLanguage();
        if     (localeLanguage=="en") {langExtentions[localeID]=&EnglishXLater;}  // English
        else if(localeLanguage=="fr") langExtentions[localeID]=0;  // French
        else if(localeLanguage=="es") langExtentions[localeID]=0;  // Spanish
        else if(localeLanguage=="zh") langExtentions[localeID]=0;  // Chinese
        else if(localeLanguage=="ar") langExtentions[localeID]=0;  // Arabic
        else if(localeLanguage=="ru") langExtentions[localeID]=0;  // Russian
        else if(localeLanguage=="de") langExtentions[localeID]=0;  // German
        else if(localeLanguage=="ja") langExtentions[localeID]=0;  // Japanese
        else if(localeLanguage=="it") langExtentions[localeID]=0;  // Italian
        else if(localeLanguage=="hi") langExtentions[localeID]=0;  // Hindi
        else if(localeLanguage=="he") langExtentions[localeID]=0;  // Hebrew
        else if(localeLanguage=="eu") langExtentions[localeID]=0;  // Basque
        else if(localeLanguage=="pt") langExtentions[localeID]=0;  // Portuguese
        else if(localeLanguage=="bn") langExtentions[localeID]=0;  // Bengali
        else langExtentions[localeID]=0;
    }
    return 0;
}

void resetLanguageData(){
    EnglishXLater.unloadLanguageData(); EnglishXLater.loadLanguageData(coreDatabase);

}

void shutdownProteusCore(){
    sqlite3_close(coreDatabase);
}

int calcScopeScore(string wrdS, string trialS){
    int score=1;
    uint wpStart=0, tpStart=0, wp, tp; // WordPos and trialPos
    wrdS+='&'; trialS+='&';
    for (wp=wrdS.find("&"),tp=trialS.find('&'); wp!=string::npos && tp!=string::npos; wp=wrdS.find("&",wp+1),tp=trialS.find('&',tp+1)){
        string WrdScopeSeg=wrdS.substr(wpStart,wp-wpStart), trialScopeSeg=trialS.substr(tpStart, tp-tpStart);
        if(WrdScopeSeg == trialScopeSeg) score+=2; else {return score-1;} // -1 if trial is defined in a sibling.
        wpStart=wp+1; tpStart=tp+1;
    }
    if(wp == string::npos && tp != string::npos) score-=1;
    return score;
}

infon* infon::isntLast(){ // 0=this is the last one. >0 = pointer to predecessor of the next one.
    if (!InfIsLast(this)) return this;
    infon* parent=getTop(this);
    if(SizeIsLiteral(parent)){ // TODO: Make this work if needed.
    //    cout<<"SIZE:"<<this->getSize()<<", POS:"<<this->pos<<"\n";
    //    int cmpNums=cmp(this->getSize(), (UInt)this->pos);
    //    if(cmpNums <= 0) return 0;
    }
    infon* gParent=getTop(parent);
    if(gParent && ValueIsConcat(gParent))
        return parent->isntLast();
    return 0;
}

infon* getParent(infon* i){
    for(i=getTop(i); i && ValueIsConcat(i) && (InfsType(i)!=tList); i=getTop(i)){}
    return i;
}

#define recAlts(lval, rval) {if(InfsType(rval)==tString) alts[dblPtr((char*)rval->value.dataHead->get_num_mpz_t()->_mp_d,lval)]++;}
#define fetchLastItem(lval, item) {for(lval=item; InfsType(lval)==tList;lval=lval->value->prev);}
#define fetchFirstItem(lval, item) {for(lval=item; InfsTYpe(lval)==tList;lval=lval->value){};}

int agent::loadInfon(const char* filename, infon** inf, bool normIt){
    cout<<"Loading:'"<<filename<<"'..."<<flush;
    fstream InfonIn(filename);
    if(InfonIn.fail()){cout<<"Error: The file "<<filename<<" was not found.\n"<<flush; return 1;}
    QParser T(InfonIn); T.agnt=this;
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

infon* agent::loadInfonFromString(string ProteusString, infon** inf, bool normIt){
    string entry="<%  " + ProteusString + " \n %>";
    istrstream InfonIn(entry.c_str());
    QParser T(InfonIn); T.agnt=this;
    *inf=T.parse();
    if ((*inf)==0) {cout<<"Error:"<<T.buf<<"   "<<flush; return 0;}
    if(normIt) {
        alts.clear();
        try{
          normalize(*inf);
        } catch (char const* errMsg){cout<<errMsg;}
    }
    return *inf;
}

int agent::StartPureTerm(pureInfon* varIn, infon** varOut){
    infon *tmp; int result;
    if((tmp=varIn->listHead)==0) {cout<<"ZERO\n";return 3;}
    if(!InfIsFirst(tmp)) {return 4;}
    while((InfsType(tmp)==tList) && (ValueIsConcat(tmp) || ((varIn->flags&(fEmbedSeq+fConcat+mType))==(fConcat+tList)))){
        result=StartTerm(tmp, &tmp); cout<<" r:"<<result<<"\n";
        if(result>0) {return result;}
        if(result==-1){
            result=getNextTerm(&tmp);
            if(result!=0) {return result;}
        } else break;
    }
    *varOut=tmp;
    return 0;
}

int agent::StartTerm(infon* varIn, infon** varOut) {
// Return 0 on success, -1 on End-Of-List, or a positive error code.
    if(varIn==0) return 1;
    if(InfsType(varIn)!=tList) return 2;
    if(((varIn->size.flags&(tNum+fLiteral))==(tNum+fLiteral) && varIn->size.dataHead->get_num()==0)||!varIn->value.listHead) return -1; //EOL
//    InitList(varIn);
    return StartPureTerm(&varIn->value, varOut);
}

int agent::LastTerm(infon* varIn, infon** varOut) {
    if (varIn==0) return 1;
    if (ValueIsConcat(varIn)) {varIn=varIn->value.listHead->prev;}
    else if(InfsType(varIn) != tList)  return 2;
    else {*varOut=varIn->value.listHead->prev; if (*varOut==0) return 3;}
    return 0;
}

int agent::getNextTerm(infon** p) {
// Return 0 on success, -1 on End-Of-List, or a positive error code.
  infon *parent, *Gparent=0; int result;
  if(InfIsBottom(*p)) {
    parent=getTop(*p);
    if (InfIsLast(*p)){
      if(parent==0){(*p)=(*p)->next; return 1;}
      Gparent=getTop(parent);//cout<<"item:"<<(*p)->value.dataHead->get_num()<<"parent->VsFlag"<<VsFlag(parent)<<"  Gparent="<<VsFlag(Gparent)<<"\n";
      if((InfsType(parent)==tList) && (ValueIsConcat(parent) || (Gparent && ((VsFlag(Gparent)&(fEmbedSeq+fConcat+mType))==(fConcat+tList))))){
        if((result=getNextTerm(&parent))!=0) return result;
        (*p)=parent; return 0;
      }
      return -1;
    } else {cout<<"WARN: Bottom-but-not-Last\n"; return -1;} //infon* slug=new infon; slug->wFlag|=nsBottomNotLast; append(slug, parent); (*p)=slug;} /*Bottom but not last, make slug*/
  } else {
    (*p)=(*p)->next;
    if ((*p)==0) {return 3;}
    parent=getTop((*p));
    if((InfsType(*p)==tList) && (ValueIsConcat(*p) || (parent && ((VsFlag(parent)&(fEmbedSeq+fConcat+mType))==(fConcat+tList))))){
        infon* tmp=*p;
        if((result=StartTerm(tmp, p))>0) {return result;}
        else if(result==-1){if((result=getNextTerm(p))!=0) return result;}
    }
   // migrateGetLastIdents(*p);
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
infon* entry;
///////////// Routines for copying, appending, etc.
infon* agent::append(infon** i, infon* list){ // appends an item to the end of a tentative list
    entry=*i;
    if(! (list && list->value.listHead && list->value.listHead->prev)) return 0;
    infon *last=list->value.listHead->prev;
    infon *p=last, *q;
    do {  // find the earliest-but-at-end tentative in list
        q=p; p=p->prev; // Shift left in list
    } while(p!=last && InfIsVirtTent(p));
    if(!(q && InfIsVirtTent(q))) return 0;
    if(InfIsVirtual(q)) processVirtual(q);
    copyTo(*i, q);
    if(q->value.listHead) q->value.listHead->top=q;
    if(q->size.listHead ) q->size.listHead->top=q;
    q->spec1=(*i)->spec1; q->spec2=(*i)->spec2; q->wrkList=(*i)->wrkList;
// TODO: update master fields in wrkList nodes.
       // q->wrkList->master=q; // Like this, but loop over all of them...
    ResetVirtTent(q);
    if(q->type && q->type->norm!="") (*list->index)[q->type->norm]=q;
    *i=q;
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
    top->prev=follower; top->wFlag|=from->wFlag & mListPos;
    follower->next=top;
    return top;
}

void agent::deepCopyPure(pureInfon* from, pureInfon* to, int flags){
    to->flags=from->flags;
    to->offset=from->offset;
    if(PureIsInListMode(*from)) to->listHead=copyList(from->listHead, flags);
    else {to->listHead=0; to->dataHead=from->dataHead;}
}

void agent::deepCopy(infon* from, infon* to, PtrMap* ptrs, int flags){
    UInt fm=from->wFlag&mFindMode; infon* tmp;
    to->wFlag=(from->wFlag&0xffffffff);
    if(to->type==0) {  // TODO: We should merge types, not just copy over.
        if(from->type){ //cout<<"TAG:"<<from->type->tag<<"\n";
            to->type=from->type;
        }
    }

    deepCopyPure(&from->size, &to->size, flags);  if(to->size.listHead) to->size.listHead->top=to;
    deepCopyPure(&from->value,&to->value,flags); if(to->value.listHead)to->value.listHead->top=to;

    if(from->wFlag&mIsHeadOfGetLast) to->top2=(*ptrs)[from->top2];
    if(fm==iToPath || fm==iToPathH || fm==iToArgs || fm==iToVars) {
        to->spec1=from->spec1;
        if (ptrs) to->top=(*ptrs)[from->top];
    } else if(fm>=iGetLast && from->spec1) {
        PtrMap *ptrMap = (ptrs)?ptrs:new PtrMap;
        if ((to->prev) && (tmp=to->prev->spec1) && (tmp->wFlag&mIsHeadOfGetLast) && (tmp=tmp->next)) to->spec1=tmp;
        else {to->spec1=new infon; }
        (*ptrMap)[from]=to;
        deepCopy (from->spec1, to->spec1, ptrMap, flags);
        if(ptrs==0) delete ptrMap;
        if (flags && from->wFlag&mAssoc) {
            from->wFlag&= ~(mAssoc+mFindMode); from->wFlag|=iAssocNxt;
            SetValueFormat(from, fUnknown); SetSizeFormat(from, fUnknown);
        }
    }

    if ((from->wFlag&mFindMode)==iAssocNxt) {to->spec2=from;}  // Breadcrumbs to later find next associated item.
    else if(!from->spec2) to->spec2=0;
    else {to->spec2=new infon; deepCopy(from->spec2, to->spec2,0,flags);}

    to->attrs=from->attrs;
    to->index=from->index;

    infNode *p=from->wrkList, *q;  // Merge Identity Lists
    if(p) do {
        q=new infNode; q->master=p->master; q->idFlags=p->idFlags;
        if(1||(p->item->wFlag&mFindMode)<iAssocNxt && (p->item->wFlag&mFindMode)>iNone) {
            q->item=new infon; q->master=p->master; q->idFlags=p->idFlags; deepCopy(p->item,q->item,ptrs,flags);
        } else {q->item=p->item;}
        prependID(&to->wrkList, q);
        p=p->next;
    } while (p!=from->wrkList);
}

void closeListAtItem(infon* lastItem){ // remove (no-longer valid) items to the right of lastItem in a list.
    // TODO: Will this work when a list becomes empty?
    infon *itemAfterLast, *nextItem; UInt count=0;
    if(lastItem->next==0){
        if(lastItem->pos!=0) lastItem=(infon*)lastItem->pos;
        else {cout<<"Item not in a list but being closed. It's a bug. Please report it.\n"; exit(1);}
    }
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
    UInt tmpFlags=v->wFlag&mListPos;
    if (spec){
        if((spec->wFlag&mFindMode)==iAssocNxt) {
            copyTo(spec, v); SetSizeType(v,tNum); SetSizeFormat(v,fUnknown); VsFlag(v)=fUnknown; v->spec1=spec->spec2;
        } else {deepCopy(spec, v,0,1); }
        if(posArea=='?' && v->spec1) posArea= 'N';
    }
    v->wFlag|=tmpFlags;
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

void agent::migrateGetLastIdents(infon *i){
    infon* parent=getTop((i));
    if (InfIsLast(i)){     // If isLast && Parent is spec1-of-get-last, get parent, apply any idents
        if(parent->wFlag&mIsHeadOfGetLast) {
            parent=parent->top2;
            infNode *j=parent->wrkList, *k;  // Merge Identity Lists
            if(j) do {
                k=new infNode; k->item=new infon;  k->master=j->master; k->idFlags=j->idFlags;
                deepCopy (j->item, k->item);
                appendID(&(i)->wrkList, k);
                j=j->next;
            } while (j!=parent->wrkList);
        }
    }
}

int agent::getFollower(infon** lval, infon* i){
    int levels=0;
    if(!i) return 0;
    gnsTop: if(i->next==0) {*lval=0; return levels;}
    if(InfIsLast(i) && i->prev){
        i=getTop(i); ++levels;
        if(i) goto gnsTop;
        else {*lval=0; return levels;}
    }//else if(InfIsBottom(i)) throw "Bottom found but not last when getting follower.";
    *lval=i->next;
    migrateGetLastIdents(*lval);
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

void agent::AddSizeAlternate(infon* Lval, infon* Rval, infon* Pred, UInt Size, infon* Last, UInt Flags, infon* master) {
    infon *copy, *copy2, *LvalFol; pureInfon size(Size); SetBits(size.flags, mFormat, Lval->size.flags&mFormat);
    copy=new infon(Lval->wFlag, &size, &Lval->value ,0, Lval->spec1,Lval->spec2,Lval->next); // TODO B4: verify correct size and val usage
    copy->prev=Lval->prev; copy->top=Lval->top;copy->pred=Pred; copy->type=Lval->type;
    insertID(&Lval->wrkList,copy,ProcessAlternatives|Flags,master); Lval->wrkList->slot=Last; Lval->wFlag&=~nsWorkListDone;
    getFollower(&LvalFol, Lval);
    if (LvalFol){
        copy2=new infon(LvalFol->wFlag, &LvalFol->size, &LvalFol->value ,0,LvalFol->spec1,LvalFol->spec2,LvalFol->next); // TODO B4: verify correct size and val usage
        copy2->top=LvalFol->top; copy2->prev=LvalFol->prev; copy2->type=LvalFol->type; copy2->pred=copy;
        insertID(&copy2->wrkList,Rval,Flags,master); insertID(&LvalFol->wrkList,copy2,ProcessAlternatives|Flags,master); LvalFol->wFlag&=~nsWorkListDone;
    }
}

void agent::addIDs(infon* Lvals, infon* Rvals, UInt flags, int asAlt, infon* master){
    const int maxAlternates=100;
    if(Lvals==Rvals) return;
    if(!(flags&mLooseType)) {insertID(&Lvals->wrkList,Rvals,flags,master); Lvals->wFlag&=~nsWorkListDone; return;}
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
                insertID(&Lvals->wrkList,Rvals,flags,master);
                Lvals->wFlag&=~nsWorkListDone;
                recAlts(Lvals,Rvals);
            } else {
                Rval=RvlLst[i];
                AddSizeAlternate(crntAlt, Rval, pred, size, (prev)?prev->prev:0, flags, master);
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
        if(findMaster->wFlag&mIsHeadOfGetLast) findMaster=findMaster->top2;
        infon* nxtParent=getTop(findMaster);
        if(nxtParent && InfIsLoop(nxtParent)) return findMaster;
    }
    return 0;
}

int agent::checkTypeMatch(WordSPtr LType, WordSPtr RType){
    return LType->norm==RType->norm;
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
                newID=getHead(CI);
                for(int s=1; s<(UInt)CI->spec1; ++s) {  // for  backslashes-1 go to parent
                    newID=getParent(newID); if (newID==0 ) {cout << "Too many '\\'s in "<<printInfon(CI)<< '\n';}
                }
                if(CIFindMode==iToPathH) {  // If no '^', move to first item in list.
                    if(!InfIsTop(newID)) {newID=newID->top; }
                    if (newID==0) {cout<<"Zero TOP in "<< printInfon(CI)<<'\n'; exit(1);}
                 //   else if(!InfIsFirst(newID)) {newID=0; cout<<"Top but not First in "<< printInfon(CI)<<'\n';}
                }
                if(newID) {CI->wFlag|=mAsProxie; CI->value.proxie=newID; newID->wFlag|=isNormed; CI->wFlag&=~mFindMode; newID=0;}
                cn->doShortNorm=true;
            } break;
            case iTagDef: break; // Reserved
            case iTagUse: {
                if(CI->type == 0) throw ("A tag was null which is a bug");
//                OUT("Recalling: "<<CI->type->norm<<":"<<CI->type->locale);
                infon* found=CI->findTag(*CI->type);
                if (found) {
                    bool asNotFlag=((CI->wFlag&asNot)==asNot);
//                    CI->type=0;
                    UInt tmpFlags=CI->wFlag&mListPos; deepCopy(found,CI,0,0); CI->wFlag|=tmpFlags;
                    if(CI->wFlag&asNot) asNotFlag = !asNotFlag;
                    SetBits(CI->wFlag, asNot, (asNotFlag)?asNot:0);
                    deTagWrkList(CI);
        CI->updateIndex();
                } else{OUT("\nBad tag:'"<<CI->type->norm<<"'\n");throw("A tag was used but never defined");}
                break;}
            case iGetFirst:  StartTerm (CI, &newID); break;
            case iGetMiddle: break; // TODO: iGetMiddle
            case iGetLast:


                if(CI->wFlag&xOptmize1){  // Optimized way to look up <type>
                    if(CI->wFlag&xDevToHome){break;} // This is beign recorded, not looked up. TODO: Later, let it go below to refine it's position.
                    string tag=CI->spec1->value.listHead->prev->type->norm;
                    infNode* wrkNode=CI->spec1->wrkList;
                    if(wrkNode==0) throw "<type> not attached to a list to search";
                    infon* infToSearch=wrkNode->next->item;

                    QitemPtr Qi(new Qitem(infToSearch));
                    fetch_NodesNormalForm(Qi);
                    if (infToSearch->wFlag&mAsProxie) {infToSearch=infToSearch->value.proxie;}
                    else if(wrkNode->idFlags&c1Right){
                        if(infToSearch->spec1){infToSearch=infToSearch->spec1;} else throw "Right side was not a reference ";
                    }
                    if(infToSearch->index==0) {cout<<"No index for tag '"<<tag<<"'\n"; exit(1);}
                    infonIndex::iterator itr=infToSearch->index->find(tag);
                    if(itr!=infToSearch->index->end()) newID=itr->second;
                    else {cout<<"'"<<tag<<"' not found in index "<<infToSearch->index.get()<<".\n"<<world->index.get()<<"\n"; exit(2);}
            cout <<"note: alts in iGetAuto not copied. ["<<tag<<"]\n";
                break;
                }



      ///////////////// Below is the unchanged, working code. Above is experimental search optimization code.

                if (SizeIsInverted(CI->spec1)){ // TODO: This block is a hack to make simple backward references work. Fix for full back-parsing.
                    normalize(CI->spec1, cn->firstID);
                    newID=CI;
cout<<"NEGATIVE INDEX: "<<CI->spec1->getSize().get_ui()<<"\n";
                    for(UInt i=(UInt)CI->spec1->getSize().get_ui(); i>0; --i){newID=newID->prev;}
                    {insertID(&CI->wrkList, newID,0,CI); if(ValueIsUnknown(CI)) {CI->size=newID->size;} cpFlags(newID, CI,~mListPos);}
  //                  SetValueFormat(CI, fUnknown);
                } else{
                    normalize(CI->spec1, cn->firstID);
                    if( ! CI->spec1->isntLast ()) CI->wFlag|=(isBottom+isLast);
                    if(InfHasAlts(CI->spec1)) {  // migrate alternates from spec1 to CI... TODO: Later, build this into LastTerm.
                        infNode *wrkNode=CI->spec1->wrkList; infon* item=0;
                        if(wrkNode)do{
                            wrkNode=wrkNode->next; item=wrkNode->item;
                            if(((wrkNode->idFlags&(ProcessAlternatives+NoMatch))==ProcessAlternatives)&& (item->wFlag & mIsHeadOfGetLast)){
                                if(*wrkNode->item->size.dataHead == 0){} // TODO: handle the null alternative
                                else {insertID(&CI->wrkList,wrkNode->slot,ProcessAlternatives+isRawFlag,CI);}
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
                    cout<<"LAST:"<<printInfon(CI)<<"\n";
                    closeListAtItem((masterItem)?masterItem : getMasterList(CI));
                }
            }
            insertID(&CI->wrkList, newID,(CIFindMode>=iAssocNxt)?skipFollower:0, CI);
            VsFlag(CI)&=~mType; CI->wFlag&=~mFindMode;
        }
    }
}

enum rejectModes {rAccept=0, rReject, rNullable, rInvertable};

int agent::doWorkList(infon* ci, infon* CIfol, int asAlt, int CIFolLvl){
    // PARSE: if no worknode, and stream is '=='...
    infNode *wrkNode=ci->wrkList; infon *item, *IDfol, *tmp, *theOne=0; Qitem cn;
    UInt altCount=0, ItemLevel, tempRes, isIndef=0, result=DoNothing, f, looseType, noNewContent=true;
    if(CIfol && !CIfol->pred) CIfol->pred=ci;
    if(wrkNode)do{
        wrkNode=wrkNode->next; item=wrkNode->item;
        bool cpySize=0, cpyValue=0, resetCIsTentative=0, linkCIFol=false, invertAcceptance=((ci->wFlag&asNot) ^ (item->wFlag&asNot)); int reject=rAccept;
        if (wrkNode->idFlags&skipFollower) CIfol=0;
        switch (wrkNode->idFlags&WorkType){
        case InitSearchList: cout<<"InitSrchList"<<printInfon(ci)<<"\n"; // E.g., {[....]|...}::={3 4 5 6}
            tmp=new infon();
            tmp->top2=ci; tmp->value.listHead=ci->value.listHead->spec1;
            {tmp->size=ci->size; cpFlags(ci,tmp,0xff0000);} VsFlag(tmp)|=(fLoop+fUnknown+tList); // TODO: Should fUnknown really be here?
            insertID(&tmp->wrkList, item, MergeIdent, wrkNode->master);  // Appending the {3 4 5 6}
            tmp->value.listHead->next = tmp->value.listHead->prev = tmp->value.listHead;
            tmp->value.listHead->wFlag|=(isTop+isBottom+isFirst+isVirtual);
            tmp->value.listHead->top=tmp;
            insertID(&tmp->value.listHead->wrkList, item->value.listHead, MergeIdent, wrkNode->master);
            processVirtual (tmp->value.listHead);
            tmp->value.listHead->next->size=(mpq_class)2;
            result=DoNext; noNewContent=false;
            break;
        case ProcessAlternatives: cout<<"PROCESS_ALTS"<<printInfon(ci)<<"\n";
            if (altCount>=1) break; // Don't keep looking after found
            if(wrkNode->idFlags&isRawFlag){
              tmp=new infon(ci->wFlag,&ci->size, &ci->value,0,ci->spec1,ci->spec2,ci->next);
              tmp->prev=ci->prev; tmp->top=ci->top; tmp->type=ci->type; tmp->pos=ci->pos;
              insertID(&tmp->wrkList, item,0, wrkNode->master);
              wrkNode->item=item=tmp;
              wrkNode->idFlags^=isRawFlag;
            }
            wrkNode->idFlags|=NodeDoneFlag;
            cn.doShortNorm=0; cn.override=0;
            prepWorkList(item, &cn);
            tempRes=doWorkList(item, CIfol,1, CIFolLvl); if(result<tempRes) result=tempRes;
            if (tempRes==BypassDeadEnd) {wrkNode->idFlags|=NoMatch; break;}
            altCount++;  theOne=item;  noNewContent=false;
            break;
        case (ProcessAlternatives+NodeDoneFlag):
            altCount++;  theOne=item; result=DoNext; noNewContent=false;
            break;
        case MergeIdent:
            wrkNode->idFlags|=SetComplete; IDfol=0;
            if(!InfIsNormed(item)) { // Norm item. esp %W, %//^
                if(item->top==0) {item->top=ci->top; item->wFlag|= (ci->wFlag & isTop); cout<<"NEW TOP. NEXT="<<item->next<<"\n";}
                QitemPtr Qi(new Qitem(item));
                fetch_NodesNormalForm(Qi); item->wFlag|=isNormed;
                if(Qi->whatNext!=DoNextBounded) noNewContent=false;
                if (item->wFlag&mAsProxie) {item=item->value.proxie;}
                else if(wrkNode->idFlags&c1Right){
                    if(item->spec1){item=item->spec1;}
                    else throw "Tag wasn't a reference ";
                }
            }
            if(wrkNode->idFlags&OverrideIdent){SetBits(ci->size.flags, (mType), tUnknown);} // also, clear everything else necessary
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
                        if (!ValueIsKnown(item) || (infValueCmp(item, ci)==0)) cpySize=true; // Override a previous alt's size if size was not definite.
                        else {reject=rNullable; break;}
                    }
                    if(ItemsType!=tUnknown){
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
                            if ((infTypes== tNum+4*tNum) || (infTypes== tString+4*tString )){if (ValueIsConcat(item)) cout << "NUM CONCAT\n";
                                if(!cpySize && SizeIsKnown(item) && SizeIsKnown(ci) && infonSizeCmp(ci,item)>0) {reject=rNullable; break;}
                                if(ValueIsUnknown(ci)) {cpyValue=true;}
                                else if (((cpySize)?infValueCmp(item, ci):infValueCmp(ci, item))!=0) {reject=rInvertable; break;}
                            }
                            else if (infTypes== tList+4*tList){
                                linkCIFol=false;
                                if(ci->value.listHead) {
                                    if(!InfIsTentative(item->value.listHead)) ResetTent(ci->value.listHead); // See note below ("This is used...")
                                    if(looseType) addIDs(ci->value.listHead, item->value.listHead, looseType, asAlt, wrkNode->master);
                                    else{
                                        insertID(&ci->value.listHead->wrkList,item->value.listHead,0, wrkNode->master);
                                        ci->value.listHead->wFlag&=~nsWorkListDone;
                                    }
                                } else {ci->value=item->value; cpFlags(item,ci,0xff);if(SizeIsKnown(ci) && *ci->size.dataHead==0){IDfol=item;} else item->value.listHead->top=ci;}
                            }
                        } else { // Copy back to item if item is a ref to an infon that was found via >= iGetLast.
                            // TODO: What about partial knowns and more complex situations?
                            if(ValueIsKnown(ci)){ // Copy when refered-to-item is written.
                                item->value=ci->value;
                                if(!item->subscriptions.empty()) item->fulfillSubscriptions(this);
                            }else ci->subscribeTo(item); // Subscribe to item's value;
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
                case tNum+4*tList: InitList(item); addIDs(ci,item->value.listHead,looseType, asAlt, wrkNode->master); linkCIFol=false; result=DoNext;break; // this should probably be insertID not addIDs. Check it out.

                case tList+4*tUnknown:
                case tList+4*tString:
                case tList+4*tNum:
                    if(ci->value.listHead){addIDs(ci->value.listHead, item, looseType, asAlt, wrkNode->master);}
                    else{copyTo(item, ci);}
                    result=DoNext;
            }
            if (invertAcceptance) {
                if(reject==rInvertable){reject=rAccept; result=DoNext; linkCIFol=true;}
                else { reject=rNullable; linkCIFol=false;}
            } else if (!reject){ // Not inverted acceptance
                if (cpySize)  ci->size=item->size;
                if (cpyValue) {ci->value=item->value; if(item->top==0 && ci->value.listHead) ci->value.listHead->top=ci;} //  do we ever need to also copy wFlags and type fields?
                if (resetCIsTentative) ResetTent(ci);
            }
            if(linkCIFol && CIfol && CIFolLvl!=0){ cout<<"AT-1\n"; // Here is where we connect any remainder-of-item or item's follower to CI's follower.
                if(infonSizeCmp(ci,item)==0 || (infTypes!= tString+4*tString)){ cout<<"At-2\n";
                    ItemLevel=((IDfol)?0:getFollower(&IDfol, item));
                    if(IDfol){if(ItemLevel==0) addIDs(CIfol, IDfol, looseType, asAlt, wrkNode->master); else {reject=rReject;}}
                    else {
                        if( (infTypes!= tList+4*tList)) {cout<<"AT-3\n";// temporary hack but it works ok.
                            // If there is no IDfol, perhaps ci is the last item in its list.
                            if ((tmp=ci->isntLast()) && InfIsTentative(tmp->next)) {cout<<"AT-4\n";reject=rNullable;}
    if(tmp) cout<<"tmp1:"<<tmp<<" ("<<printInfon(tmp)<<"> tmp->next:"<<tmp->next<<"\n";
    cout<<"ITEM:"<<printInfon(item)<<" item->top:"<<item->top<<" ci:"<<printInfon(ci)<<"\n";
    cout<<"Item's master:"<<printInfon(wrkNode->master)<<"\n";
                            if(!tmp || !SizeIsKnown(getTop(tmp)))
                                if ((tmp=getMasterList(ci) )){ cout<<"tmp2:"<<tmp<<" ("<<printInfon(tmp)<<" tmp->top:"<<tmp->top<<"\n";
                                    if(getTop(tmp)==wrkNode->master)
                                        // The state "end of item-list" (i.e., there is no IDfol) can signify the end of a loop-list
                                        //  but only if the item originated as an rvalue of the loop ending (i.e., it's "master")
                                        {cout<<"getTop(tmp):"<<tmp<<"\n"; closeListAtItem(tmp); if(!reject) reject=rReject; }
                                }
                        }
                    }
                } else if((infTypes== tString+4*tString) && infonSizeCmp(ci,item)<0){ cout<<"EXTRA String\n";
                    BigInt cSize=ci->getSize();
                    pureInfon pSize(item->getSize()-cSize);
                    pureInfon pVal(item->value.dataHead, item->value.flags, item->value.offset+cSize);
                    tmp=new infon(item->wFlag, &pSize, &pVal,0,0,item->next);
                    addIDs(CIfol, tmp, looseType, asAlt, wrkNode->master);
                }
            }
            if(item->next==0 && ci->next!=0) item->pos=(UInt)ci; // Special use of pos. When item isn't in a list but can signal EOL for ci. So we must know ci's address.
            if (reject){ cout<<"REJECT "<<CIfol<<"\n";
                result=BypassDeadEnd;
                if(reject >= rNullable){
                    infon* CA=getTop(ci); if (CA) {AddSizeAlternate(CA, item, 0, ((UInt)ci->next->pos)-1, ci, looseType, wrkNode->master); }
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
ci->updateIndex(); // TODO: This should be optimized into merge of values.
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
        if(!(InfAsDesc(CI)&&!cn->override)&&((CI->value.listHead&&(InfsType(CI)==tList))||ValueIsConcat(CI)) && !InfIsNormed(CI->value.listHead)){
            ItmQ.push(QitemPtr(new Qitem(CI->value.listHead,cn->firstID,((cn->IDStatus==1) & !ValueIsConcat(CI))?2:cn->IDStatus,cn->level+1,0,cn))); // Push CI->value
        }else if (cn->CIfol){cout<<printInfon(cn->CIfol)<<"\n"; pushCIsFollower;}
        break;
    case BypassDeadEnd:  {cn->nxtLvl=getFollower(&cn->CIfol,getTop(CI))+1; pushCIsFollower;} break;
    case DoNothing: break;
    }
}

int agent::fetch_NodesNormalForm(QitemPtr cn){
        int result=0; cn->CI=cn->item;
        if(cn->item->wFlag&nsBottomNotLast) return 0;
        if(!(cn->item->wFlag&nsNormBegan)){cn->CI=cn->item; cn->CIfol=0; cn->doShortNorm=0; cn->item->wFlag|=nsNormBegan;}
        infon* CI=cn->CI;
        if(!(CI->wFlag&nsPreNormed)){
            result=0; prepWorkList(CI, &*cn); // NOWDO: make prepWorkList return something
            if(result>0) return result;
            CI->wFlag|=nsPreNormed;
            if (cn->doShortNorm) CI->wFlag|=nsWorkListDone;
            if((CI->wFlag&mFindMode)==0 && cn->IDStatus==2)
                {cn->IDStatus=0; insertID(&CI->wrkList,cn->firstID,0, CI);}
        }

        if(!(CI->wFlag&nsWorkListDone)){
            if(!cn->CIfol) cn->nxtLvl=getFollower(&cn->CIfol, CI);
            if(InfAsDesc(CI) && !cn->override) {cn->whatNext=DoNext;}
            else {
                if(InfsType(CI)==tList){InitList(CI);}

                // Why do these next three lines work? Comprehensively document all the ways to determine EOL
                //   then ensure that logic is followed here and in doWorkList.
                int CIFolLvl=(cn->level-cn->nxtLvl);
                infon* prnt=(cn->CIfol)?getTop(cn->CIfol):0;
                if(!(prnt && !InfIsLoop(prnt))){CIFolLvl=1;}
                cn->whatNext=doWorkList(CI, cn->CIfol, 0, CIFolLvl);
            }
           //CI->wFlag|=nsWorkListDone;
        }
        if(!CI->subscriptions.empty()) CI->fulfillSubscriptions(this);
    return 0;
}

infon* agent::normalize(infon* i, infon* firstID){
//cout<<printInfon(entry)<<"\n";
    if (i==0) return 0;
    QitemPtr Qi(new Qitem(i,firstID,(firstID)?1:0,0));
    infQ ItmQ; ItmQ.push(Qi);
    while (!ItmQ.empty()){
        QitemPtr cn=ItmQ.front(); ItmQ.pop(); infon* CI=cn->item;
        fetch_NodesNormalForm(cn);
        pushNextInfon(CI, cn, ItmQ);
        // Below: handle numeric concats. e.g.: (3 4 5)
        infon *p, *n, *parent;
        while((CI != i) && (parent=getTop(CI)) && (InfsType(parent)==tNum) && (InfsFormat(parent)==fConcat) && !InfIsTop(CI) && (InfsType(CI)==tNum) && isGivenNumber(CI->prev) ){
            if(isGivenNumber(CI)){// Combine with previous...
                p=CI->prev; n=CI->next;
                p->next=n; n->prev=p; if(n->pred==CI) n->pred=p;
                p->join(CI);
                p->wFlag|=(CI->wFlag&(isLast+isBottom));
                if(InfIsFirst(p) && p->next==p) {// Remove one layer
                    SetValueFormat(parent, fLiteral);
                    parent->value=p->value; parent->size=p->size;
                }
            } else {} // TODO: subscribe
            CI=parent;
        }
    }
    return 0;
}
