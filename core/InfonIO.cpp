/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2012 Bruce Long
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

#include <unicode/putil.h>
#include <unicode/uniset.h>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>
using namespace icu;

#define Indent {for (int x=0;x<indent;++x) s+=" ";}
std::string printPure (pureInfon* i, UInt f, UInt wSize, infon* CI){
    std::string s;
    if (((f&(fIncomplete+mFormat))==(fIncomplete+fUnknown)) && ((f&tType)!=tNum)) s+="?";
    UInt type=f&tType;
    if (type==tNum){
        if (FormatIsUnknown(f)) s+='_';
        else if(FormatIs(f,fFloat)){s.append("FLOAT.float");}
        else if(FormatIs(f,fLiteral)){s.append(i->dataHead->get_str());}
    } else if(type==tString){s+="\"";s.append(i->toString(wSize));s+="\""; }
    else if(type==tList || FormatIsConcat(f)){
        s+=(FormatIsConcat(f))?"(":"{";
        for(infon* p=i->listHead;p;) {
           // if(p==i->listHead && f&fLoop && ((infon*)i)->spec2){printInfon(((infon*)i)->spec2,CI); s+=" | ";} // TODO: when overhauling printing, use this.
            if(InfIsTentative(p)) {s+="..."; break;}
            else {s+=printInfon(p, CI); s+=' ';}
            if (InfIsBottom(p)) p=0; else p=p->next;
        }
        s+=(FormatIsConcat(f))?")":"}";
    }
    return s;
}

std::string printInfon(infon* i, infon* CI){
    std::string s; //Indent;
    if(i==0) {s+="null"; return s;}
    if(i==CI) s+="<font color=green>";
    UInt mode=i->wFlag&mFindMode;
//    if(InfToExec(i)) s+="@";
    if(InfAsDesc(i)) s+="#";
    if (mode==iTagUse) {
        s+=i->type->tag; s+=" ";
    } else if(InfIsNormed(i) || mode==iNone /* || VsFlag(i)&notParent */){
        if (SizeIsUnknown(i)) {s+=printPure(&i->value, VsFlag(i), i->wSize, CI); if(InfsType(i) != tList) s+=",";}
        else if (ValueIsUnknown(i)) {s+=printPure(&i->size, SsFlag(i), i->wSize, CI); s+=";";}
        else{
            if(InfsType(i)==tNum) {s+=(SsFlag(i)&fInvert)?"/":"*"; s+=printPure(&i->size, SsFlag(i), 0,CI);}
            if(InfsType(i)==tNum) s+=(VsFlag(i)&fInvert)?"-":"+";
            s+=printPure(&i->value, VsFlag(i), i->getSize().get_ui(), CI);
        }
    } else {
        if (!InfIsNormed(i)) {
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
    if (!InfIsNormed(i) && i->wrkList) {
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

#include "unicode/uspoof.h"
#include "unicode/unorm.h"
#define U8_IS_SINGLE(c) (((c)&0x80)==0)
UErrorCode err=U_ZERO_ERROR;
const icu::Normalizer2 *tagNormer=Normalizer2::getNFKCCasefoldInstance(err);
const UnicodeSet TagChars(UnicodeString("[:XID_Continue:]"), err);
const UnicodeSet TagStarts(UnicodeString("[:XID_Start:]"), err);
bool iscsymOrUni (char nTok) {return (iscsym(nTok) || (nTok&0x80));}
bool isTagStart(char nTok) {return (iscsymOrUni(nTok)&&!isdigit(nTok)&&(nTok!='_'));}

bool tagIsBad(std::string tag, const char* locale) {
    if(!TagStarts.contains(tag.c_str()[0])) return 1; // First tag character is invalid
    if((size_t)TagChars.spanUTF8(tag.c_str(), -1, USET_SPAN_SIMPLE) != tag.length()) return 1;
    UErrorCode err;  USpoofChecker *sc = uspoof_open(&err);
    uspoof_setChecks(sc, USPOOF_SINGLE_SCRIPT|USPOOF_INVISIBLE, &err);
    uspoof_setAllowedLocales(sc, locale, &err);
    int result=uspoof_checkUTF8(sc, tag.c_str(), -1, 0, &err);
    return (result!=0);
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
bool isBinDigit(char ch) {return (ch=='0' || ch=='1');}

const char* QParser::nxtTokN(int n, ...){
    char* tok; va_list ap; va_start(ap,n); int i,p;
    for(i=n; i; --i){
        tok=va_arg(ap, char*); nTok=Peek();
        if(strcmp(tok,"cTok")==0) {if(iscsym(nTok)&&!isdigit(nTok)&&(nTok!='_')) {getbuf(iscsym(peek())); break;} else tok=0;}
        else if(strcmp(tok,"unicodeTok")==0) {if(isTagStart(nTok)) {getbuf(iscsymOrUni(peek())); break;} else tok=0;}
        else if(strcmp(tok,"<abc>")==0) {if(chkStr("<")) {getbuf((peek()!='>')); chkStr(">"); break;} else tok=0;}
        else if(strcmp(tok,"123")==0) {if(isdigit(nTok)) {getbuf((isdigit(peek())||peek()=='.')); break;} else tok=0;}
        else if(strcmp(tok,"0x#")==0) {if(isxdigit(nTok)) {getbuf((isxdigit(peek())||peek()=='.')); break;} else tok=0;}
        else if(strcmp(tok,"0b#")==0) {if(isBinDigit(nTok)) {getbuf((isBinDigit(peek())||peek()=='.')); break;} else tok=0;}
        else if (chkStr(tok) || chkStr(altTok(tok))) break; else tok=0;
    }
    va_end(ap);
    return tok;
}
#define nxtTok(tok) nxtTokN(1,tok)

bool IsHardFunc(std::string tag);
void chk4HardFunc(infon* i){
    if((i->wFlag&mFindMode)==iTagUse && IsHardFunc(i->type->tag)){i->wFlag&=~iTagUse; i->wFlag|=iHardFunc;}
}

mpz_class* sizeFromExp(int exp){
    std::string size="1";
    size.append(exp, '0');
    mpz_class *Z=new mpz_class(size);
    return Z;
}

void numberFromString(char* buf, pureInfon* pInf, int base=10){
    // pInf should be empty when calling this to load it.
    char *pch=strchr(buf,'.');
    if(pch){
        pInf->flags|=fFloat;
        (*pch)=(char)0;
        // Make a listHead ptr. 1st node=
        infDataPtr intPart(new infonData(buf));
        infDataPtr fracPart(new infonData(pch+1));
        int eos=strlen(pch+1);
        infDataPtr intSize(new infonData(*sizeFromExp(pch-buf)));
        infDataPtr fracSize1(new infonData(*sizeFromExp(eos)));
        infDataPtr fracSize2(new infonData(*fracSize1));
 /*        pInf->listHead=new infon(0, 0, (infon*)new pureInfon(intSize),
                     new infon(tNum+fLiteral+((tNum+fLiteral)<<goSize), isTop+isFirst, (infon*)intSize, (infon*)intPart, 0,0,0,
                         new infon(tNum+fLiteral+((tNum+fLiteral)<<goSize), 0, (infon*)fracSize1, (infon*)fracPart, 0,0,0,
                             new infon(tNum+fLiteral+((tNum+fLiteral)<<goSize), isBottom+isLast, (infon*)fracSize2, (infon*)new pureInfon(0))
                        )
                     )
                 );*/
    }else {pInf->flags|=fLiteral; pInf->dataHead=infDataPtr(new infonData(buf,base)); }
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
UInt QParser::ReadPureInfon(pureInfon* pInf, UInt* flags, UInt *wFlag, infon** s2){
    UInt p=0, size=0, stay=1; char rchr, tok; infon *head=0, *prev; infon* j;
    infDataPtr* i=&(pInf)->dataHead;
    if(nxtTok("(") || nxtTok("{") || nxtTok("[")){
        if(nTok=='(') {rchr=')'; SetBits(*flags, mFormat+tType,(fConcat+tNum));}
        else if(nTok=='['){rchr=']'; *flags|=(tList+fLiteral); *wFlag=iGetLast;}
        else {rchr='}'; *flags|=(tList+fLiteral);}
        RmvWSC(); int foundRet=0; int foundBar=0;
        for(tok=peek(); tok != rchr && stay; tok=peek()){
            if(tok=='&') { // This is a definition, not an element
                streamGet();
                Tag* tag=0; Tag* prevTag=0; bool done=false;
                do{ // Here we process tag definitions
                tag=new Tag; tag->definition=(infon*)prevTag; prevTag=tag;
                if (!nxtTok("unicodeTok")) throw "Expected a tag after '&'";
                tag->tag=buf; tag->locale=locale.getBaseName();
                tagNormer->normalize(UnicodeString::fromUTF8(tag->tag), err).toUTF8String(tag->norm);
                if(tagIsBad(tag->norm, tag->locale.c_str())) throw "Illegal tag being defined";
                if(nxtTok(":")){if(nxtTok("cTok")) {icu::Locale tmp; tag->locale=(tmp.createCanonical(buf)).getBaseName();}}
                if(nxtTok(":")){if(nxtTok("<abc>")) tag->pronunciation=buf;}
                if(nxtTok("=")){done=true;}
                else if(!nxtTok("&")) throw "Expected &tag or tag locale, pronunciation or definition";
                } while (!done);
                infon* definition=ReadInfon();
                for(Tag* t=tag; t; t=prevTag){
                    prevTag=(Tag*)t->definition; t->definition=definition;
                    std::map<Tag,infon*>::iterator tagPtr=tag2Ptr.find(*t);
                    if (tagPtr==tag2Ptr.end()) {tag2Ptr[*t]=definition; ptr2Tag[definition]=*t;}
                    else{throw("A tag is being redefined, which isn't allowed");}
                }
                continue;
            }

            // OK, process an element or description of this list
            if(tok=='<') {foundRet=1; streamGet(); j=ReadInfon();}
            else if(nxtTok("...")){
                pureInfon pSize(size+1);
                j=new infon(iNone+isVirtual, &pSize); SetValueFormat(j, fUnknown); j->pos=(size+1); stay=0;
            } else j=ReadInfon();
            if(++size==1){
                if(!foundRet && !foundBar && stay && nxtTok("|")){
                    *s2=j; *flags|=fLoop; size=0; foundBar=1; RmvWSC(); continue;
                }
                pInf->listHead=j; head=prev=j; head->wFlag|=isFirst+isTop;
                if(FormatIsConcat(*flags)) *flags|=(InfsType(j));
            }
            if (foundRet==1) {check('>'); pInf->listHead=j; foundRet++; /* *pFlag|=intersect; */}  // TODO: when repairing Middle-Indexing, repair 'intersect'
            j->top=head; j->next=head; prev->next=j; j->prev=prev; head->prev=j; prev=j; RmvWSC();
        }
        if(head){
            head->prev->wFlag|=isBottom;
            if (!(VsFlag(head)&fIncomplete) && stay) head->prev->wFlag|=isLast;
        }
        check(rchr);
        if(nxtTok("~"))  (*wFlag)|=mAssoc;
    } else if (nxtTok("0X") || nxtTok("0x")) { // read hex number
        if( nxtTok("0x#")){ size=1; numberFromString(buf, pInf, 16); *flags|=((tNum+dHex)|pInf->flags); } else throw "Hex number expected after '0x'";
    } else if (nxtTok("0B") || nxtTok("0b")) { // read binary number
        if( nxtTok("0b#")){ size=1; numberFromString(buf, pInf, 2); *flags|=((tNum+dBinary)|pInf->flags);} else throw "Binary number expected after '0b'";
    } else if (nxtTok("123")) { size=1; *flags|=(tNum+dDecimal+(strchr(buf,'.')?fFloat:fLiteral)); *i=infDataPtr(new infonData(buf,10)); // read decimal number
    } else if (nxtTok("_")){*flags|=fUnknown+tNum;
    } else if (nxtTok("$")){*flags|=fUnknown+tString;
    } else if (nxtTok("\"") || nxtTok("'")){ // read string
        getbuf(peek()!=nTok); streamGet();
        size=p; *flags+=(tString+fLiteral);
        *i=(FormatIsUnknown(*flags))? 0 :infDataPtr(new infonData(buf));
    } else {strcpy(errMsg, "'X' was not expected"); errMsg[1]=nTok; throw errMsg;}
    pInf->flags=*flags;
    return size;
}

infon* QParser::ReadInfon(int noIDs){
    char op=0; UInt size=0; pureInfon iSize, iVal; infon *s1=0,*s2=0; UInt wFlag=0,fs=0,fv=0;
    Tag* tags=0; const char* cTok, *eTok, *cTok2; /*int textEnd=0; int textStart=textParsed.size();*/
    if(nxtTok("@")){wFlag|=toExec;}
    if(nxtTok("#")){wFlag|=asDesc;}
    if(nxtTok("?")){iSize.flags=fUnknown; iVal.flags=fUnknown; wFlag=iNone;}
    else if(nxtTok("unicodeTok")){
        wFlag|=iTagUse; tags=new Tag;
        tags->tag=buf; tags->locale=locale.getBaseName();
        tagNormer->normalize(UnicodeString::fromUTF8(tags->tag), err).toUTF8String(tags->norm);
        if(tagIsBad(tags->norm, locale.getBaseName())) throw "Illegal tag being used";
    }else if( nxtTok("%")){ // TODO: Don't allow these outside of := or :
        iVal.flags|=fUnknown;
        if (nxtTok("cTok")){
            if      (strcmp(buf,"W")==0){wFlag|=iToWorld;}
            else if (strcmp(buf,"C")==0){wFlag|=iToCtxt;}
            else if (strcmp(buf,"A")==0){wFlag|=iToArgs;}
            else if (strcmp(buf,"V")==0){wFlag|=iToVars;}
        } else if (nTok=='\\' || nTok=='^'){
            for(s1=0; (nTok=streamGet())=='\\';) {s1=(infon*)((UInt)s1+1); ChkNEOF;}
            if (nTok=='^') wFlag|=iToPath; else {wFlag|=iToPathH; streamPut(1);}
        }
    }else{  // OK, then we're parsing some form of *... +...
        wFlag|=iNone;
        if(nxtTok("+") || nxtTok("-")) op='+'; else if(nxtTok("*") || nxtTok("/")) op='*';
        if( nTok=='-' || nTok=='/') fs|=fInvert;
        size=ReadPureInfon(&iSize, &fs, &wFlag, &s2);
        if(op=='+'){
            iVal=iSize; iSize=pureInfon(size); fv=iVal.flags&(mFormat+tType); // use identity term '1'
            if (size==0 && (fv==(fUnknown+tNum) || fv==(fUnknown+tString))) {iSize.flags=fUnknown+tNum;}
            else if(((fv&tType)==tList) && iVal.listHead && InfIsVirtual(iVal.listHead->prev)) {iSize.flags=fUnknown+tNum;}
        }else if(op=='*'){
            if((fs&tType)==tString || (fs&tType)==tList) throw("Terms cannot be strings or lists");
            if(nxtTok("+")) op='+'; else if(nxtTok("-")) {op='+'; fv|=fInvert;}
            if (op=='+'){size=ReadPureInfon(&iVal,&fv,&wFlag,&s2);}
            else {iVal=0; fv=tNum+fLiteral;}    // use identity summand '0'
        } else { // no operator
            if(nxtTok(";")){iVal=0; SetBits(iVal.flags, (mFormat), fUnknown)}
            else {
                nxtTok(",");
                iVal=iSize; iSize=pureInfon(size); fv=iVal.flags&(mFormat+tType);
                if (size==0 && (fv==(fUnknown+tNum) || fv==(fUnknown+tString))) iSize.flags=fUnknown+tNum; // set size's flags for _ and $
                else if(((fv&tType)==tList) && iVal.listHead && InfIsVirtual(iVal.listHead->prev)) {iSize.flags=fUnknown+tNum;} // set size's flags for {...}
            }
        }
    }
    infon* i=new infon(wFlag, &iSize,&iVal,0,s1,s2); i->wSize=size; i->type=tags;
    if(ValueIsConcat(i) && (*i->size.dataHead)==1){infon* ret=i->value.listHead; delete(i); ret->top=ret->next=ret->prev=0; return ret;} // BUT we lose i's idents and some flags (desc, ...)
    if (i->size.listHead) i->size.listHead->top=i;
    if (i->value.listHead)i->value.listHead->top=i;
    if ((i->wFlag&mFindMode)==iGetLast){i->wFlag&=~(mFindMode+mAssoc); i->wFlag|=mIsHeadOfGetLast; i=new infon(wFlag,0,0,0,i); i->spec1->top2=i;}
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
            if(isEq(eTok,"==")) {code|=mLooseType;}
            if(isEq(cTok2,":")){toRef=grok(R,c1Right,&code);}
            else if(isEq(cTok2,"::")){toRef=grok(R,c2Right,&code);}
            else toRef=R;
        }
        if(toSet==0) throw ":= operator requires [....] on the left side";
        if(toRef==0) throw "=: operator requires [....] on the right side";
        if((toRef->type==0) && !(code&mLooseType)) toRef->type=toSet->type;
        insertID(&toSet->wrkList, toRef, code);
    }
    if(!(noIDs&2)){  // Load function "calls"
        if(nxtTok(":>" )) {infon* j=ReadInfon(1); j->wFlag|=sUseAsFirst; j->spec2=i; i=j; chk4HardFunc(i);}
        else if(nxtTok("<:")) {i->wFlag|=sUseAsFirst;
            i->spec2=ReadInfon(1);  chk4HardFunc(i);}
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
