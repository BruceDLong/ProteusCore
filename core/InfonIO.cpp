/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2011 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "Proteus.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <math.h>
#include <cstdio>
#include "remiss.h"

#define Indent {for (int x=0;x<indent;++x) s+=" ";}
std::string printPure (infon* i, UInt f, UInt wSize, infon* CI){
    std::string s;
    if ((f&(fIncomplete+fUnknown)) && !(f&tNum)) s+="?";
    UInt type=f&tType;
    if (type==tNum){
        if (f & fUnknown) s+='_';
        else if(f&tReal){char buf[70]; sprintf(buf,"%f", fix16_to_dbl((fix16_t)i)); s.append(buf);}
        else {char buf[70]; itoa((UInt)i, buf); s.append(buf);}
    } else if(type==tString){s+="\"";s.append((char*)i,wSize);s+="\""; }
    else if(type==tList||(f&fConcat)){
        s+=(f&fConcat)?"(":"{";
        for(infon* p=i;p;) {
            if(p==i && f&fLoop && i->spec2){printInfon(i->spec2,CI); s+=" | ";}
            if(p->pFlag&isTentative) {s+="..."; break;}
            else {s+=printInfon(p, CI); s+=' ';}
            if (p->pFlag&isBottom) p=0; else p=p->next;
        }
        s+=(f&fConcat)?")":"}";
    }
    return s;
}

std::string printInfon(infon* i, infon* CI){
    std::string s; //Indent;
    if(i==0) {s+="null"; return s;}
    if(i==CI) s+="<font color=green>";
    UInt f=i->pFlag;
    UInt mode=i->wFlag&mFindMode;
//    if(f&toExec) s+="@";
    if(f&asDesc) s+="#";
    if (mode==iTagUse) {
       char* ch=i->type->S;
        for(int x=i->type->L;x;--x){
            s+=(*ch=='\0')?' ':ch++[0];
        }s+=' ';
    } else if(f&isNormed || mode==iNone /* || f&notParent */){
        if (f&(fUnknown<<goSize)) {s+=printPure(i->value, f, i->wSize, CI); if((f&tType)!=tList) s+=",";}
        else if (f&fUnknown) {s+=printPure(i->size, f>>goSize,i->wSize, CI); s+=";";}
        else{
            if((f&tType)==tNum) {s+=((f>>goSize)&fInvert)?"/":"*"; s+=printPure(i->size, f>>goSize, 0,CI);}
             if((f&tType)==tNum) s+=(f&fInvert)?"-":"+";
            s+=printPure(i->value,f, (UInt)i->size, CI);
        }
    } else {
        if (!(f&isNormed)) {
            if(mode==iToWorld) s+="%W ";
            else if(mode==iToCtxt) s+="%C";
            else if(mode==iToArgs) s+="%A ";
            else if(mode==iToVars) s+="%V ";
            else if(mode==iToPath || mode==iToPathH){
                s+="%";
                for(UInt h=(UInt)(i->spec1); h; h--) s+="\\";
                if(mode==iToPath) s+="^";
            }
        }
    }

    if (!(f&isNormed) && i->wrkList) {
        infNode* k=i->wrkList;
        do {k=k->next; s+="="; s+="<ID> "; /*printInfon(k->item,CI);*/} while(k!=i->wrkList);
    }
    if (i==CI) s+="</font>";
    return s;
}

#define streamPut(nChars) {for(int n=nChars; n>0; --n){stream.putback(textParsed[textParsed.size()-1]); textParsed.resize(textParsed.size()-1);}}
#define ChkNEOF {if(stream.eof() || stream.fail()) throw "Unexpected End of file";}
#define getbuf(c) {ChkNEOF; for(p=0;(c);buf[p++]=streamGet()){if (p>=bufmax) throw "String Overflow";} buf[p]=0;}
#define check(ch) {RmvWSC(); ChkNEOF; tok=streamGet(); if(tok != ch) {std::cout<<"Expected "<<ch<<"\n"; throw "Unexpected character";}}

char QParser::streamGet(){char ch=stream.get(); textParsed+=ch; return ch;}
char QParser::peek(){if (stream.fail()) throw "Unexpected end of file";  return stream.peek();}
char QParser::Peek(){RmvWSC(); return peek();}

void QParser::scanPast(char* str){
    char p; char* ch=str;
    while(*ch!='\0'){
        p=streamGet();
        if (stream.eof() || stream.fail()) throw "Expected String not found before end-of-file";
        if (*ch==p) ch++; else ch=str;
        if (p=='\n') ++line;
    }
}

void QParser::RmvWSC (){ // Remove whitespace and comments.
    char p,p2;
    for (p=stream.peek(); (p==' '||p=='/'||p=='\n'||p=='\r'||p=='\t'); p=stream.peek()){
        if (p=='/') {
            streamGet(); p2=stream.peek();
            if (p2=='/') {
                for (p=stream.peek(); !(stream.eof() || stream.fail()) && p!='\n'; p=stream.peek()) streamGet();
            } else if (p2=='*') {
                for (p=streamGet(); !(stream.eof() || stream.fail()) && !(p=='*' && stream.peek()=='/'); p=streamGet())
                    if (p=='\n') ++line;
                if (stream.eof() || stream.fail()) throw "'/*' Block comment never terminated";
            } else {streamPut(1); return;}
        }
        if (streamGet()=='\n') ++line;
    }
}



const char* altTok(std::string tok){
    return 0;
    // later this should load from a dictionary file for different languages.
    if (tok=="=") return "is";
    if (tok==": =") return "'s";
    if (tok=="= :") return "of";
    if (tok=="%C") return "the";
    if (tok=="%W") return "thee";
   // if (tok=="%A") return "%Arg";
    if (tok=="%V") return "%Var";

    return 0;
}

bool QParser::chkStr(const char* tok){
    int startPos=textParsed.size();
    if (tok==0) return 0;
    for(const char* p=tok; *p; p++) {
        if ((*p)==' ') RmvWSC();
        else if ((*p) != streamGet ()){
            streamPut(textParsed.size()-startPos);
            return false;
        }
    }
    return true;
}

#include <cstdarg>
const char* QParser::nxtTokN(int n, ...){
    char* tok; va_list ap; va_start(ap,n); int i,p;
    for(i=n; i; --i){
        tok=va_arg(ap, char*); nTok=Peek();
        if(strcmp(tok,"ABC")==0) {if(iscsym(nTok)&&!isdigit(nTok)&&(nTok!='_')) {getbuf(iscsym(peek())); break;} else tok=0;}
        else if(strcmp(tok,"123")==0) {if(isdigit(nTok)) {getbuf((isdigit(peek())||peek()=='.')); break;} else tok=0;}
        else if (chkStr(tok) || chkStr(altTok(tok))) break; else tok=0;
    }
    va_end(ap);
    return tok;
}
#define nxtTok(tok) nxtTokN(1,tok)

bool IsHardFunc(char* tag);
void  chk4HardFunc(infon* i){
    if((i->wFlag&mFindMode)==iTagUse && IsHardFunc(i->type->S)){i->wFlag&=~iTagUse; i->wFlag|=iHardFunc;}
}

infon* grok(infon* item, UInt tagCode, int* code){
    if((item->wFlag&mFindMode)==iTagUse) {(*code)|=tagCode; return item;}
    //else if((item->wFlag&mFindMode)==iGetLast) {return item->spec1;}
    else {
        if (tagCode==c1Left || tagCode==c1Right) {
            if((item->wFlag&mFindMode)>=iGetLast) {return item->spec1;}
        } else if (tagCode==c2Left || tagCode==c2Right) {
            if((item->wFlag&mFindMode)>=iGetLast) {
                // if (item->spec1->spec2 a tag)  mark inner-tag; return it.
                if ((item->spec1->spec2->wFlag&mFindMode)>=iGetLast) return item->spec1->spec2->spec1;
            } else{ // { [A V R] | ...} ::= ABC
                // if (item->spec2 a tag)  mark inner-tag; return it.
                if ((item->spec2->wFlag&mFindMode)>=iGetLast) return item; //item->spec2->spec1;
            }
        }
    }
    return 0;
}

char errMsg[100];
UInt QParser::ReadPureInfon(infon** i, UInt* pFlag, UInt *wFlag, infon** s2){
    UInt p=0, size=0, stay=1; char rchr, tok; infon *head=0, *prev; infon* j;
    if(nxtTok("(") || nxtTok("{") || nxtTok("[")){
        if(nTok=='(') {rchr=')'; *pFlag|=(fConcat+tNum);}
        else if(nTok=='['){rchr=']'; *pFlag|=tList; *wFlag=iGetLast;}
        else {rchr='}'; *pFlag|=tList;}
        RmvWSC(); int foundRet=0; int foundBar=0;
        for(tok=peek(); tok != rchr && stay; tok=peek()){
            if(tok=='<') {foundRet=1; streamGet(); j=ReadInfon();}
            else if(nxtTok("...")){
                j=new infon(fUnknown+isVirtual+(tNum<<goSize),iNone,(infon*)(size+1));stay=0;
            } else j=ReadInfon();
            if(++size==1){
                if(!foundRet && !foundBar && stay && nxtTok("|")){
                    *s2=j; *pFlag|=fLoop; size=0; foundBar=1; RmvWSC(); continue;
                }
                *i=head=prev=j; head->pFlag|=isFirst+isTop;if(*pFlag&fConcat)*pFlag|=(j->pFlag&tType);
            }
            if (foundRet==1) {check('>'); *i=j; foundRet++; *pFlag|=intersect;}
            j->top=head; j->next=head; prev->next=j; j->prev=prev; head->prev=j;
            prev=j; RmvWSC();
        }
        if(head){
            head->prev->pFlag|=isBottom;
            if (!(head->pFlag&fIncomplete) && stay) head->prev->pFlag|=isLast;
        }
        check(rchr);
        if(nxtTok("~"))  (*wFlag)|=mAssoc;
    } else if (nxtTok("123")) {  // read number
        *pFlag|=tNum; size=1;
        if(strchr(buf,'.')) {(*i)=(infon*)fix16_from_dbl(atof(buf)); *pFlag|=tReal;} else *i=(infon*)atoi(buf);
    } else if (nxtTok("_")){*pFlag|=fUnknown+tNum;
    } else if (nxtTok("$")){*pFlag|=fUnknown+tString;
    } else if (nxtTok("\"") || nxtTok("'")){  // read string
        getbuf(peek()!=nTok); streamGet();
        *pFlag+=tString; *i=(((*pFlag)&fUnknown)? 0 : (infon*)new char[p]); memcpy(*i,buf,p);
        size=p;
    } else {strcpy(errMsg, "'X' was not expected"); errMsg[1]=nTok; throw errMsg;}
    return size;
}

infon* QParser::ReadInfon(int noIDs){
    char op=0; UInt size=0; infon*iSize=0,*iVal=0,*s1=0,*s2=0; UInt pFlag=0,wFlag=0,fs=0,fv=0;
    stng* tags=0; const char* cTok, *eTok, *cTok2; /*int textEnd=0; int textStart=textParsed.size();*/
    if(nxtTok("@")){pFlag|=toExec;}
    if(nxtTok("#")){pFlag|=asDesc;}
    if(nxtTok("?")){fs=fv=fUnknown; wFlag=iNone;}
    else if(nxtTok("ABC")){
        wFlag|=iTagUse; tags=new stng;
        // if(iscsym(tok)&&!isdigit(tok)){  // change 'if' to 'do-while' when tag-chains are ready.
            stngApnd((*tags),buf,strlen(buf)+1);
    }else if( nxtTok("%")){ // TODO: Don't allow these outside of := or :
        pFlag|=fUnknown;
        // TODO: %Wind, %Cars, %ART, %Veins are mis-read here.
        if (nxtTok("W")){std::cout<<"WORLD"<<"\n"; wFlag|=iToWorld; }
        else if (nxtTok("C")){wFlag|=iToCtxt;}
        else if (nxtTok("A")){wFlag|=iToArgs;}
        else if (nxtTok("V")){wFlag|=iToVars;}
        else if(nxtTok("ABC")){wFlag|=iTagDef; tags=new stng; stngApnd((*tags),buf,strlen(buf)+1); caseDown(tags);}
        else if (nTok=='\\' || nTok=='^'){
            for(s1=0; (nTok=streamGet())=='\\';) {s1=(infon*)((UInt)s1+1); ChkNEOF;}
            if (nTok=='^') wFlag|=iToPath; else {wFlag|=iToPathH; streamPut(1);}
        }
    }else{  // OK, then we're parsing some form of *... +...
        wFlag|=iNone;
        if(nxtTok("+") || nxtTok("-")) op='+'; else if(nxtTok("*") || nxtTok("/")) op='*';
        if( nTok=='-' || nTok=='/') fs|=fInvert;
        size=ReadPureInfon(&iSize, &fs, &wFlag, &s2);
        if(op=='+'){
            iVal=iSize; iSize=(infon*)size; fv=fs; fs=tNum; // use identity term '1'
            if (size==0 && (fv==(fUnknown+tNum) || fv==(fUnknown+tString))) fs=fUnknown+tNum;
        }else if(op=='*'){
            if((fs&tType)==tString || (fs&tType)==tList) throw("Terms cannot be strings or lists");
            if(nxtTok("+")) op='+'; else if(nxtTok("-")) {op='+'; fv|=fInvert;}
            if (op=='+'){size=ReadPureInfon(&iVal,&fv,&wFlag,&s2);}
            else {iVal=0; fv=tNum;}    // use identity summand '0'
        } else { // no operator
            if(nxtTok(";")){iVal=0; fv=fUnknown;}
            else {
                nxtTok(",");
                iVal=iSize;fv=fs; fs=tNum; iSize=(infon*)size;
                if (size==0 && (fv==(fUnknown+tNum) || fv==(fUnknown+tString))) fs=fUnknown+tNum; // set size's flags for _ and $
                else if(((fv&tType)==tList) && iVal && ((iVal->prev)->pFlag)&isVirtual) {fs|=fUnknown;} // set size's flags for {...}
            }
        }
    }
    infon* i=new infon((fs<<goSize)+fv+pFlag, wFlag, iSize,iVal,0,s1,s2); i->wSize=size; i->type=tags;
    if(i->pFlag&fConcat && i->size==(infon*)1){infon* ret=i->value; delete(i); return ret;} // BUT we lose i's idents and some flags (desc, ...)
    if ((i->size && ((fs&tType)==tList))||(fs&fConcat)) i->size->top=i;
    if ((i->value&& ((fv&tType)==tList))||(fv&fConcat))i->value->top=i;
    if ((i->wFlag&mFindMode)==iGetLast){i->wFlag&=~(mFindMode+mAssoc); i->wFlag|=mIsHeadOfGetLast; i=new infon(0,wFlag,0,0,0,i); i->spec1->top2=i;}
    for(char c=Peek(); !(noIDs&1) && (c==':' || c=='='); c=Peek()){
        infon *R, *toSet=0, *toRef=0; int code=0;
        cTok=nxtTokN(2,"::",":");
        eTok=nxtTokN(2,"==","=");
        if(isEq(cTok,":") && (eTok==0)){
            if(peek()=='>') {streamPut(1); break;}
            toRef=i; i=ReadInfon(0); toSet=grok(i,c1Left,&code);
            if((toRef->wFlag&mFindMode)!=iTagUse) toRef->top=i;
        } else {
            cTok2=nxtTokN(2,"::",":");
            if(isEq(cTok,":")){
                toSet=grok(i,c1Left,&code); R=ReadInfon(4);
                if((i->wFlag&mFindMode)>=iGetLast) {
                    i->spec1->top=R;
                    if((noIDs&4)==0){ //  set 'top' for any \\^, etc.
                        infon *p, *q;
                        for(p=i; (p->wFlag&mFindMode)==iGetLast; p=q){
                            q=p->spec1->top; q->top=i; p->spec1->top=0;
                        }
                    }
                }
            } else if(isEq(cTok,"::")){
                // Add a new head to the spec list.
                toSet=grok(i,c2Left,&code); code=InitSearchList; R=ReadInfon(4);
            } else {toSet=i; R=ReadInfon(1);}
            if(isEq(eTok,"==")) {code|=mMatchType;}
            if(isEq(cTok2,":")){toRef=grok(R,c1Right,&code);}
            else if(isEq(cTok2,"::")){toRef=grok(R,c2Right,&code);}
            else toRef=R;
        }
        if(toSet==0) throw ":= operator requires [....] on the left side";
        if(toRef==0) throw "=: operator requires [....] on the right side";
        insertID(&toSet->wrkList, toRef, code);
    }
    if(!(noIDs&2)){  // load function "calls"
        if(nxtTok(":>" )) {infon* j=ReadInfon(1); j->wFlag|=sUseAsFirst; j->spec2=i; i=j; chk4HardFunc(i);}
        else if(nxtTok("<:")) {i->wFlag|=sUseAsFirst; i->spec2=ReadInfon(1);  chk4HardFunc(i);}
        else if(nxtTok("!>")) {}
        else if(nxtTok("<!")){}
    }
    /*textEnd=textParsed.size(); */
    return i;
}

infon* QParser::parse(){
    char tok;
    try{
        textParsed="";
        line=1; scanPast((char*)"<%");
        infon*i=ReadInfon();
        //std::cout<<"\n["<<textParsed<<"]\n";
        check('%'); check('>'); buf[0]=0; return i;
    }
    catch(char const* err){char l[30]; itoa(line,l); strcpy(buf,"An Error Occured: "); strcat(buf,err);
        strcat(buf,". (line ");strcat(buf,l); strcat(buf,")\n");}
    catch(...){strcpy(buf,"An Unknown Error Occurred While Parsing.\n");};
    if(stream.fail()) std::cout << "End of File Reached";
    return 0;
}
