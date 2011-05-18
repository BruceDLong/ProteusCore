///////////////////////////////////////////////////
// Proteus.cpp 10.0  Copyright (c) 1997-2008 Bruce Long
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
    along with The Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <fstream>
#include "Proteus.h"
#include "Functions.h"
#include "remiss.h"

infon* World;
std::map<stng,infon*> tag2Ptr;
std::map<infon*,stng> ptr2Tag;

std::map<dblPtr,UInt> alts;
typedef std::map<dblPtr,UInt>::iterator altIter;

inline int agent::StartTerm(infon* varIn, infon** varOut) {
  infon* tmp;
  if (varIn==0) return 1;
  int varInFlags=varIn->flags;
  if (varInFlags&fConcat){
    if ((*varOut=varIn->value)==0) return 2;
    do {
      switch(varInFlags&tType){
        case tUnknown: *varOut=0; return -1;
        case tUInt: case tString: return 0;
        case tList: if(StartTerm(*varOut, &tmp)==0) {
          *varOut=tmp;
          return 0;
          }
        }
      if (getNextTerm(varOut)) return 4;
    } while(1);
  }
  if((varInFlags&mRepMode)==toWorldCtxt && varIn->spec1==(infon*)miscFindAssociate)
		{ ((assocInfon*)(varIn->spec2))->nextRef=0;  *varOut=varIn; return 0;}
  if ((varIn->flags&tType)!=tList) {return 3;}
  else {*varOut=varIn->value; if (*varOut==0) return 4;}
  return 0;
}

inline int agent::LastTerm(infon* varIn, infon** varOut) {
  if (varIn==0) return 1;
  if (varIn->flags&fConcat) {varIn=varIn->value->prev;}
  else if((varIn->flags&tType)!=tList)  return 2;
  else {*varOut=varIn->value->prev; if (*varOut==0) return 3;}
  return 0;
}

inline int agent::getNextTerm(infon** p) {
  infon *parent, *Gparent=0, *GGparent=0;
  if((*p)->flags&isBottom) {
    if ((*p)->flags&isLast){
      parent=((*p)->flags&isFirst)?(*p)->top:(*p)->top->top;
      if(parent==0){(*p)=(*p)->next; return 1;}
      if(parent->top) Gparent=(parent->flags&isFirst)?parent->top:parent->top->top;
      if(Gparent && (Gparent->flags&fConcat)) {
        if(Gparent->top) GGparent=(Gparent->flags&isFirst)?Gparent->top:Gparent->top->top;
        do {
          if(getNextTerm(&parent)) return 4;
// TODO: the next line fixes one problem but causes another: nested list-concats.
// test it with:  (   ({} {"X", "Y"} ({} {9,8} {7,6}) {5} ) {1} {{}} {2, 3, 4} )
          if(GGparent && (GGparent->flags&fConcat)) {*p=parent; return 0;}
//	  normalize(parent);
          switch(parent->flags&tType){
            case tUnknown: (*p)=0; return -1;
            case tUInt: case tString: throw("Item in list concat is not a list.");
            case tList: if(!StartTerm(parent, p)) return 0;
            }
        } while (1);
      }
      return 2;
    } else {return 5;} /*Bottom but not end, make subscription*/
  }else {
    (*p)=(*p)->next;
    if ((*p)==0) {return 3; }
  }
  return 0;
}

// TODO: This function is out-of-date but not used yet anyhow.
inline int agent::getPrevTerm(infon** p) { gpt1:
  if((*p)->flags&isTop)
    if ((*p)->flags&isFirst) {}
//      if ((*p)!=(*p)->first->size){(*p)=(*p)->prev; return 1;}
//      else {(*p)=(*p)->first->first; goto gpt1;}
          else {return 2;} /*Bottom but not end, make subscription*/
  else {
    (*p)=(*p)->prev;
gpt2: //if (((((*p)->flags&mFlags1)>>8)&rType)==rList)
    if ((*p)->size==0) {goto gpt1;}
    else {(*p)=(*p)->size->prev; goto gpt2;}
    if((*p)==0) return 3;
  }
  return 0;
}

void agent::append(infon* i, infon* list){ // appends an item to the end of a list
	i->next=i->top=list->value; i->prev=i->next->prev; i->next->prev=i;
	i->prev->flags^=isBottom; i->flags|=isBottom;
//	signalSubscriptions();
}

infNode* agent::copyIdentList(infNode* from){
    infNode* top=0; infNode* follower; infNode* p=from; infNode* q;
    if(from==0)return 0;
    do {
        q=new infNode; q->item=new infon; q->idFlags=from->idFlags;
        deepCopy(p->item,q->item);
        if (top==0) follower=top=q;
        follower=follower->next=q;
        p=p->next;
    } while(p!=from);
    follower->next=top;
    return top;
}
infon* agent::copyList(infon* from){
    infon* top=0; infon* follower; infon* p=from;
    if(from==0)return 0;
    do {
        infon* q=new infon;
        deepCopy(p,q);
        if (top==0) follower=top=q;
        q->prev=follower; q->top=top; follower=follower->next=q;
        p=p->next;
    } while(p!=from);
    top->prev=follower; top->flags|=from->flags&mListPos;
    follower->next=top;
    return top;
}

#define recAlts(lval, rval) {if((rval->flags&tType)==tString) alts[dblPtr((char*)rval->value,lval)]++;}
#define getTop(item) (((item->flags&isTop)||item->top==0)? item->top : item->top->top)
#define fetchLastItem(lval, item) {for(lval=item;(lval->flags&tType)==tList;lval=lval->value->prev);}
#define fetchFirstItem(lval, item) {for(lval=item;(lval->flags&tType)==tList;lval=lval->value){};}
#define cpFlags(from, to) {to->flags=(to->flags&0xff000000)+(from->flags&0x00ffffff);}
#define copyTo(from, to) {if(from!=to){to->size=from->size; to->value=from->value; cpFlags(from,to);}}
#define pushCIsFollower {int lvl=cn.level-nxtLvl; if(lvl>0) ItmQ.push(Qitem(CIfol,0,0,lvl));}
#define AddSizeAlternate(Lval, Rval, Pred, Size, Last) {   infon *copy, *copy2, *LvalFol;    \
        copy=new infon(Lval->flags,(infon*)(Size),Lval->value,0,Lval->spec1,Lval->spec2,Lval->next); \
        copy->prev=Lval->prev; copy->top=Lval->top;copy->pred=Pred; \
        insertID(&Lval->wrkList,copy,ProcessAlternatives); Lval->wrkList->slot=Last;\
        getFollower(&LvalFol, Lval);                  \
        if (LvalFol){\
            copy2=new infon(LvalFol->flags,LvalFol->size,LvalFol->value,0,LvalFol->spec1,LvalFol->spec2,LvalFol->next);\
            copy2->pred=copy; insertID(&copy2->wrkList,Rval,0); insertID(&LvalFol->wrkList,copy2,ProcessAlternatives);}}

infon* getVeryTop(infon* i){
	infon* j=i;
	while(i!=0) {j=i; i=getTop(i);}
	return j;
}

void agent::deepCopy(infon* from, infon* to, infon* args){
    UInt fm=from->flags&mRepMode;
    to->flags=from->flags;
    if(((from->flags>>goSize)&tType)==tList || ((from->flags>>goSize)&fConcat)){to->size=copyList(from->size); if(to->size)to->size->top=to;}
    else to->size=from->size;
    if((from->flags&tType)==tList || ((from->flags)&fConcat)){to->value=copyList(from->value); if(to->value)to->value->top=to;}
    else to->value=from->value;
    to->wrkList=copyIdentList(from->wrkList);
    if(fm==toHomePos) to->spec1=from->spec1;
    else if(fm==asTag) to->spec1=from->spec1;
    else if((UInt)(from->spec1)<=20) to->spec1=from->spec1;
    else if(fm<(UInt)asFunc){
		to->spec1=new infon; copyTo(from->spec1,to->spec1);
		if(to->prev && to->prev!=to){
			infon* spec2=to->prev->spec2;
			if (spec2 && spec2->next==0 && (UInt)(spec2->prev)>1){
				to->spec1->value=spec2->prev;
				to->spec1->size=(infon*)((UInt)to->prev->spec1->size - (UInt)spec2->size);
			}
		}
    } else {to->spec1=new infon; deepCopy((args)?args:from->spec1,to->spec1);}
    if(from->spec2){
	    	if(fm==toWorldCtxt && from-> spec1==(infon*)miscFindAssociate){
			to->spec2=from->spec2;
    		}else {to->spec2=new infon; deepCopy(from->spec2, to->spec2);}
	}else to->spec2=0;
}

void recover(infon* i){ delete i;}

void closeListAtItem(infon* lastItem){ // remove (no-longer valid) items to the right of lastItem in a list.
	// TODO: Will this work when a list becomes empty?
	infon *itemAfterLast, *nextItem; UInt count=0;
	for(itemAfterLast=lastItem->next; !(itemAfterLast->flags&isTop); itemAfterLast=nextItem){
		nextItem=itemAfterLast->next;
		recover(itemAfterLast);
	}
	lastItem->next=itemAfterLast; itemAfterLast->prev=lastItem; lastItem->flags|=(isBottom+isLast);
	// remove any tentative flags then set parent's size
	do{
		itemAfterLast->flags &=~ isTentative;
		itemAfterLast=itemAfterLast->next;
		count++;
	} while (!(itemAfterLast->flags&isTop));
	itemAfterLast->top->size=(infon*)count;
	itemAfterLast->flags &=~(fUnknown<<goSize);
}

const int simpleUnknownUint=((fUnknown+tUInt)<<goSize) + fUnknown+tUInt+asNone;
char isPosLorEorGtoSize(UInt pos, infon* item){
    if(item->flags&(fUnknown<<goSize)) return '?';
    if(((item->flags>>goSize)&tType)!=tUInt) throw "Size as non integer is probably not useful, so not allowed.";
    if(item->flags&(fConcat<<goSize)){
   	infon* size=item->size;
    	//if ((size->flags&mRepMode)!=asNone) normalize(size);
    	if ((size->flags&simpleUnknownUint)==simpleUnknownUint)
        	if(size->next!=size && size->next==size->prev && ((size->next->flags&simpleUnknownUint)==simpleUnknownUint)){
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
    infon *args=v->spec1, *spec=v->spec2, *parent=getTop(v); int EOT=0; UInt mode; UInt vSize=(UInt)v->size;
    char posArea=(v->flags&(fUnknown<<goSize))?'?':isPosLorEorGtoSize(vSize, parent);
    if(posArea=='G'){std::cout << "EXTRA ITEM ALERT!\n"; closeListAtItem(v); return;}
    UInt tmpFlags=v->flags&0xff000000;
    if (spec){
        if((mode=(spec->flags&mRepMode))==asFunc){
            deepCopy(spec,v,args);
			if((args->flags&mRepMode)==toWorldCtxt && args->spec1==(infon*)miscFindAssociate){
				EOT=0;
			} else EOT=getNextTerm(&args);
        } else if(mode<(UInt)asFunc){
			deepCopy(spec,v);
			if(posArea=='?' && v->spec1) posArea= 'N'; // N='Not greater'
        } else deepCopy(spec, v);
    }
    v->flags|=tmpFlags; v->flags&=~isVirtual;
    if(EOT){ if(posArea=='?'){posArea='E'; closeListAtItem(v);}
        else if(posArea!='E') throw "List was too short";}
    if (posArea=='E') {v->flags|=isBottom+isLast; return;}
    infon* tmp= new infon;  tmp->size=(infon*)(vSize+1); tmp->spec2=spec;
    tmp->flags|=fUnknown+isBottom+isVirtual+asNone+(tUInt<<goSize);
    tmp->top=tmp->next=v->next; v->next=tmp; tmp->prev=v; tmp->next->prev=tmp; tmp->spec1=args;
    v->flags&=~(isBottom+isTentative);
    if (posArea=='?'){ v->flags|=isTentative;}
}

void agent::InitList(infon* item) {
     infon* tmp;
    if(item->value && (((tmp=item->value->prev)->flags)&isVirtual)){
        tmp->spec2=item->spec2;
        if(tmp->spec2 && ((tmp->spec2->flags&mRepMode)==asFunc))
                StartTerm(tmp->spec2->spec1, &tmp->spec1);
        processVirtual(tmp); item->flags|=tList;
    }
}

int agent::getFollower(infon** lval, infon* i){
    infon* size; int levels=0;
    gnsTop: if(i->next==0) {*lval=0; return levels;}
    if((i->next->flags&isVirtual) && i->spec2 && i->spec2->prev==(infon*)2){
		i->flags|=(isLast+isBottom);
		size=i->next->size-1;
		i->next->next->prev=i; i->next=i->next->next;// TODO when working on mem mngr: i->next is dangling
		i=getTop(i); i->size=size; i->flags&= ~fLoop; ++levels;
		goto gnsTop;
	}
    if(i->flags&isLast && i->prev){
        i=getTop(i); ++levels;
        if(i) goto gnsTop;
        else {*lval=0; return levels;}
    }
    *lval=i; getNextTerm(lval); // *lval=i->next;
    if((*lval)->flags&isVirtual) processVirtual(*lval);
    return levels;
}

void agent::addIDs(infon* Lvals, infon* Rvals, int asAlt){
    const int maxAlternates=100;
	if(Lvals==Rvals) return;
    infon* RvlLst[maxAlternates]; infon* crntAlt=Rvals; infon *pred, *Rval, *prev=0; infNode* IDp; UInt size;
    int altCnt=1; RvlLst[0]=crntAlt;
    while(crntAlt && (crntAlt->flags&isTentative)){
        getFollower(&crntAlt, getTop(crntAlt));
        RvlLst[altCnt++]=crntAlt;
      	if(altCnt>=maxAlternates) throw "Too many nested alternates";
    }
    size=((UInt)Lvals->next->size)-2; crntAlt=Lvals; pred=Lvals->pred;
    while(crntAlt){  // Lvals
        for(int i=0; i<altCnt; ++i){   // Rvals
            if (!asAlt && altCnt==1 && crntAlt==Lvals){
                insertID(&Lvals->wrkList,Rvals,0); recAlts(Lvals,Rvals);
            } else {
                Rval=RvlLst[i];
                AddSizeAlternate(crntAlt, Rval, pred, size, (prev)?prev->prev:0);
            }
        }
        if(crntAlt->flags&isTentative) {prev=crntAlt; size=((UInt)crntAlt->next->size)-2; crntAlt=getTop(crntAlt); if(crntAlt) crntAlt->flags|=hasAlts;}
        else crntAlt=0;
    }
}

infon* getIdentFromPrev(infon* prev){
// TODO: this may only work in certain cases. It should do a recursive search.
    return prev->wrkList->item->wrkList->item;
}

int agent::compute(infon* i){
    infon* p=i->value; int vAcc, sAcc, count=0;
    if(p) do{
        normalize(p); // TODO: appending inline rather than here would allow streaming.
        if((p->flags&((tType<<goSize)+tType))==((tUInt<<goSize)+tUInt)){
            if (p->flags&(fUnknown<<goSize)) return 0;
            if (p->flags&fConcat) compute(p->size);
            if ((p->flags&(tType<<goSize))!=(tUInt<<goSize)) return 0;
            if (p->flags&fUnknown) return 0;
            if ((p->flags&tType)==tList) compute(p->value);
            if ((p->flags&tType)!=tUInt) return 0;
            int val=(p->flags&fInvert)?-(UInt)p->value:(UInt)p->value;
            if(p->flags&(fInvert<<goSize)){
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
    i->flags=(i->flags&0xFF00FF00)+(tUInt<<goSize)+tUInt;
    return 1;
}

void resolve(infon* i, infon* theOne){
    infon *prev=0;
    while(i && theOne){
        if(theOne->flags&isTentative){
			closeListAtItem(theOne);
			return;
        } else {
			std::cout <<"$$$$"<<printInfon (i)<<i->size<<"\n";
			std::cout <<"####"<<printInfon (theOne)<<theOne->size<<"\n";
            i->size=theOne->size; i->value=theOne->value; i->flags&=~hasAlts;
			if((theOne->flags&tType)==tList) {infon* itm=i->value; for (UInt p=(UInt)i->size; p; --p) {itm->flags&=~isTentative; itm=itm->next;}}
            prev=i; i=i->pred; theOne=theOne->pred;
        }
    } if (theOne){theOne->next=prev->value; theOne->flags|=isLast+isBottom;}
}

#define SetBypassDeadEnd() {result=BypassDeadEnd; infon* CA=getTop(ci); if (CA) AddSizeAlternate(CA, item, 0, ((UInt)ci->next->size)-1, ci); }

enum WorkItemResults {DoNothing, BypassDeadEnd, DoNext};
int agent::doWorkList(infon* ci, infon* CIfol, int asAlt){
    infNode *wrkNode=ci->wrkList, *IDp; infon *item, *IDfol, *tmp, *tmp2, *theOne=0;
    UInt altCount=0, cSize, tempRes, isIndef=0, result=DoNothing, f;
    if(CIfol && !CIfol->pred) CIfol->pred=ci;
    if(wrkNode)do{
        wrkNode=wrkNode->next; item=wrkNode->item; IDfol=(infon*)1; DEB(" Doing Work Order:" << item);
		if (wrkNode->idFlags&skipFollower) CIfol=0;
        switch (wrkNode->idFlags&WorkType){
        case ProcessAlternatives:
            if (altCount>=1) break; // don't keep looking after found
            if(wrkNode->idFlags&isRawFlag){
              tmp=new infon(ci->flags,ci->size,ci->value,0,ci->spec1,ci->spec2,ci->next);
              tmp->prev=ci->prev; tmp->top=ci->top;
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
            if((item->flags&mRepMode)!=asNone && (item->flags&mRepMode)!=fromHere) normalize(item);
            if((ci->flags&tType)==0) {
                ci->flags|=((item->flags&tType)+(tUInt<<goSize)+sizeIndef); isIndef=1;
                ci->size=item->size;if (!(item->flags&(fUnknown<<goSize))) ci->flags&=~(fUnknown<<goSize);
                    } else isIndef=0;
			if (item->flags&fConcat) std::cout << "WARNING: Trying to merge a concatenation.\n";
            if ((ci->flags&matchType) && ((ci->flags&tType) != (item->flags&tType)))
                        {result=BypassDeadEnd; std::cout << "Non-matching types\n";}
            else switch((ci->flags&tType)+4*(item->flags&tType)){
                case tUInt+4*tUInt: DEB("(UU)")
                    if(!(ci->flags&fUnknown) && ci->value!=item->value) {SetBypassDeadEnd(); break;}
                    if(!(ci->flags&(fUnknown<<goSize)) && ci->size!=item->size) {SetBypassDeadEnd(); break;}
                    copyTo(item, ci);
                  case tUnknown+4*tUnknown:
                  case tUInt+4*tUnknown:
                  case tString+4*tUnknown:
                    result=DoNext;
                    getFollower(&IDfol, item);
                    if(CIfol){
						if(IDfol) addIDs(CIfol, IDfol, asAlt);
						else if(ci->next && (ci->next->flags&isTentative))
						{SetBypassDeadEnd(); closeListAtItem(ci);}
					}
                    break;
                case tUInt+4*tString: DEB("(US)") result=DoNext; break;
                case tUInt+4*tList:DEB("(UL)") InitList(item); addIDs(ci,item->value,asAlt); result=DoNext;break;
                case tString+4*tUInt:DEB("(SU)") result=DoNext;break;
                case tString+4*tString: DEB("(SS)")
                    if (!isIndef && ci->flags&sizeIndef){
                        if(ci->size < item->size) {result=BypassDeadEnd; break;}
                        if (memcmp(ci->value, item->value, (UInt)item->size)!=0)  {result=BypassDeadEnd;break;}
                        ci->size=item->size;
                        if(CIfol){
                            IDfol=item->next;
                            if(IDfol) addIDs(CIfol, IDfol, asAlt); else {SetBypassDeadEnd(); break;}
				}
                    result=DoNext;
                    break;
                    } else isIndef=0;
                    if((ci->flags&((fUnknown+tType)<<goSize))==((fUnknown+tUInt)<<goSize) &&  (!(item->flags&(fUnknown<<goSize)))) {
                    	ci->size=item->size;
                        ci->flags&=~(fUnknown<<goSize);
                     }
                    cSize=(UInt)ci->size;
                    if(cSize>(UInt)item->size) {SetBypassDeadEnd(); break;}
                    if(ci->flags&fUnknown){ci->flags&=~fUnknown;}
                    else if (memcmp(ci->value, item->value, cSize)!=0) {SetBypassDeadEnd(); break;}
                    ci->value=item->value;
                    result=DoNext;
                    getFollower(&IDfol, item);
                    if(CIfol){
                        if(cSize==(UInt)item->size){
                            if(IDfol) {addIDs(CIfol, IDfol, asAlt);}
                            else if(ci->next && (ci->next->flags&isTentative))
								SetBypassDeadEnd();
                        }else if(cSize < (UInt)item->size){
                            tmp=new infon(item->flags,(infon*)((UInt)item->size-cSize),(infon*)((UInt)item->value+cSize),0,0,item->next);
                            addIDs(CIfol, tmp, asAlt);
                        }
                    }
                    break;
                case tString+4*tList:DEB("(SL)") InitList(item);  result=DoNext;
                    break;
                case tList+4*tList: InitList(item);
					result=DoNext;
                    if(ci->value) {addIDs(ci->value, item->value, asAlt);}
                    else{
                        copyTo(item, ci); item->value->top=ci;
                        getFollower(&IDfol, item);
                        if(CIfol && IDfol) {addIDs(CIfol, IDfol, asAlt);}
                    }
                    break;
                case tList+4*tUnknown:
                case tList+4*tString:
                case tList+4*tUInt: DEB("(L?)")
                    if(ci->value){addIDs(ci->value, item, asAlt);}
                    else{copyTo(item, ci);}
                    result=DoNext;
            }
            break;
        }
        if(!CIfol && ((tmp2=getVeryTop(ci))!=0) && (tmp2->prev==((infon*)1)))
			tmp2->prev=(IDfol==0)?(infon*)2:IDfol; // Set the next seed for index-lists
    }while (wrkNode!=ci->wrkList); else{ result=((ci->flags&fUnknown) && ci->next && (ci->next->flags&isTentative))?BypassDeadEnd:DoNext; if(result==BypassDeadEnd) 
			std::cout<<"#############>>"<<printInfon(ci)<<"\n";}
    if(altCount==1){
            for (f=1, tmp=getTop(ci); tmp!=0; tmp=getTop(tmp)) // check ancestors for alts
               if (tmp->flags&hasAlts) {f=0; break;}
            if(f) resolve(ci, theOne);
    }
    ci->flags|=isNormed; ci->flags&=~sizeIndef;
    return result;
}

infon* agent::normalize(infon* i, infon* firstID, bool doShortNorm){
    if (i==0) return 0;
    int nxtLvl, override; infon *CI, *CIfol, *tmp; infNode* IDp;
    if((i->flags&tType)==tList) InitList(i);
    infQ ItmQ; ItmQ.push(Qitem(i,firstID,(firstID)?1:0,0));
//    DEB("\n<h2>Normalizing: " << i << "</h2>\n<table border=\"3\" cellpadding=\"5\" width=\"100%\"><tr><td>\n");
    while (!ItmQ.empty()){
        Qitem cn=ItmQ.front(); ItmQ.pop(); CI=cn.item;
        int CIRepMode=CI->flags&mRepMode; override=0;
        while (CIRepMode!=asNone){
            if(CI->flags&toExec) override=1;
            if(CI->flags&asDesc) {if(override) override=0; else break;}
            if(CIRepMode==asFunc){
                if(CI->flags&mMode){//Evaluating Inverse function... TODO: this is broken
                    LastTerm(CI, &tmp);
                    insertID(&tmp->wrkList, CI->spec1,0);
                }else { //Evaluating Function...
                    if(autoEval(CI, this)) break;
					// When the current slip is executed with this it gets into an infinite look generating multiples of 50 forever.
					// We could have it check to see if the first and last items in the list are known. If so, assign arge to the
					// first item then normalize the last. The last one will have references to the first and so will lazily normalize it.
					// But what if the first and last items are NOT known? Then we have to do it this current way which still has the problem. 
					// Sure we could say "if using locals make sure the first and last items are known" but I'm sure that will be problematic later.
					// So we COULD mark certain items "lazy only" but that's more syntax and a hassle for programmers. How can we infer when to be lazy?
					// 
                    normalize(CI->spec2,CI->spec1); // norm(func-body-list (CI->spec2))
                    LastTerm(CI->spec2, &tmp);
                    if(tmp->flags&fUnknown && tmp->wrkList==0 && CI->next && CI->next->flags&isVirtual){
                        // Here is where I think an endless loop bug should be fixed. I BELEIVE the above condition will detect  the situation
                        // Here in the body we need to handle it.
						std::cout << "###>"<<printInfon (CI)<<"\n";
                        }
                    insertID(&CI->wrkList, tmp,0);
                    cpFlags(tmp,CI); CI->flags|=fUnknown+(fUnknown<<goSize);
                }
            } else if(CIRepMode<(UInt)asFunc){ // Processing Index...
               if(CI->flags&mMode){insertID(&CI->wrkList, CI->spec2,0); fetchFirstItem(CIfol,getTop(CI->spec2))}
               else {
	       		assocInfon* assoc=0; tmp=0;
	       		{ //  TODO: This block really needs to be part of a  re-structuring involving CI->flags
	       		if(CIRepMode==toWorldCtxt && CI->spec1==(infon*)miscFindAssociate){
				   assoc=((assocInfon*)(CI->spec2));
				    if(assoc->nextRef) {
					    tmp=assoc->nextRef; getNextTerm (&tmp);
						if(tmp->flags&isLast){ // set that this is the last one.
							for(infon* findLast=CI; findLast; findLast=findLast->top)
								if(findLast->next && findLast->next->flags&(isTentative+isVirtual)){
									closeListAtItem(findLast);
									break;
								}
						}
					    assoc->nextRef=tmp;
				    }else{
						UInt tmpFlags=CI->flags&0xff000000;
					    deepCopy(assoc->VarRef, CI);
						CI->flags|=tmpFlags;
					    CIRepMode=CI->flags&mRepMode;
				    }
		        }
	      		}
		if(tmp==0){
                    switch (CIRepMode){
                    case toGiven:
                        if (CI->spec1>(infon*)1) {normalize(CI->spec1,0,1); StartTerm(CI->spec1, &tmp);}
                        else tmp=getIdentFromPrev(CI->prev);
                        break;
                    case toWorldCtxt:
		    	        if(CI->spec1==(infon*)miscWorldCtxt) {tmp=&context; break;}
                    case toHomePos: //  Handle \, \\, \\\, etc.
                    case fromHere:  // Handle ^, \^, \\^, \\\^, etc.
                        tmp=CI;
                        for(UInt i=0; i<(UInt)CI->spec1; ++i) {  // for each backslash
                                if(tmp==0 /* || ((tmp->flags&mRepMode)==asFunc)*/) {tmp=0;std::cout << "Too many '\\'s in "<<printInfon(CI)<< '\n';}
                            if(CIRepMode!=toHomePos || i>0) tmp=getTop(tmp);
                        }
                        if(CIRepMode==toHomePos) {
                            if(!(tmp->flags&isTop)) {tmp=tmp->top; }
                            if (tmp==0) std::cout<<"Zero TOP in "<< printInfon(CI)<<'\n';
                            if(!(tmp->flags&isFirst)) {tmp=0; std::cout<<"Top but not First in "<< printInfon(CI)<<'\n';}
                        }
                    }
                    if(tmp) { // Now add that to the 'item-list's wrkList...
                        if ((CI->spec2->flags)&(fInvert<<goSize)){ // TODO: This block is a hack to make simple backward references work. Fix for full back-parsing.
                            for(UInt i=(UInt)CI->spec2->size; i>0; --i){tmp=tmp->prev;}
                            {insertID(&CI->wrkList, tmp,0); if(CI->flags&fUnknown) {CI->size=tmp->size;} cpFlags(tmp, CI);}
                            CI->flags|=fUnknown;
                        } else {
                          insertID(&CI->spec2->wrkList, tmp,0);
                          CI->spec2->prev=(infon*)1;  // Set sentinal value so this can tell it is an index
                          normalize(CI->spec2);
                          tmp->flags|=toExec;
                          LastTerm(CI->spec2, &tmp);
  			  if (assoc){assoc->nextRef=tmp->wrkList->item;}
			 }
			 }
			 }
			 if (tmp->flags&isLast||assoc) {
				 insertID(&CI->wrkList, tmp,(assoc)?skipFollower:0); 
				 if(CI->flags&fUnknown) {CI->size=tmp->size;} 
				 cpFlags(tmp, CI);
			 } else {  // migrate alternates from spec2 to CI...
                            infNode *wrkNode=CI->spec2->wrkList; infon* item=0;
                            if(wrkNode)do{
                                wrkNode=wrkNode->next; item=wrkNode->item;
                                if((wrkNode->idFlags&(ProcessAlternatives+NoMatch))==ProcessAlternatives){
                                    if(wrkNode->item->size==0){} // TODO: handle the null alternative
                                    else {insertID(&CI->wrkList,wrkNode->slot,ProcessAlternatives+isRawFlag);}
                                }
                            }while (wrkNode!=CI->spec2->wrkList);
                            CI->flags|=asNone;
                        }
                        CI->flags|=fUnknown;

                }
            } else if(CIRepMode==asTag){
                if(CI->wrkList){std::cout<<"Defining:'"<<(char*)((stng*)(CI->spec1))->S<<"'\n";
                    std::map<stng,infon*>::iterator tagPtr=tag2Ptr.find(*(stng*)(CI->spec1));
                    if (tagPtr==tag2Ptr.end()) {tag2Ptr[*(stng*)(CI->spec1)]=CI->wrkList->item; CI->wrkList=0; break;}
                    else{throw("A tag is being redefined, which is illegal");}
                } else { DEB("Recalling: "<<(char*)((stng*)(CI->spec1))->S)
                    std::map<stng,infon*>::iterator tagPtr=tag2Ptr.find(*(stng*)(CI->spec1));
                    if (tagPtr!=tag2Ptr.end()) {UInt tmpFlags=CI->flags&0xff000000; deepCopy(tagPtr->second,CI); CI->flags|=tmpFlags;}
                    else{std::cout<<"Bad tag:'"<<(char*)((stng*)(CI->spec1))->S<<"'\n";throw("A tag was used but never defined");}
                }
            }
            CIRepMode=CI->flags&mRepMode;
        }
        if (doShortNorm) return 0;
        if(CIRepMode==asNone&&cn.IDStatus==2)
            {cn.IDStatus=0; insertID(&CI->wrkList,cn.firstID,0);}
           //{cn.IDStatus=0; infon*p,*r; for(p=CI,r=cn.firstID; p&&r; p=p->next, r=r->next) insertID(&p->wrkList,r,0);}
        if(CI!=i && (CI->flags&tType)==tList){InitList(CI);}
        nxtLvl=getFollower(&CIfol, CI);
        if((CI->flags&asDesc) && !override) {pushCIsFollower; continue;}
        switch (doWorkList(CI, CIfol)) {
        case DoNext:
            if((CI->flags&(fConcat+tType))==(fConcat+tUInt))
                {compute(CI); if(CIfol && !(CI->flags&isLast)){pushCIsFollower;}} // push CI's follower
            else if(!((CI->flags&asDesc)&&!override)&&((CI->value&&((CI->flags&tType)==tList))||(CI->flags&fConcat))){
                // push CI's value
                ItmQ.push(Qitem(CI->value,cn.firstID,((cn.IDStatus==1)&!(CI->flags&fConcat))?2:cn.IDStatus,cn.level+1));
            }else if (CIfol){pushCIsFollower;} // push CI's follower
            break;
        case BypassDeadEnd: {nxtLvl=getFollower(&CIfol,getTop(CI))+1; pushCIsFollower;} break;
        case DoNothing: break;
        }
    }
    DEB("<br>RESULT:<b>"<<printInfon(i)<<"</b><br>"); DEB("</td></tr></table>\n");
    return (i->flags&isNormed)?i:0;
}
