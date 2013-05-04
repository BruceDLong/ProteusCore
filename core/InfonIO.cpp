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
string agent::printPure (pureInfon* i, UInt wSize, infon* CI){
    string s, tmp; UInt f=i->flags; infon *p;
    if (((f&(fIncomplete+mFormat))==(fIncomplete+fUnknown)) && ((f&mType)!=tNum)) s+="?";
    UInt type=f&mType; bool doAsList=false;
    if (type==tNum){
        if (FormatIsUnknown(f)) s+="_";
        else if(FormatIs(f,fFloat)){s.append(i->listHead->value.dataHead->get_str()); s+="."; s.append(i->listHead->next->value.dataHead->get_num().get_str());}
        else if(FormatIs(f,fLiteral)){s.append(i->dataHead->get_str());}
        else doAsList=true;
    } else if(type==tString){
        if (FormatIsUnknown(f)) s+="$";
        else if(try2CatStr(&tmp, i, wSize)) {s+="\""; s+=tmp; s+="\"";}
        else doAsList=true;
    }
    if(type==tList || doAsList){
        s+=(FormatIsConcat(f) || (f&fEmbedSeq))?"(":"{";
        if(1 /*or mode == NormForm*/){
            if(i->listHead){
                for(int result=StartPureTerm(i, &p); result==0; result=getNextTerm(&p)){
                    if(InfIsTentative(p)) {s+="..."; break;}
                    else {s+=printInfon(p, CI); s+=' ';}
                }
            }

        } else{ // print lists to show structure
            for(infon* p=i->listHead;p;) {
               // if(p==i->listHead && f&fLoop && ((infon*)i)->spec2){printInfon(((infon*)i)->spec2,CI); s+=" | ";} // TODO: when overhauling printing, use this.
                if(InfIsTentative(p)) {s+="..."; break;}
                else {s+=printInfon(p, CI); s+=' ';}
                if (InfIsBottom(p)) p=0; p=p->next;
            }
        }
        s+=(FormatIsConcat(f) || (f&fEmbedSeq))?")":"}";
    }
    return s;
}

string agent::printInfon(infon* i, infon* CI){
    string s; //Indent;
    if(i==0) {s+="null"; return s;}
    if(i==CI) s+="<font color=green>";
    UInt mode=i->wFlag&mFindMode;
//    if(InfToExec(i)) s+="@";
    if(InfAsDesc(i)) s+="#";
    if(i->wFlag&xDevToHome) s+="\\";
    if(i->wFlag & asNot) s+="!";
    if (mode==iTagUse) {
        s+=i->type->norm; s+=" ";
    } else if(InfIsNormed(i) || mode==iNone /* || VsFlag(i)&notParent */){
        if (SizeIsUnknown(i)) {s+=printPure(&i->value, i->wSize, CI);}
        else{
            if(InfsType(i)==tNum) {s+=(SsFlag(i)&fInvert)?"/":"*"; s+=printPure(&i->size, 0,CI);}
            if(InfsType(i)==tNum) s+=(VsFlag(i)&fInvert)?"-":"+";
            s+=printPure(&i->value, i->getSize().get_ui(), CI);
        }
    } else {
        if (!InfIsNormed(i)) {
            if(mode==iAssocNxt) s+="%~ ";
            else if(mode==iToWorld) s+="%W ";
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
        do {k=k->next; s+="="; s+=printInfon(k->item,CI);} while(k!=i->wrkList);
    }
    if (i==CI) s+="</font>";
    return s;
}

#define streamPut(nChars) {for(int n=nChars; n>0; --n){stream.putback(textParsed[textParsed.size()-1]); textParsed.resize(textParsed.size()-1);}}
#define ChkNEOF {if(stream.eof() || stream.fail()) throw "Unexpected End of file";}
#define getbuf(c) {ChkNEOF; for(p=0;(c);buf[p++]=streamGet()){if (p>=bufmax) throw "String Overflow";} buf[p]=0;}
#define check(ch) {RmvWSC(); ChkNEOF; tok=streamGet(); if(tok != ch) {cout<<"Expected "<<ch<<"\n"; throw "Unexpected character";}}

char QParser::streamGet(){char ch=stream.get(); textParsed+=ch; return ch;}
char QParser::peek(){if (stream.fail()) throw "Unexpected end of file";  return stream.peek();}
char QParser::Peek(){RmvWSC(); return peek();}

void QParser::scanPast(char* str){
    char p; char* ch=str;
    while(*ch!='\0'){
        p=streamGet();
        if (stream.eof() || stream.fail())
            throw (string("Expected String not found before end-of-file: '")+string(str)+"'").c_str();
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
const UnicodeSet TagChars(UnicodeString("[[:XID_Continue:]']"), err);
const UnicodeSet TagStarts(UnicodeString("[:XID_Start:]"), err);
bool iscsymOrUni (char nTok) {return (iscsym(nTok) || (nTok&0x80));}
bool isTagStart(char nTok) {return (iscsymOrUni(nTok)&&!isdigit(nTok)&&(nTok!='_'));}
bool isAscStart(char nTok) {return (iscsym(nTok)&&!isdigit(nTok));}
const icu::Normalizer2 *tagNormer=Normalizer2::getNFKCCasefoldInstance(err);

bool tagIsBad(string tag, const char* locale) {
    UErrorCode err=U_ZERO_ERROR;
    if(!TagStarts.contains(tag.c_str()[0])) return 1; // First tag character is invalid
    if((size_t)TagChars.spanUTF8(tag.c_str(), -1, USET_SPAN_SIMPLE) != tag.length()) return 1;
    USpoofChecker *sc = uspoof_open(&err);
    uspoof_setChecks(sc, USPOOF_SINGLE_SCRIPT|USPOOF_INVISIBLE, &err);
    uspoof_setAllowedLocales(sc, locale, &err);
    int result=uspoof_checkUTF8(sc, tag.c_str(), -1, 0, &err);
    return (result!=0);
}

const char* altTok(string tok){
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

xlater* QParser::chkLocale(icu::Locale* locale){
    int p, startPos=textParsed.size();
    getbuf(isAscStart(peek()));
    string localeID=buf;
    LanguageExtentions::iterator lang = langExtentions.find(localeID);
    if (lang != langExtentions.end()){
        if(chkStr(":")){
            *locale=icu::Locale::createCanonical(localeID.c_str());
            return lang->second;
        }
    }
    streamPut(textParsed.size()-startPos);
    lang = langExtentions.find(locale->getLanguage());
    if (lang != langExtentions.end()){return lang->second;}
    return 0;
}

WordSPtr QParser::ReadTagChain(icu::Locale* locale, xlater **XL_return, string scopeID){
    icu::Locale tmpLocale= *locale;
    xlater* Xlater = chkLocale(&tmpLocale);
    if(Xlater==0) throw "Words read with unsupported or no locale";
    if(XL_return) (*XL_return)=Xlater;
    WordSPtr result=WordSPtr(new WordS);
    Xlater->ReadTagChain(this, tmpLocale, *result);
    result->key=result->norm + "%" + scopeID;
    result->xLater=Xlater;
    return result;
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

bool IsHardFunc(string tag);
void chk4HardFunc(infon* i){
    if((i->wFlag&mFindMode)==iTagUse && IsHardFunc(i->type->norm)){i->wFlag&=~iTagUse; i->wFlag|=iHardFunc;}
}

string sizeFromExp(int exp){
    string size="1";
    size.append(exp, '0');
    return size;
}

void numberFromString(char* buf, pureInfon* pInf, int base){
    // pInf should be empty when calling this to load it.
    char *pch=strchr(buf,'.');
    if(pch){
        pInf->flags|=tNum+fFloat;
        (*pch)=(char)0;
        infDataPtr intPart(new infonData(buf,base));                                    pureInfon pIntPart (intPart,  tNum+fLiteral, 0);
        infDataPtr intSize(new infonData(sizeFromExp(pch-buf).c_str(),base));           pureInfon pIntSize (intSize,  tNum+fLiteral, 0);
        infDataPtr fracSize(new infonData(sizeFromExp(strlen(pch+1)).c_str(),base));    pureInfon pFracSize(fracSize, tNum+fLiteral, 0);
        BigFrac frac(BigInt(pch+1), fracSize->get_num() );
        infDataPtr fraction(new infonData(frac));                                       pureInfon pFraction(fraction, tNum+fLiteral, 0);

        infon*  intInf=new infon(isTop+isFirst,   &pIntSize, &pIntPart);
        infon* fracInf=new infon(isBottom+isLast, &pFracSize, &pFraction);
        pInf->listHead=intInf; intInf->next=intInf->prev=fracInf; fracInf->next=fracInf->prev=intInf;
    }else {pInf->flags|=fLiteral; pInf->dataHead=infDataPtr(new infonData(buf,base)); }
}


infon* grok(infon* item, UInt tagCode, int* code){
    if((item->wFlag&mFindMode)==iTagUse) {(*code)|=tagCode; return item;}
    else {
        if (tagCode==c1Left || tagCode==c1Right) {
            if((item->wFlag&mFindMode)>=iGetLast) {return item->spec1;}
        } else if (tagCode==c2Left || tagCode==c2Right) {
            if((item->wFlag&mFindMode)>=iGetLast) {
                // if (item->spec1->spec2 a tag)  mark inner-tag; return it.
                if ((item->spec1->spec2->wFlag&mFindMode)>=iGetLast) return item->spec1->spec2->spec1;
            } else{ // { [A V R] | ...} ::= ABC
                // if (item->spec2 a tag)  mark inner-tag; return it.
                if ((item->spec2->wFlag&mFindMode)>=iGetLast) return item;
            }
        }
    }
    return 0;
}

char errMsg[100];
UInt QParser::ReadPureInfon(pureInfon* pInf, UInt* flags, UInt *wFlag, infon** s2, string &scopeID){
    UInt p=0, size=0, stay=1; char rchr, tok, seperator='\0'; const char* pTok; infon *head=0, *prev; infon* j;
    infDataPtr* i=&(pInf)->dataHead;
    if(nxtTok("(") || nxtTok("{") || nxtTok("[") || nxtTok("<")){
        if(nTok=='(') {rchr=')'; seperator='?'; SetBits(*flags, mFormat+mType,(fConcat+tNum));}
        else if(nTok=='['){rchr=']'; *flags|=(tList+fLiteral); *wFlag|=iGetLast;}
        else if(nTok=='<'){rchr='>'; *flags|=(tList+fLiteral); *wFlag|=(iGetLast+xOptmize1);} // Later: iGetMiddle
        else {rchr='}'; *flags|=(tList+fLiteral);}
        RmvWSC(); int foundRet=0; int foundBar=0;
        for(tok=peek(); tok != rchr && stay; tok=peek()){
            if(tok=='&') { // This is a definition, not an element
                if(rchr=='>') throw "Definitions (&) are not allowed inside <TYPE>";
                streamGet();
                xlater *Xlater;
                attrStorePtr attrs=0; // For model attributes
                list<WordSPtr> tagList;
                WordSPtr tag=0; bool done=false;
                do{ // Here we process tag definitions
                    tag=ReadTagChain(&agnt->locale, &Xlater, scopeID); if (tag==0) throw "Null tag was read";
                    tagList.push_back(tag);
//cout<<"TAG:"<<tag->norm<<"\n";
                    while((pTok = nxtTokN(2,"%","#"))){ // read tag's (%) or definition's (#) attributes
                        nxtTok("cTok");
                        string attrTag=buf;
//cout<<"    AttrTag:"<<attrTag<<"\n";
                        chkStr(":");
                        if(*pTok=='%' && attrTag=="say") nxtTok("<abc>");
                        else getbuf((peek()!='%' && peek()!='#' && peek()!='&' && peek()!='='));
                        string attrText=buf;
                        if(*pTok=='%') tag->attributes.insert(pair<string,string>(attrTag,attrText));
                        else if(*pTok=='#'){
                            if(attrs==0) attrs=new attrStore;
                            attrs->insert(pair<string,string>(attrTag,attrText));
                        }
                    }
                    if(nxtTok("=")){done=true;}
                    else if(!nxtTok("&")) throw "Expected &tag or tag locale";
                } while (!done);
                string scopeTag=scopeID + (string)"&"+tag->norm;
                infon* definition=ReadInfon(scopeTag);
                definition->attrs=attrs;
                WordLibrary *wordLib=Xlater->wordLibrary;
                for(list<WordSPtr>::iterator t=tagList.begin(); t!=tagList.end(); ++t){
                    (*t)->definition=definition;
                    WordSPtr tagPtr=wordLib->chkExists((*t)->key, (*t)->senseID); //find((*t)->key);
//if (tagPtr!=wordLib->end()) cout<<"DEFINING:"<< (*t)->key <<"::"<<(*t)->senseID <<" found: " << tagPtr->second->senseID <<"\n";
                    if (!tagPtr){//==wordLib->end() || (tagPtr->second->senseID != (*t)->senseID)) {
                        wordLib->insert(pair<wordKey, WordSPtr>((*t)->key, (*t)));
                        DefPtr2Tag.insert(pair<infon*,WordSPtr>(definition,(*t)));
                    }else{cout<<"REDEFINED: "<<tagPtr->norm<<"\n"; throw("A word/sense is being redefined, which isn't allowed");}
                }
                continue;
            }

            // OK, process an element or description of this list
            if(tok=='<' && rchr==']') {foundRet=1; streamGet(); j=ReadInfon(scopeID);}
            else if(nxtTok("...")){
                pureInfon pSize(size+1);
                j=new infon(iNone+isVirtual, &pSize); SetValueFormat(j, fUnknown); j->pos=(size+1); stay=0;
            } else j=ReadInfon(scopeID);
            if(++size==1){
                if(!foundRet && !foundBar && stay && nxtTok("|")){
                    *s2=j; *flags|=fLoop; size=0; foundBar=1; RmvWSC(); continue;
                }
                pInf->listHead=j; head=prev=j;
                if(FormatIsConcat(*flags) && (InfsType(j)!=tUnknown)) {SetBits((*flags), mType, InfsType(j));}
            }
            if (foundRet==1) {
                check('>'); pInf->listHead=j; foundRet++;
                if(peek()==':') throw "'<TYPE>' syntax was used in non-working context.";
                /* *pFlag|=intersect; */  // TODO: when repairing Middle-Indexing, repair 'intersect'
            }
            if(j->type){}//cout<<"TYPE"<<j->type->norm;}
            j->top=head; j->next=head; prev->next=j; j->prev=prev; head->prev=j; prev=j; RmvWSC();

            char sepIsBoundry=(nxtTok(","))?',':' ';
            if(seperator){ // In (...) ensure items are seperated by either ',' or ' ', not a mixture.
                if(seperator=='?' || (*flags&fLoop)) {
                    seperator=sepIsBoundry;
                    if(seperator==',' || (*flags&fLoop)){SetBits(*flags, fEmbedSeq+mFormat+mType,(fEmbedSeq+fConcat+tList));}
                }else {if(seperator != sepIsBoundry && nTok!=rchr) throw "All seperators in (...) must be the same; either ',' or nothing.";}
            }
        }
        if(head){
            if(rchr=='>'){ // Create negative copy of type-to-find. <type> == [{~type|...} type]
                // NOTE: currently, negCpy is not exactly correctly constructed as we're waiting for the internal repetition syntax type|...
                if(size!=1) throw "<TYPE> must have exctly one element";
                infon* negCpy=new infon; agnt->deepCopy(head->prev, negCpy); negCpy->wFlag|=asNot; // Copy the last item.
                negCpy->next=head; negCpy->prev=head->prev; negCpy->top=head->top;
                head->prev->top = head->prev->next = negCpy;
                head->prev = head->top = negCpy;
                pInf->listHead = head = negCpy;
                size++;
            }
            head->prev->wFlag|=isBottom; head->wFlag|=isFirst+isTop;
            if (!(VsFlag(head)&fIncomplete) && stay) head->prev->wFlag|=isLast;
        }
        check(rchr);
        if(nxtTok("~"))  {(*wFlag)|=mAssoc;}
    } else if (nxtTok("0X") || nxtTok("0x")) { // read hex number
        if( nxtTok("0x#")){ size=1; numberFromString(buf, pInf, 16); *flags|=((tNum+dHex)|pInf->flags); } else throw "Hex number expected after '0x'";
    } else if (nxtTok("0B") || nxtTok("0b")) { // read binary number
        if( nxtTok("0b#")){ size=1; numberFromString(buf, pInf, 2); *flags|=((tNum+dBinary)|pInf->flags);} else throw "Binary number expected after '0b'";
    } else if (nxtTok("123")) { size=1; numberFromString(buf, pInf, 10); *flags|=((tNum+dDecimal)|pInf->flags); // read decimal number
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

infon* QParser::ReadInfon(string &scopeID, int noIDs){
    char op=0; UInt size=0; pureInfon iSize, iVal; infon *s1=0,*s2=0; UInt wFlag=0,fs=0,fv=0;
    WordSPtr tags=0; const char* cTok, *eTok, *cTok2; /*int textEnd=0; int textStart=textParsed.size();*/
    if(nxtTok("@")){wFlag|=toExec;}
    if(nxtTok("#")){wFlag|=asDesc;}
    if(nxtTok("\\")){wFlag|=xDevToHome;}
    if(nxtTok("!")){wFlag|=asNot;}
    if(nxtTok("?")){iSize.flags=fUnknown; iVal.flags=fUnknown; wFlag=iNone;}
    else if( nxtTok("%")){
        iVal.flags|=fUnknown;
        if (nxtTok("123")){
            wFlag|=iTagUse; if(!(tags=ReadTagChain(&agnt->locale, 0, scopeID))) throw "Null Words found after %123. Shouldn't happen";
        }else if (nxtTok("cTok")){
            if      (strcmp(buf,"W")==0){wFlag|=iToWorld;}
            else if (strcmp(buf,"C")==0){wFlag|=iToCtxt;}
            else if (strcmp(buf,"A")==0){wFlag|=iToArgs;}
            else if (strcmp(buf,"V")==0){wFlag|=iToVars;}
           // else if (strcmp(buf,"P")==0){wFlag|=iToPrev;}
           // else if (strcmp(buf,"L")==0){wFlag|=iToLastOfPrev;}
        } else if (nTok=='\\' || nTok=='^'){  // TODO: Don't allow these (or %A or %V) outside of := or :
            for(s1=0; (nTok=streamGet())=='\\';) {s1=(infon*)((UInt)s1+1); ChkNEOF;}
            if (nTok=='^') wFlag|=iToPath; else {wFlag|=iToPathH; streamPut(1);}
        } else {s1=0; wFlag|=iToPathH;}
    }else if(isTagStart(Peek())){
        wFlag|=iTagUse; if(!(tags=ReadTagChain(&agnt->locale, 0, scopeID))) throw "Null Words found. Shouldn't happen";
        nxtTok(",");
    }else{  // OK, then we're parsing some form of *... +...
        wFlag|=iNone;
        if(nxtTok("+") || nxtTok("-")) op='+'; else if(nxtTok("*") || nxtTok("/")) op='*';
        if( nTok=='-' || nTok=='/') fs|=fInvert;
        size=ReadPureInfon(&iSize, &fs, &wFlag, &s2, scopeID);
        if(op=='+'){
            iVal=iSize; iSize=pureInfon(size); fv=iVal.flags&(mFormat+mType); // Use identity term '1'
            if (size==0 && (fv==(fUnknown+tNum) || fv==(fUnknown+tString))) {iSize.flags=fUnknown+tNum;}
            else if(((fv&mType)==tList) && iVal.listHead && InfIsVirtual(iVal.listHead->prev)) {iSize.flags=fUnknown+tNum;}
        }else if(op=='*'){
            if((fs&mType)==tString || (fs&mType)==tList) throw("Terms cannot be strings or lists");
            if(nxtTok("+")) op='+'; else if(nxtTok("-")) {op='+'; fv|=fInvert;}
            if (op=='+'){size=ReadPureInfon(&iVal,&fv,&wFlag,&s2, scopeID);}
            else {iVal=0; fv=tNum+fLiteral;}    // Use identity summand '0'
        } else { // No operator given
                iVal=iSize; iSize=pureInfon(size); fv=iVal.flags&(mFormat+mType);
                if (size==0 && (fv==(fUnknown+tNum) || fv==(fUnknown+tString))) iSize.flags=fUnknown+tNum; // Set size's flags for _ and $
                else if(((fv&mType)==tList) && iVal.listHead && InfIsVirtual(iVal.listHead->prev)) {iSize.flags=fUnknown+tNum;} // Set size's flags for {...}
        }
    }
    infon* i=new infon(wFlag, &iSize,&iVal,0,s1,s2,0); i->wSize=size; i->type=tags;
    if(ValueIsConcat(i) && ((*i->size.dataHead)==1) && !InfIsLoop(i)){infon* ret=i->value.listHead; delete(i); i=ret; i->wFlag&=~mListPos; i->top=i->next=i->prev=0;} // BUT we lose some flags (desc, ...)
    else {
        if (i->size.listHead) i->size.listHead->top=i;
        if (i->value.listHead){i->value.listHead->top=i; i->updateIndex();}
        if ((i->wFlag&mFindMode)==iGetLast){
            i->wFlag&=~(mFindMode+mAssoc+xOptmize1); i->wFlag|=mIsHeadOfGetLast;
            i=new infon(wFlag,0,0,0,i); i->spec1->top2=i;
            if(i->wFlag&xDevToHome){
                if(!((i->wFlag&mFindMode)>=iGetLast)){throw "'\' Must be followed by a selecting list like [...] or <...>";}
                copyTo(i->spec1->value.listHead->prev, i); i->wFlag|=(xOptmize1+xDevToHome);
                // TODO: i-<updateIndex()
            }
        }

    }

    for(char c=Peek(); !(noIDs&1) && (c==':' || c=='='); c=Peek()){
        infon *R, *toSet=0, *toRef=0; int idFlags=0;
        cTok=nxtTokN(2,"::",":");
        eTok=nxtTokN(2,"==","=");
        if(peek()=='!'){stream.get(); idFlags|=OverrideIdent;}
        if(isEq(cTok,":") && (eTok==0)){  // X:Y is about the same as Y := X.
            if(peek()=='>') {streamPut(1); break;}  // Oops, this is a function :> Parse it below.
            if(noIDs&8){streamPut(1); break;}
            toRef=i; i=ReadInfon(scopeID, 8); toSet=grok(i,c1Left,&idFlags);
            if((toRef->wFlag&mFindMode)!=iTagUse) toRef->top=i;
        } else {
            cTok2=nxtTokN(2,"::",":");
            if(isEq(cTok,":")){
                toSet=grok(i,c1Left,&idFlags); R=ReadInfon(scopeID, 4);
                if((i->wFlag&mFindMode)>=iGetLast) {
                    i->spec1->top=R;
                    if((noIDs&4)==0){ // Set 'top' for any \\^, etc.
                        infon *p, *q;
                        for(p=i; (p->wFlag&mFindMode)>=iGetLast; p=q){
                            q=p->spec1->top; q->top=i; p->spec1->top=0;
                        }
                    }
                }
            } else if(isEq(cTok,"::")){
                // Add a new head to the spec list.
                toSet=grok(i,c2Left,&idFlags); idFlags|=InitSearchList; R=ReadInfon(scopeID, 4);
            } else {toSet=i; R=ReadInfon(scopeID, 1);}
            if(isEq(eTok,"==")) {idFlags|=mLooseType;}
            if(isEq(cTok2,":")){toRef=grok(R,c1Right,&idFlags);}
            else if(isEq(cTok2,"::")){toRef=grok(R,c2Right,&idFlags);}
            else toRef=R;
        }
        if(toSet==0) throw ":= operator requires [....] on the left side";
        if(toRef==0) throw "=: operator requires [....] on the right side";
        if((toRef->type==0) && !(idFlags&mLooseType)) toRef->type=toSet->type;
        insertID(&toSet->wrkList, toRef, idFlags, toSet);
    }
    if(!(noIDs&2)){  // Load function "calls". In addOne<:5, i->spec2 is the 5.
        if(nxtTok(":>" )) {infon* j=ReadInfon(scopeID, 1); j->wFlag|=sUseAsFirst; j->spec2=i; i=j; chk4HardFunc(i);}
        else if(nxtTok("<:")) {i->wFlag|=sUseAsFirst; i->spec2=ReadInfon(scopeID, 1);  chk4HardFunc(i);}
        else if(nxtTok("!>")){}
        else if(nxtTok("<!")){}
    }
    /*textEnd=textParsed.size(); */
    return i;
}

infon* QParser::parse(){
    char tok;
    try{
        textParsed=""; string topScope="U";
        line=1; scanPast((char*)"<%");
        infon*i=ReadInfon(topScope, 0);
        //cout<<"\n["<<textParsed<<"]\n";
        check('%'); check('>'); buf[0]=0; return i;
    }
    catch(char const* err){char l[30]; itoa(line,l); strcpy(buf,"An Error Occured: "); strcat(buf,err);
        strcat(buf,". (line ");strcat(buf,l); strcat(buf,")\n");}
    catch(...){strcpy(buf,"An Unknown Error Occurred While Parsing.\n");};
    if(stream.fail()) cout << "End of File Reached";
    return 0;
}
