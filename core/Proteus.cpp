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
#include "remiss.h"

infon* World;
std::map<stng,infon*> tag2Ptr;
std::map<infon*,stng> ptr2Tag;

std::map<dblPtr,uint> alts;
typedef std::map<dblPtr,uint>::iterator altIter;

void deepCopy(infon* from, infon* to, infon* args=0);

infNode* copyIdentList(infNode* from){
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
infon* copyList(infon* from){
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
    follower->next=top; //follower->flags|=from->prev->flags&mListPos;
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

void deepCopy(infon* from, infon* to, infon* args){
	uint fm=from->flags&mRepMode;
    to->flags=from->flags;
    if(((from->flags>>goSize)&tType)==tList){to->size=copyList(from->size); if(to->size)to->size->top=to;}
    else to->size=from->size;
    if((from->flags&tType)==tList){to->value=copyList(from->value); if(to->value)to->value->top=to;}
    else to->value=from->value;
    to->wrkList=copyIdentList(from->wrkList);
    if(fm==toHomePos) to->spec1=from->spec1;
    else if(fm==asTag) to->spec1=from->spec1;
    else if(from->spec1==0) to->spec1=0;
	else if(fm<asFunc){
		to->spec1=new infon; copyTo(from->spec1,to->spec1);
		if(to->prev && to->prev!=to){
			infon* spec2=to->prev->spec2;
			if (spec2 && spec2->next==0 && (uint)(spec2->prev)>1){
				to->spec1->value=spec2->prev;
				to->spec1->size=(infon*)((uint)to->prev->spec1->size - (uint)spec2->size);
			}
		}
	}
	else {to->spec1=new infon; deepCopy((args)?args:from->spec1,to->spec1);}
    if(from->spec2){to->spec2=new infon; deepCopy(from->spec2, to->spec2);}else to->spec2=0;
}

const int simpleUnknownUint=((fUnknown+tUInt)<<goSize) + fUnknown+tUInt+asNone;
char isPosLorEorGtoSize(uint pos, infon* item){
    if(item->flags&(fUnknown<<goSize)) return '?';
    if(((item->flags>>goSize)&tType)!=tUInt) throw "Size as non integer is probably not useful, so not allowed.";
    if(item->flags&(fConcat<<goSize)){
   	infon* size=item->size;
    	//if ((size->flags&mRepMode)!=asNone) normalize(size);
    	if ((size->flags&simpleUnknownUint)==simpleUnknownUint)
        	if(size->next!=size && size->next==size->prev && ((size->next->flags&simpleUnknownUint)==simpleUnknownUint)){
            		if(pos<(uint)size->value) return 'L';
                        if(pos>(uint)size->size) return 'G';
            		return '?';  // later, if pos is in between two ranges, return 'X'
                }
    }
    if(pos<(uint)item->size) return 'L';  // Less
    if(pos==(uint)item->size) return 'E';  // Equal
    else return 'G';  // Greater
}

void processVirtual(infon* v){
    infon *args=v->spec1, *spec=v->spec2, *parent=getTop(v); int EOT_PV=0, mode; uint vSize=(uint)v->size;
    char posArea=isPosLorEorGtoSize(vSize, parent);
    if(posArea=='G'){return;} // TODO: go backward, renaming/affirming tentatives. Mark last
    uint tmpFlags=v->flags&0xff000000;
    if (spec){
        if((mode=(spec->flags&mRepMode))==asFunc){
            deepCopy(spec,v,args);
            getNextTerm(args,PV)
        } else if(mode<asFunc){
			deepCopy(spec,v);
			if(posArea=='?' && v->spec1) posArea= 'N'; // N='Not greater' Is this logic correct?
        } else deepCopy(spec, v);
    }
    v->flags|=tmpFlags; v->flags&=~isVirtual;
    if(EOT_PV) if(posArea=='?'){posArea='E'; /* TODO:Set Parent's Size to v->size */ }
        else if(posArea!='E') throw "List was too short";
    if (posArea=='E') {v->flags|=isBottom+isLast; return;}
    infon* tmp= new infon;  tmp->size=(infon*)(vSize+1); tmp->spec2=spec;
    tmp->flags|=fUnknown+isBottom+isVirtual+asNone+(tUInt<<goSize);
    tmp->top=tmp->next=v->next; v->next=tmp; tmp->prev=v; tmp->next->prev=tmp; tmp->spec1=args;
    v->flags&=~isBottom;
    if (posArea=='?'){ v->flags|=isTentative;}
}

void InitList(infon* item) {
    int EOT_IL=0; infon* tmp; infNode* IDp;
    if(item->value && (((tmp=item->value->prev)->flags)&isVirtual)){
        tmp->spec2=item->spec2;
        if(tmp->spec2 && ((tmp->spec2->flags&mRepMode)==asFunc)) StartTerm(tmp->spec2->spec1,tmp->spec1,IL);
        processVirtual(tmp); item->flags|=tList;
//        if((tmp->flags&isTentative) && tmp==item->value) {AddSizeAlternate(item,0, 0, 0);}
    }
}

int getFollower(infon** lval, infon* i){
    infon* size; int levels=0;
    gnsTop: if(i->next==0) {*lval=0; return levels;}
    if((i->next->flags&isVirtual) && i->spec2 && i->spec2->prev==(infon*)2){
		i->flags|=(isLast+isBottom);
		size=i->next->size-1;
		i->next->next->prev=i; i->next=i->next->next;// TODO when working on mem mngr: i->next is dangling
		i=getTop(i); i->size=size; i->flags&= ~fLoop; ++levels;
		goto gnsTop;
	}
    if(i->flags&isLast){
        i=getTop(i); ++levels;
        if(i) goto gnsTop;
        else {*lval=0; return levels;}
    }
    *lval=i->next;
    if((*lval)->flags&isVirtual) processVirtual(*lval);
    return levels;
}

void addIDs(infon* Lvals, infon* Rvals, int asAlt=0){
    const int maxAlternates=100;
    infon* RvlLst[maxAlternates]; infon* crntAlt=Rvals; infon *pred, *Rval, *prev=0; infNode* IDp; uint size;
    int altCnt=1; RvlLst[0]=crntAlt;
    while(crntAlt && (crntAlt->flags&isTentative)){
        getFollower(&crntAlt, getTop(crntAlt));
        RvlLst[altCnt++]=crntAlt;
        if(altCnt>=maxAlternates) throw "Too many nested alternates";
    }
    size=((uint)Lvals->next->size)-2; crntAlt=Lvals; pred=Lvals->pred;
    while(crntAlt){  // Lvals
        for(int i=0; i<altCnt; ++i){   // Rvals
            if (!asAlt && altCnt==1 && crntAlt==Lvals){
                insertID(&Lvals->wrkList,Rvals,0); recAlts(Lvals,Rvals);
            } else {
                Rval=RvlLst[i];
                AddSizeAlternate(crntAlt, Rval, pred, size, (prev)?prev->prev:0);
            }
        }
        if(crntAlt->flags&isTentative) {prev=crntAlt; size=((uint)crntAlt->next->size)-2; crntAlt=getTop(crntAlt); if(crntAlt) crntAlt->flags|=hasAlts;}
        else crntAlt=0;
    }
}

infon* getIdentFromPrev(infon* prev){
// TODO: this may only work in certain cases. It should do a recursive search.
    return prev->wrkList->item->wrkList->item;
}

int compute(infon* i){
    infon* p=i->value; uint vAcc, sAcc, sSign, vSign, count=0;
    if(p) do{
        if((p->flags&((tType<<goSize)+tType))==((tUInt<<goSize)+tUInt)){
            if (p->flags&(fUnknown<<goSize)) return 0;
            if (p->flags&fConcat) compute(p->size);
            if ((p->flags&(tType<<goSize))!=(tUInt<<goSize)) return 0;
            if (p->flags&fUnknown) return 0;
            if ((p->flags&tType)==tList) compute(p->value);
            if ((p->flags&tType)!=tUInt) return 0;
            if (++count==1){sAcc=(uint)p->size; vAcc=(uint)p->value;}
            else {sAcc+=(uint)p->size; vAcc*=(uint)p->size; vAcc+=(uint)p->value;}
            } else return 0;
        p=p->next;
    } while (p!=i->value);
    i->value=(infon*)vAcc; i->size=(infon*)sAcc;
    i->flags=(i->flags&0xFF00FF00)+(tUInt<<goSize)+tUInt;
    return 1;
}

void resolve(infon* i, infon* theOne){
    infon* parent, *tmp, *prev=0; uint count;
    while(i && theOne){
        if(theOne->flags&isTentative){
            parent=getTop(theOne);
            // TODO: delete any residue following item, i.e., the old item->next;
            theOne->next=(infon*)parent->value; theOne->next->prev=theOne; theOne->flags|=(isBottom+isLast);
            for (count=1, tmp=theOne; !(tmp->flags&isTop); tmp=tmp->prev) {tmp->flags^=isTentative; ++count;}
            parent->size=(infon*)count; parent->flags^=(fUnknown<<goSize);
            return; // TODO: probably this should continue going left
        } else {
            i->size=theOne->size; i->value=theOne->value; i->flags^=hasAlts;
            prev=i; i=i->pred; theOne=theOne->pred;
        }
    } if (theOne){theOne->next=prev->value; theOne->flags|=isLast+isBottom;}
}

#define SetBypassDeadEnd() {result=BypassDeadEnd; infon* CA=getTop(ci); if (CA) AddSizeAlternate(CA, item, 0, ((uint)ci->next->size)-1, ci); }

enum WorkItemResults {DoNothing, BypassDeadEnd, DoNext};
int agent::doWorkList(infon* ci, infon* CIfol, int asAlt){
    infNode *wrkNode=ci->wrkList, *iter, *IDp; infon *item, *IDfol, *tmp, *tmp2, *parent, *theOne=0;
    uint altCount=0, cSize, tempRes, isIndef=0, result=DoNothing, f;
    if(CIfol && !CIfol->pred) CIfol->pred=ci;
    if(wrkNode)do{
        wrkNode=wrkNode->next; item=wrkNode->item; IDfol=(infon*)1; DEB(" Doing Work Order:" << item);
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
            if((item->flags&mRepMode)!=asNone) normalize(item);
            if((ci->flags&tType)==0) {
                ci->flags|=((item->flags&tType)+(tUInt<<goSize)+sizeIndef); isIndef=1;
                ci->size=item->size;if (!(item->flags&(fUnknown<<goSize))) ci->flags&=~(fUnknown<<goSize);
            } else isIndef=0;
            switch((ci->flags&tType)+4*(item->flags&tType)){
                case tUInt+4*tUInt: DEB("(UU)")
                    if(!(ci->flags&fUnknown) && ci->value!=item->value) {SetBypassDeadEnd(); break;}
                    if(!(ci->flags&(fUnknown<<goSize)) && ci->size!=item->size) {SetBypassDeadEnd(); break;}
                    copyTo(item, ci);
                    result=DoNext;
                    getFollower(&IDfol, item);
                    if(CIfol)
						if(IDfol) addIDs(CIfol, IDfol, asAlt);
						else if(ci->next && (ci->next->flags&isTentative))
							SetBypassDeadEnd();
                    break;
                case tUInt+4*tString: DEB("(US)") result=DoNext; break;
                case tUInt+4*tList:DEB("(UL)") InitList(item); addIDs(ci,item->value,asAlt); result=DoNext;break;
                case tString+4*tUInt:DEB("(SU)") result=DoNext;break;
                case tString+4*tString: DEB("(SS)")
                    if (!isIndef && ci->flags&sizeIndef){
                        if(ci->size < item->size) {result=BypassDeadEnd; break;}
                        if (memcmp(ci->value, item->value, (uint)item->size)!=0)  {result=BypassDeadEnd;break;}
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
                    cSize=(uint)ci->size;
                    if(cSize>(uint)item->size) {SetBypassDeadEnd(); break;}
                    if(ci->flags&fUnknown){ci->flags&=~fUnknown;}
                    else if (memcmp(ci->value, item->value, cSize)!=0) {SetBypassDeadEnd(); break;}
                    ci->value=item->value;
                    result=DoNext;
                    getFollower(&IDfol, item);
                    if(CIfol){
                        if(cSize==(uint)item->size){
                            if(IDfol) {addIDs(CIfol, IDfol, asAlt);}
                            else if(ci->next && (ci->next->flags&isTentative))
								SetBypassDeadEnd();
                        }else if(cSize < (uint)item->size){
                            tmp=new infon(item->flags,(infon*)((uint)item->size-cSize),(infon*)((uint)item->value+cSize),0,0,item->next);
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
    }while (wrkNode!=ci->wrkList); else result=DoNext;
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
    int cnt=0; int nxtLvl, override, EOT_n; infon *CI, *CIfol, *tmp; infNode* IDp;
    if((i->flags&tType)==tList) InitList(i);
    infQ ItmQ; ItmQ.push(Qitem(i,firstID,(firstID)?1:0,0));
    DEB("\n<h2>Normalizing: " << i << "</h2>\n<table border=\"3\" cellpadding=\"5\" width=\"100%\"><tr><td>\n");
    while (!ItmQ.empty()){
        Qitem cn=ItmQ.front(); ItmQ.pop(); CI=cn.item;
        int CIRepMode=CI->flags&mRepMode; override=0;
        DEB("<br>CI:"<<cnt++<<" "<<CI<<":<b>"<<printInfon(i,CI)<<"</b><br>\n")
        while (CIRepMode!=asNone){
            if(CI->flags&toExec) override=1;
            if(CI->flags&asDesc) if(override) override=0; else break;
            if(CIRepMode==asFunc){
                if(CI->flags&mMode){//Evaluating Inverse function...
                    LastTerm(CI, tmp, n);
                    insertID(&tmp->wrkList, CI->spec1,0);
                }else { //Evaluating Function...
                    normalize(CI->spec2,CI->spec1); // norm(func-body-list (CI->spec2))
                    LastTerm(CI->spec2, tmp, n);
                    insertID(&CI->wrkList, tmp,0);
                    cpFlags(tmp,CI); CI->flags|=fUnknown+(fUnknown<<goSize);
                }
            } else if(CIRepMode<asFunc){ // Processing Index...
               if(CI->flags&mMode){insertID(&CI->wrkList, CI->spec2,0); fetchFirstItem(CIfol,getTop(CI->spec2))}
               else {
                    switch (CIRepMode){
                    case toGiven:
                        if (CI->spec1>(infon*)1) {normalize(CI->spec1,0,1); StartTerm(CI->spec1, tmp, n);}
                        else tmp=getIdentFromPrev(CI->prev);
                        break;
                    case toWorldCtxt:tmp=&context; break;
                    case toHomePos:
                        tmp=CI;
                        for(uint i=(uint)CI->spec1; i--;) {
                            if(tmp==0 || ((tmp->flags&mRepMode)==asFunc)) {tmp=0;break;}
                            tmp=getTop(tmp); DEB("#goUpTo:"<<tmp)
                        }
                        if(tmp) {
                            if(!(tmp->flags&isTop)) {tmp=tmp->top; DEB(" then home to:"<<tmp<<" ")}
                            if (tmp==0) std::cout<<"Zero TOP in "<< printInfon(CI)<<'\n';
                            if(!(tmp->flags&isFirst)) tmp=0;
                        } else std::cout << "Too many '\\'s in "<<printInfon(CI)<< '\n';
                        break;
                    case fromHere:tmp=CI; break;
                    }
                    if(tmp) { // Now add that to the 'item-list's wrkList...
                        insertID(&CI->spec2->wrkList, tmp,0);
                        CI->spec2->prev=(infon*)1;  // Set sentinal value so this can tell it is an index
                        normalize(CI->spec2);
                        LastTerm(CI->spec2, tmp, n); // Add that last term to CI's wrkList.
                        if (tmp->flags&isLast) {insertID(&CI->wrkList, tmp,0); if(CI->flags&fUnknown) {CI->size=tmp->size;} cpFlags(tmp, CI);}
                        else {  // migrate alternates from spec2 to CI...
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
                }
            } else if(CIRepMode==asTag){
                if(CI->wrkList){DEB("Defining: "<<(char*)((stng*)(CI->spec1))->S)
                    std::map<stng,infon*>::iterator tagPtr=tag2Ptr.find(*(stng*)(CI->spec1));
                    if (tagPtr==tag2Ptr.end()) {tag2Ptr[*(stng*)(CI->spec1)]=CI->wrkList->item; CI->wrkList=0; break;}
                    else{throw("A tag is being redefined, which is illegal");}
                } else { DEB("Recalling: "<<(char*)((stng*)(CI->spec1))->S)
                    std::map<stng,infon*>::iterator tagPtr=tag2Ptr.find(*(stng*)(CI->spec1));
                    if (tagPtr!=tag2Ptr.end()) {uint tmpFlags=CI->flags&0xff000000; deepCopy(tagPtr->second,CI); CI->flags|=tmpFlags;}
                    else{throw("A tag was used but never defined");}
                }
            }
            CIRepMode=CI->flags&mRepMode;
        }
        if (doShortNorm) return 0;
        if(CIRepMode==asNone&&cn.IDStatus==2){cn.IDStatus=0; insertID(&CI->wrkList,cn.firstID,0);}
        if(CI!=i && (CI->flags&tType)==tList){InitList(CI);}
        nxtLvl=getFollower(&CIfol, CI);
        if((CI->flags&asDesc) && !override) {pushCIsFollower; continue;}
        switch (doWorkList(CI, CIfol)) {
        case DoNext:
            if((CI->flags&(fConcat+tType))==(fConcat+tUInt))
                {normalize(CI->value); compute(CI); if(CIfol){pushCIsFollower;}} // push CI's follower
            else if(!((CI->flags&asDesc)&&!override)&&(CI->value&&((CI->flags&tType)==tList)||(CI->flags&fConcat))){
                // push CI's value
                ItmQ.push(Qitem(CI->value,cn.firstID,(cn.IDStatus==1&!(CI->flags&fConcat))?2:cn.IDStatus,cn.level+1));
            }else if (CIfol){pushCIsFollower;} // push CI's follower
            break;
        case BypassDeadEnd: {nxtLvl=getFollower(&CIfol,getTop(CI))+1; pushCIsFollower;} break;
        case DoNothing: break;
        }
    }
    DEB("<br>RESULT:<b>"<<printInfon(i)<<"</b><br>"); DEB("</td></tr></table>\n");
    return (i->flags&isNormed)?i:0;
}
