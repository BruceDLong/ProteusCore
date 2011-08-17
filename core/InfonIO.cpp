/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2011 Bruce Long
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
#include "Proteus.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include "remiss.h"

#define Indent {for (int x=0;x<indent;++x) s+=" ";}
std::string printPure (infon* i, UInt f, UInt wSize, infon* CI){
    std::string s;
    if ((f&(fIncomplete+fUnknown)) && !(f&tUInt)) s+="?";
    UInt type=f&tType;
    if (type==tUInt){
        if (f & fUnknown) s+='_';
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
    if(i==CI)s+="<font color=green>";
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
            if((f&tType)==tUInt) {s+=((f>>goSize)&fInvert)?"/":"*"; s+=printPure(i->size, f>>goSize, 0,CI);}
             if((f&tType)==tUInt) s+=(f&fInvert)?"-":"+";
            s+=printPure(i->value,f, (UInt)i->size, CI);
        }
    } else {
        if (!(f&isNormed)) {
            if(i->wFlag&iToWorld) s+="%W ";
            if(i->wFlag&iToCtxt) s+="%C";
            if(i->wFlag&iToArgs) s+="%A ";
            if(i->wFlag&iToVars) s+="%V ";
            switch (f&mRepMode){
            case toGiven: s+="%["; s+=printInfon(i->spec1, CI); s+="]"; break;
            case toWorldCtxt: s+="&"; break;
            case toHomePos: for(UInt h=(UInt)(i->spec1); h; h--) s+="\\["; s+=printInfon(i->spec2,CI); s+="]"; break;
            case fromHere: s+="^"; break;
            case asFunc:  s+="[";s+=printInfon(i->spec2, CI); s+="]<:";s+=printInfon(i->spec1, CI); break;
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

char QParser::streamGet(){
	char ch=stream.get();
	textParsed+=ch;
	return ch;
}
#define streamPut(nChars) {for(int n=nChars; n>0; --n){stream.putback(textParsed[textParsed.size()-1]); textParsed.resize(textParsed.size()-1);}}

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

char QParser::peek(){
    if (stream.fail()) throw "Unexpected end of file";
    return stream.peek();
}

#define ChkNEOF {if(stream.eof() || stream.fail()) throw "Unexpected End of file";}
#define getToken(tok) {RmvWSC(); ChkNEOF; tok=streamGet();}
#define getbuf(c) {ChkNEOF; for(p=0;(c);buf[p++]=streamGet()){if (p>=bufmax) throw "String Overflow";} buf[p]=0;}
//#define readTag(tag)  {getbuf(iscsym(peek())); if(!p){throw "Tag expected";} else {lstngCpy(tag,buf,p);}}
#define Peek(tok) {RmvWSC(); tok=stream.peek();}
#define check(ch) {getToken(tok);  if(tok != ch) {std::cout<<"Expected "<<ch<<"\n"; throw "Unexpected character";}}
#define chk(ch) {if(stream.peek()==ch) streamGet(); else throw "Expected something else";}
                                                                                                                     

const char* altTok(std::string tok){
    // later this should load from a dictionary file for different languages.
    if (tok=="=") return "is";
    if (tok==": =") return "'s";
    if (tok=="= :") return "of";
    if (tok=="%C") return "the";
    if (tok=="%W") return "thee";
    if (tok=="%A") return "%Arg";
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
const int QParser::nxtTokN(int n, ...){
    char* tok; va_list ap; va_start(ap,n); int i,p;
    for(i=n; i; --i){
        tok=va_arg(ap, char*); Peek(ch);
        if(strcmp(tok,"ABC")==0) {if(iscsym(ch)&&!isdigit(ch)&&(ch!='_')) {getbuf(iscsym(peek())); break;}}
        else if(strcmp(tok,"123")==0) {if(isdigit(ch)) {getbuf(isdigit(peek())); break;}}
        else if (chkStr(tok) || chkStr(altTok(tok))) break;
    }
    va_end(ap);
    return i;
}
#define nxtTok(tok) nxtTokN(1,tok)

char errMsg[100];
UInt QParser::ReadPureInfon(infon** i, UInt* pFlag, UInt *wFlag, infon** s2){
    UInt p=0, size=0, stay=1; char rchr; infon *head=0, *prev; infon* j;
    if(nxtTok("(") || nxtTok("{") || nxtTok("[")){
        if(ch=='(') {rchr=')'; *pFlag|=(fConcat+tUInt);}
        else if(ch=='['){rchr=']'; *pFlag|=(tList+fUnknowns); *wFlag=iGetLast;}
        else {rchr='}'; *pFlag|=tList;}
        RmvWSC(); int foundRet=0; int foundBar=0;
        for(char tok=peek(); tok != rchr && stay; tok=peek()){
            if(tok=='<') {foundRet=1; getToken(tok); j=ReadInfon();}
            else if(tok=='.'){
                streamGet();
                if(stream.peek()=='.'){
                    streamGet(); chk('.');
                    j=new infon(fUnknown+isVirtual+(tUInt<<goSize),iNone,(infon*)(size+1));stay=0;
                    } else {streamPut(1);  j=ReadInfon();}
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
    } else if (nxtTok("123")) {   // read number
        *pFlag+=tUInt; 
        *i=(infon*)atoi(buf);
        size=1;
    } else if (nxtTok("_")){*pFlag+=fUnknown+tUInt;
    } else if (nxtTok("$")){*pFlag+=fUnknown+tString;
    } else if (nxtTok("\"") || nxtTok("'")){   // read string
        getbuf(peek()!=ch); streamGet();
        *pFlag+=tString; *i=(((*pFlag)&fUnknown)? 0 : (infon*)new char[p]); memcpy(*i,buf,p);
        size=p;
    } else {strcpy(errMsg, "'X' was not expected"); errMsg[1]=ch; throw errMsg;}
    return size;
}

infon* QParser::ReadInfon(int noIDs){
    char tok, op=0; UInt size=0; infon*iSize=0,*iVal=0,*s1=0,*s2=0; UInt pFlag=0,wFlag=0,fs=0,fv=0; infNode *IDp=0;
	/*int textStart=textParsed.size();*/ int textEnd=0; stng* tags=0;
    if(nxtTok("@")){pFlag|=toExec;}
    if(nxtTok("#")){pFlag|=asDesc;}
    if(nxtTok(".")){pFlag|=matchType;} // This is a hint that idents must match type, not just value.
    if(nxtTok("?")){fs=fv=fUnknown; wFlag=iNone;}
    else if(nxtTok("ABC")){
        wFlag|=iTagUse; tags=new stng;
        // if(iscsym(tok)&&!isdigit(tok)){  // change 'if' to 'do-while' when tag-chains are ready.
            stngApnd((*tags),buf,strlen(buf)+1);
    }else if( nxtTok("%")){
        pFlag|=fUnknown;
        if (nxtTok("W")){std::cout<<"WORLD"<<"\n"; wFlag|=iToWorld;}
        if (nxtTok("C")){wFlag|=iToCtxt;}
        if (nxtTok("A")){wFlag|=iToArgs;}
        if (nxtTok("V")){wFlag|=iToVars;}
        
    }else if(nxtTok("\\") || nxtTok("^")){
        pFlag|=fUnknown; // BROKEN TOK USAGE
        if (tok=='\\' || tok=='^') {
            for(s1=0; tok=='\\';  tok=streamGet()) {s1=(infon*)((UInt)s1+1); ChkNEOF;}
             if (tok=='^') wFlag|=iToPathH; else {wFlag|=iToPath; streamPut(1);}
        }
        if(nxtTok(".")) {
            s2=(infon*)new assocInfon(new infon(pFlag, wFlag, iSize,iVal,0,s1,s2));
            wFlag=iStartAssoc; fs=fv=fUnknown;
        }
    }else{  // OK, then we're parsing some form of *... +...
        wFlag|=iNone;
        if(nxtTok("+") || nxtTok("-")) op='+'; else if(nxtTok("*") || nxtTok("/")) op='*'; 
        if( ch=='-' || ch=='/') fs|=fInvert;
        size=ReadPureInfon(&iSize, &fs, &wFlag, &s2);
        if(op=='+'){
            iVal=iSize; iSize=(infon*)size; fv=fs; fs=tUInt; // use identity term '1'
            if (size==0 && (fv==(fUnknown+tUInt) || fv==(fUnknown+tString))) fs=fUnknown+tUInt;
        }else if(op=='*'){
            if((fs&tType)==tString || (fs&tType)==tList) throw("Terms cannot be strings or lists");
            if(nxtTok("+")) op='+'; else if(nxtTok("-")) {op='+'; fv|=fInvert;}
            if (op=='+'){size=ReadPureInfon(&iVal,&fv,&wFlag,&s2);}
            else {iVal=0; fv=tUInt;}    // use identity summand '0'
        } else { // no operator
            if(nxtTok(";")){iVal=0; fv=fUnknown;}
            else {
                nxtTok(",");
                iVal=iSize;fv=fs; fs=tUInt; iSize=(infon*)size;
                if (size==0 && (fv==(fUnknown+tUInt) || fv==(fUnknown+tString))) fs=fUnknown+tUInt; // TODO: code review these two lines
                else if(((fv&tType)==tList) && iVal && ((iVal->prev)->pFlag)&isVirtual) {fs|=fUnknown;}
            }
        }
    }
    infon* i=new infon((fs<<goSize)+fv+pFlag, wFlag, iSize,iVal,0,s1,s2); i->wSize=size; i->type=tags;
    while (!(noIDs&1) && (op=nxtTokN(4, ": = :", ": =", "= :", "="))){
        infon* tmp= ReadInfon(1); 
        insertID(&i->wrkList, tmp, op-1); // BUT those with []= should insert into GetLast parent.
    }
    if(i->pFlag&fConcat && i->size==(infon*)1){infon* ret=i->value; delete(i); return ret;} // BUT we lose i's idents and some flags (desc, ...)
    if ((i->size && ((fs&tType)==tList))||(fs&fConcat)) i->size->top=i;
    if ((i->value&& ((fv&tType)==tList))||(fv&fConcat))i->value->top=i;
    if ((i->wFlag&mFindMode)==iGetLast){i->wFlag&=~mFindMode;i=new infon(0,iGetLast,0,0,0,i);}
    if(!(noIDs&2)){  // load function "calls"
        if(nxtTok(":>" )) {i=new infon(0,sUseAsFirst,0,0,0,i,ReadInfon(1));}
        else if(nxtTok("!>") ) {i=new infon(0,sUseAsLast+iGetFirst,0,0,0,i,ReadInfon(1));}
        else if(nxtTok("<:")) {i->wFlag|=sUseAsFirst; i->spec2=ReadInfon(1); }
        else if(nxtTok("<!")){i=new infon(0,sUseAsLast+iGetFirst,0,0,0,ReadInfon(1),i);}
    }
	textEnd=textParsed.size();
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

