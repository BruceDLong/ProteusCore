/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2008 Bruce Long
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

std::string printHTMLHeader(std::string ItemToNorm){
    std::string s;
    s="<html><head><title>Normalize Log</title></head><body>\n";
    s+="<h2>Text that was parsed</h2>\n<tt>"; s+=ItemToNorm; s+="</tt><br><hr>";
    return s;
}

std::string printHTMLFooter(std::string ErrorMsg){
    std::string s;
    s="<hr>Result: "; s+=ErrorMsg; s+="<br></body></html>\n";
    return s;
}

#define Indent {for (int x=0;x<indent;++x) s+=" ";}
std::string printPure (infon* i, uint f, uint wSize, infon* CI){
    std::string s;
    if ((f&fIncomplete+fUnknown) && !(f&tUInt)) s+="?";
    uint type=f&tType;
    if (type==tUInt){
        if (f & fUnknown) s+='_';
        else {char buf[70]; itoa((uint)i, buf); s.append(buf);}
    } else if(type==tString){s+="\"";s.append((char*)i,wSize);s+="\""; }
    else if(type==tList||(f&fConcat)){
        s+=(f&fConcat)?"(":"{";
        for(infon* p=i;p;) {
            if(p==i && f&fLoop && i->spec2){printInfon(i->spec2,CI); s+=" | ";}
            s+=printInfon(p, CI); s+=' ';
            if (p->flags&isBottom) p=0; else p=p->next;
        }
        s+=(f&fConcat)?")":"}";
    }
    return s;
}
std::string printInfon(infon* i, infon* CI){
    std::string s; //Indent;
    if(i==0) {s+="null"; return s;}
    if(i==CI)s+="<font color=green>";
    uint f=i->flags;
    uint mode=f&mRepMode;
//    if(f&toExec) s+="@";
    if(f&asDesc) s+="#";
    if (mode==asTag) {
       char* ch=((stng*)(i->spec1))->S;
        for(int x=((stng*)(i->spec1))->L;x;--x){
            s+=(*ch=='\0')?' ':ch++[0];
        }s+=' ';
    } else if(f&isNormed || mode==asNone /* || f&notParent */){
        if (f&(fUnknown<<goSize)) {s+=printPure(i->value, f, i->wSize, CI); if((f&tType)!=tList) s+=",";}
        else if (f&fUnknown) {s+=printPure(i->size, f>>goSize,i->wSize, CI); s+=";";}
        else{
            if((f&tType)==tUInt) {s+=((f>>goSize)&fInvert)?"/":"*"; s+=printPure(i->size, f>>goSize, 0,CI);}
             if((f&tType)==tUInt) s+=(f&fInvert)?"-":"+";
            s+=printPure(i->value,f, (uint)i->size, CI);
        }
    } else {
        if (!(f&isNormed)) {
            switch (f&mRepMode){
            case toGiven: s+="%["; s+=printInfon(i->spec1, CI); s+="]"; break;
            case toWorldCtxt: s+="&"; break;
            case toHomePos: for(int h=(int)(i->spec1); h; h--) s+="\\["; s+=printInfon(i->spec2,CI); s+="]"; break;
            case fromHere: s+="^"; break;
            case asFunc:  s+="[";s+=printInfon(i->spec2, CI); s+="]:";s+=printInfon(i->spec1, CI); break;
            }
        }
    }
    if (!(f&isNormed) && i->wrkList) {
        infNode* k=i->wrkList;
        do {k=k->next; s+="="; s+="IDx "; /*printInfon(k->item,CI);*/} while(k!=i->wrkList);
    }
    if (i==CI) s+="</font>";
    return s;
}

void QParser::scanPast(char* str){
    char p; char* ch=str;
    while(*ch!='\0'){
        p=stream.get();
        if (stream.eof() || stream.fail()) throw "Expected String not found before end-of-file";
        if (*ch==p) ch++; else ch=str;
        if (p=='\n') ++line;
    }
}

void QParser::RmvWSC (){
    char p,p2;
    for (p=stream.peek(); (p==' '||p=='/'||p=='\n'||p=='\r'||p=='\t'); p=stream.peek()){
        if (p=='/') {
            stream.get(); p2=stream.peek();
            if (p2=='/') {
                for (p=stream.peek(); !(stream.eof() || stream.fail()) && p!='\n'; p=stream.peek()) stream.get();
            } else if (p2=='*') {
                for (p=stream.get(); !(stream.eof() || stream.fail()) && !(p=='*' && stream.peek()=='/'); p=stream.get())
                    if (p=='\n') ++line;
                if (stream.eof() || stream.fail()) throw "'/*' Block comment never terminated";
            } else {stream.putback('/'); return;}
        }
        if (stream.get()=='\n') ++line;
    }
}

char QParser::peek(){
    if (stream.fail()) throw "Unexpected end of file";
    return stream.peek();
}

#define ChkNEOF {if(stream.eof() || stream.fail()) throw "Unexpected End of file";}
#define getToken(tok) {RmvWSC(); ChkNEOF; tok=stream.get();}
#define getbuf(c) {ChkNEOF; for(p=0;(c);buf[p++]=stream.get()){if (p>=bufmax) throw "String Overflow";} buf[p]=0;}
#define readTag(tag)  {getbuf(iscsym(peek())); if(!p){throw "Tag expected";} else {lstngCpy(tag,buf,p);}}
#define Peek(tok) {RmvWSC(); tok=stream.peek();}
#define check(ch) {getToken(tok);  if(tok != ch) throw "Unexpected character";}
#define chk(ch) {if(stream.peek()==ch) stream.get(); else throw "Expected something else";}

char errMsg[100];
uint QParser::ReadPureInfon(char &tok, infon** i, uint* flags, infon** s2){
    uint p=0, size=0, stay=1; char rchr; infon *head=0, *prev; infon* j;
    //if(tok=='$'){*flags|=fLoop+tList+fUnknown; *i=ReadInfon(1); return 0;}
    if(tok=='('||tok=='{'||tok=='['){
        if(tok=='(') {rchr=')'; *flags|=(fConcat+tUInt);}
        else if(tok=='['){rchr=']'; *flags|=tList;}
        else {rchr='}'; *flags|=tList;}
        RmvWSC(); int foundRet=0; int foundBar=0;
        for(tok=peek(); tok != rchr && stay; tok=peek()){
            if(tok=='<') {foundRet=1; getToken(tok); j=ReadInfon();}
            else if(tok=='.'){stream.get();chk('.');chk('.');
                j=new infon(fUnknown+isVirtual+asNone+(tUInt<<goSize),(infon*)(size+1));stay=0;}
            else j=ReadInfon();
            if(++size==1){
                Peek(tok);
                if(!foundRet && !foundBar && stay && tok=='|'){
                    getToken(tok); *s2=j; *flags|=fLoop; size=0; foundBar=1; RmvWSC(); continue;
                }
                *i=head=prev=j; head->flags|=isFirst+isTop;if(*flags&fConcat)*flags|=(j->flags&tType);
            }
            if (foundRet==1) {check('>'); *i=j; foundRet++; *flags|=intersect;}
            j->top=head; j->next=head; prev->next=j; j->prev=prev; head->prev=j;
            prev=j; RmvWSC();
        }
        if(head){
            head->prev->flags|=isBottom;
            if (!(head->flags&fIncomplete) && stay) head->prev->flags|=isLast;
        }
        check(rchr);
    } else if (isdigit(tok)) {   // read number
        stream.putback(tok); getbuf(isdigit(peek()));
        *flags+=tUInt; *i=(infon*)atoi(buf);
        size=1;
    } else if (tok=='_'){*flags+=fUnknown+tUInt;
    } else if (tok=='$'){*flags+=fUnknown+tString;
    } else if (tok=='"' || tok=='\''){   // read string
        getbuf(peek()!=tok); stream.get();
        *flags+=tString; *i=(((*flags)&fUnknown)? 0 : (infon*)new char[p]); memcpy(*i,buf,p);
        size=p;
    } else {strcpy(errMsg, "'X' was not expected"); errMsg[1]=tok; throw errMsg;}
    return size;
}

infon* QParser::ReadInfon(int noIDs){
    char tok, op=0; uint p=0, size=0; infon*i1=0,*i2=0,*s1=0,*s2=0; uint flags=0,f1=0,f2=0;
    getToken(tok); //DEB(tok)
    if(tok=='@'){flags|=toExec; getToken(tok);}
    if(tok=='#'){flags|=asDesc; getToken(tok);}
    if(tok=='?'){f1=f2=fUnknown; flags=asNone;}
    else if(iscsym(tok)&&!isdigit(tok)&&(tok!='_')){
        stream.putback(tok); flags|=asTag; stng tag; stng* tags=new stng;
        while(iscsym(tok)&&!isdigit(tok)){
            readTag(tag);
            stngApnd((*tags),tag.S,tag.L+1);
            Peek(tok);
        }
        s1=(infon*)tags;
    }
    else if(tok=='\\'||tok=='%'||tok=='&'||tok=='^'){
        flags|=fUnknown;
        if (tok=='%'){flags|=toGiven; s1=ReadInfon();} // % search
        else if (tok=='\\') {
            flags|=toHomePos; s1=0;
            while(tok=='\\') {s1=(infon*)((uint)s1+1); ChkNEOF; tok=stream.get();}stream.putback(tok);
        } else if (tok=='&') flags|=toWorldCtxt;
        else if (tok=='^') flags|=fromHere;
        s2=ReadInfon(3);
    }else{
        flags|=asNone;
        if(tok=='~') {f1|=fInvert; getToken(tok);}
        if(tok=='+' || tok=='-')op='+'; else if(tok=='*'||tok=='/')op='*';
        if(tok=='-'||tok=='/') {f1^=fInvert;}
        if(op) getToken(tok);
        size=ReadPureInfon(tok, &i1, &f1, &s2);
        if(op=='+'){
            i2=i1; i1=(infon*)size; f2=f1; f1=tUInt; // use identity term '1'
            if (size==0 && (f2==(fUnknown+tUInt) || f2==(fUnknown+tString))) f1=fUnknown+tUInt;
        }else if(op=='*'){
            if((f1&tType)==tString){throw("Terms cannot be strings");}
            if((f1&tType)==tList){throw("Terms cannot be lists");}
            getToken(tok); if(tok=='~'){f2|=fInvert; getToken(tok);}
            if(tok=='-'||tok=='+'){op='+'; if(tok=='-')f2^=fInvert; getToken(tok);}
            else {stream.putback('~'); f2^=fInvert;}
            if (op=='+'){size=ReadPureInfon(tok,&i2,&f2,&s2);}
            else {i2=0; f2=tUInt;}    // use identity summand '0'
        } else { // no operator
            Peek(tok);
            if(tok==','||tok==')'||tok=='}'||tok==']'||((f1&tType)!=tUInt)){
                i2=i1;f2=f1; f1=tUInt; i1=(infon*)size;
                if (size==0 && (f2==(fUnknown+tUInt) || f2==(fUnknown+tString))) f1=fUnknown+tUInt;
                else if(((f2&tType)==tList) && i2 && ((i2->prev)->flags)&isVirtual) {f1|=fUnknown;}
                if(tok==',')getToken(tok);
            }else if(tok==';'){i2=0; f2=fUnknown; getToken(tok);}
        }
    }
    Peek(tok);
    infNode *ID=0, *IDp=0; uint modeBit=0;
    if(!(noIDs&1)) while(tok=='=') {
        getToken(tok);
        infon* tmp= ReadInfon(1); insertID(&ID, tmp,0);
        Peek(tok);
    }
    infon* i=new infon((f1<<goSize)+f2+flags, i1,i2,ID,s1,s2);
    i->wSize=size;
    if (i->size && ((f1&tType)==tList)||(f1&fConcat)) i->size->top=i;
    if (i->value&& ((f2&tType)==tList)||(f2&fConcat))i->value->top=i;
    if(!(noIDs&2)){
        if (tok=='!'){modeBit=mMode; getToken(tok); if(peek()!=':') throw"Expected ':'";}
        if (tok==':'){getToken(tok); i=new infon(asFunc+modeBit,0,0,0,ReadInfon(1),i);}
    }
    return i;
}

infon* QParser::parse(){
    char tok;
    try{
        line=1; scanPast((char*)"<%");
        infon*i=ReadInfon();
        check('%'); check('>'); buf[0]=0; return i;
    }
    catch(char const* err){char l[30]; itoa(line,l); strcpy(buf,"An Error Occured: "); strcat(buf,err);
        strcat(buf,". (line ");strcat(buf,l); strcat(buf,")\n");}
    catch(...){strcpy(buf,"An Unknown Error Occurred While Parsing.\n");};
    if(stream.fail()) std::cout << "End of File Reached";
    return 0;
}

