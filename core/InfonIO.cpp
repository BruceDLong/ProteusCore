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
    if(i==CI) s+="<font color=green>";
    UInt f=i->pFlag;
    UInt mode=i->wFlag&mFindMode;
//    if(f&toExec) s+="@";
    if(f&asDesc) s+="#";
    if (mode==iTagUse) { s+="TAGG\n";
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
            if(mode==iToWorld) s+="%W ";
            else if(mode==iToCtxt) s+="%C";
            else if(mode==iToArgs) s+="%A ";
            else if(mode==iToVars) s+="%V ";
            else if(mode==iToPath || mode==iToPathH){
                s+="%";
                for(UInt h=(UInt)(i->spec1); h; h--) s+="\\";
                if(mode==iToPathH) s+="^";
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
const char* QParser::nxtTokN(int n, ...){
    char* tok; va_list ap; va_start(ap,n); int i,p;
    for(i=n; i; --i){
        tok=va_arg(ap, char*); Peek(nTok);
        if(strcmp(tok,"ABC")==0) {if(iscsym(nTok)&&!isdigit(nTok)&&(nTok!='_')) {getbuf(iscsym(peek())); break;} else tok=0;}
        else if(strcmp(tok,"123")==0) {if(isdigit(nTok)) {getbuf(isdigit(peek())); break;} else tok=0;}
        else if (chkStr(tok) || chkStr(altTok(tok))) break; else tok=0;
    }
    va_end(ap);
    return tok;
}
#define nxtTok(tok) nxtTokN(1,tok)

infon* grok(infon* item, UInt tagCode, int* code){
    if((item->wFlag&mFindMode)==iTagUse) {(*code)|=tagCode; return item;}
    else if((item->wFlag&mFindMode)==iGetLast) {return item->spec1;}
    else return 0;
}

char errMsg[100];
UInt QParser::ReadPureInfon(infon** i, UInt* pFlag, UInt *wFlag, infon** s2){
    UInt p=0, size=0, stay=1; char rchr, tok; infon *head=0, *prev; infon* j;
    if(nxtTok("(") || nxtTok("{") || nxtTok("[")){
        if(nTok=='(') {rchr=')'; *pFlag|=(fConcat+tUInt);}
        else if(nTok=='['){rchr=']'; *pFlag|=(tList+fUnknowns); *wFlag=iGetLast;}
        else {rchr='}'; *pFlag|=tList;}
        RmvWSC(); int foundRet=0; int foundBar=0;
        for(tok=peek(); tok != rchr && stay; tok=peek()){
            if(tok=='<') {foundRet=1; getToken(tok); j=ReadInfon();}
            else if(nxtTok("...")){
                j=new infon(fUnknown+isVirtual+(tUInt<<goSize),iNone,(infon*)(size+1));stay=0;
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
        getbuf(peek()!=nTok); streamGet();
        *pFlag+=tString; *i=(((*pFlag)&fUnknown)? 0 : (infon*)new char[p]); memcpy(*i,buf,p);
        size=p;
    } else {strcpy(errMsg, "'X' was not expected"); errMsg[1]=nTok; throw errMsg;}
    return size;
}

infon* QParser::ReadInfon(int noIDs){
    char op=0; UInt size=0; infon*iSize=0,*iVal=0,*s1=0,*s2=0; UInt pFlag=0,wFlag=0,fs=0,fv=0; infNode *IDp=0;
	stng* tags=0; const char* cTok; int textEnd=0; /*int textStart=textParsed.size();*/
    if(nxtTok("@")){pFlag|=toExec;}
    if(nxtTok("#")){pFlag|=asDesc;}
    if(nxtTok(".")){pFlag|=matchType;} // This is a hint that idents must match type, not just value.
    if(nxtTok("?")){fs=fv=fUnknown; wFlag=iNone;}
    else if(nxtTok("ABC")){
        wFlag|=iTagUse; tags=new stng;
        // if(iscsym(tok)&&!isdigit(tok)){  // change 'if' to 'do-while' when tag-chains are ready.
            stngApnd((*tags),buf,strlen(buf)+1);
    }else if( nxtTok("%")){ // TODO: Don't allow these outside of := or :
        pFlag|=fUnknown;
        if (nxtTok("W")){std::cout<<"WORLD"<<"\n"; wFlag|=iToWorld; }
        else if (nxtTok("C")){wFlag|=iToCtxt;}
        else if (nxtTok("A")){wFlag|=iToArgs;}
        else if (nxtTok("V")){wFlag|=iToVars;}
        else if(nxtTok("ABC")){wFlag|=iTagDef; tags=new stng; stngApnd((*tags),buf,strlen(buf)+1);}
        else if (nTok=='\\' || nTok=='^'){
            for(s1=0; (nTok=streamGet())=='\\';) {s1=(infon*)((UInt)s1+1); ChkNEOF;}
            if (nTok=='^') wFlag|=iToPathH; else {wFlag|=iToPath; streamPut(1);}
        }
        if(nxtTok(".")) {
            s2=(infon*)new assocInfon(new infon(pFlag, wFlag, iSize,iVal,0,s1,s2));
            wFlag=iStartAssoc; fs=fv=fUnknown;
        }
    }else{  // OK, then we're parsing some form of *... +...
        wFlag|=iNone;
        if(nxtTok("+") || nxtTok("-")) op='+'; else if(nxtTok("*") || nxtTok("/")) op='*'; 
        if( nTok=='-' || nTok=='/') fs|=fInvert;
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
    if(i->pFlag&fConcat && i->size==(infon*)1){infon* ret=i->value; delete(i); return ret;} // BUT we lose i's idents and some flags (desc, ...)
    if ((i->size && ((fs&tType)==tList))||(fs&fConcat)) i->size->top=i;
    if ((i->value&& ((fv&tType)==tList))||(fv&fConcat))i->value->top=i;
    if ((i->wFlag&mFindMode)==iGetLast){i->wFlag&=~mFindMode;i=new infon(0,iGetLast,0,0,0,i);}
    while (!(noIDs&1) && (cTok=nxtTokN(5, ": = :", ": =", "= :", ":", "="))){
        std::string tok=cTok; infon *R, *toSet=0, *toRef=0; int code=0;
        if(tok==":"){
            if(peek()=='>') {streamPut(1); break;}
            toRef=i; i=ReadInfon(0); toSet=grok(i,0x200,&code); 
            if((toRef->wFlag&mFindMode)!=iTagUse) toRef->top=i;}
        else { 
            if(tok==": =" || tok==": = :"){
                toSet=grok(i,0x200,&code); R=ReadInfon(4);
                if((i->wFlag&mFindMode)==iGetLast) {
                    i->spec1->top=R;
                    if((noIDs&4)==0){
                        infon *p, *q;
                        for(p=i; (p->wFlag&mFindMode)==iGetLast; p=q){
                            q=p->spec1->top; q->top=i; p->spec1->top=0;
                        }
                    }
                }
            } else {toSet=i; R=ReadInfon(1);}
            if(tok=="= :" || tok==": = :"){toRef=grok(R,0x100,&code);}
            else toRef=R;
        }
        if(toSet==0) throw ":= operator requires [....] on the left side";
        if(toRef==0) throw "=: operator requires [....] on the right side";
        insertID(&toSet->wrkList, toRef, code);
    }
    if(!(noIDs&2)){  // load function "calls"
        if(nxtTok(":>" )) {infon* j=ReadInfon(1); j->wFlag|=sUseAsFirst; j->spec2=i; i=j;}
        else if(nxtTok("<:")) {i->wFlag|=sUseAsFirst; i->spec2=ReadInfon(1); }
        else if(nxtTok("!>")) {}
        else if(nxtTok("<!")){}
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

