/////////////////////////////////////////////////////
// Proteus Infon Class and Related Classes  Copyright (c) 1997-2012 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Proteus.h"
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <inttypes.h>

using namespace std;

infon::infon(UInt wf, pureInfon* s, pureInfon* v, infNode*ID,infon*s1,infon*s2,infon*n,WordSMap* tagMap):
        wFlag(wf), wSize(0), next(n), pred(0), spec1(s1), spec2(s2), wrkList(ID), tag2Ptr(tagMap) {
    prev=0; top=0; top2=0; type=0; pos=0;
    if(s) size=*s;
    if(v) value=*v;
};

BigInt& infon::getSize(){
    if(size.offset!=0) throw "Size needs updateing. Account for offset.";
    if((size.flags&(tNum+fLiteral))!=(tNum+fLiteral)) {};//cout<< "Size needs updating. Account for non-literal sizes.\n\n";
    if(size.dataHead->get_den().get_ui()!=1) throw "Size needs updating. Account for fractional sizes.";
    return size.dataHead->get_num();
}

bool infon::getInt(BigInt* num) {
  if(!InfIsLiteralNum(this)) return 0;
  (*num)=*this->value.dataHead;
  if(value.flags & fInvert) *num =- *num;
  return 1;
 }

bool infon::getReal(double* d) {
    *d=0;
    UInt format=InfsFormat(this);
    if(InfsType(this)!=tNum || format==fUnknown || format==fConcat) return 0;
    if(format==fFract || format==fLiteral) *d=value.dataHead->get_d();
    else if(format==fFloat) *d=value.listHead->value.dataHead->get_d() + value.listHead->next->value.dataHead->get_d();
    if(value.flags & fInvert) *d =- *d;
    return 1;
}

bool try2CatStr(string* s, pureInfon* i, UInt wSize){  // returns true if s was set to the string of i.
    string tmp;
    if((i->flags&mType)!=tString) return 0;
    if((i->flags&mFormat)==fLiteral) {s->append(i->toString(wSize)); return true;}
    if(((i->flags&mFormat)!=fConcat) || InfIsTentative(i->listHead->prev)) return false;
    for(infon* p=i->listHead;p;) {
        if(!try2CatStr(&tmp, &p->value, p->getSize().get_ui())) return false;
        if (InfIsBottom(p)) p=0; else p=p->next;
    }
    s->append(tmp);
    return true;
}

bool infon::getStng(string* str) {
    *str="";
    return try2CatStr(str, &value, getSize().get_ui());
}

infon* infon::findTag(WordSPtr word){
    if(word->definition) return word->definition;
    infon* def=word->xLater->tags2Proteus(word);
    if(def) return def;
    return 0;
}

int infonSizeCmp(infon* left, infon* right) { // -1: L<R,  0: L=R, 1: L>R. Infons must have fLiteral, numeric sizes
    UInt leftType=InfsType(left), rightType=InfsType(right);
    if(leftType==0 || SizeIsUnknown(left) || SizeIsUnknown(right)) return 2; // Let's say '2' is an error.
    if(leftType==rightType) return cmp(left->getSize(), right->getSize());
    else {
        // HERE: if (Neither is a list) If (num/string) swap(left,right); compare string/num
        throw "TODO: make infonSizeCmp compare strings to numbers, etc.";
    }
}

int infValueCmp(infon* dest, infon* src){
    // dest is where the size-to-compare comes from.
    UInt dest_flag=dest->value.flags&(mType+mFormat), src_flag=src->value.flags&(mType+mFormat);
    if(dest_flag != (tNum+fLiteral) && dest_flag != (tString+fLiteral)) return 2;
    if(src_flag != (tNum+fLiteral) && src_flag != (tString+fLiteral)) return 2;

    if(dest_flag==(tNum+fLiteral) && src_flag==(tNum+fLiteral)) return cmp(*dest->value.dataHead, *src->value.dataHead); // TODO: Account for sizes
    if(dest_flag==(tString+fLiteral) && src_flag==(tString+fLiteral))
        return memcmp(
            ((char*)(dest->value.dataHead->get_num_mpz_t()->_mp_d))+dest->value.offset.get_num().get_ui(),
            ((char*)(src->value.dataHead->get_num_mpz_t()->_mp_d))+src->value.offset.get_num().get_ui(),
            dest->getSize().get_ui());
    throw "TODO: in infValueCmp(), handle mixed infon types (string/num)";
}

const int blockSize=sizeof(mp_limb_t);

infonData::infonData(const string str):mpq_class(0), refCnt(0){
    int strSize=str.length()+1;
    mpz_ptr numerator=get_num_mpz_t();
    numerator->_mp_size = numerator->_mp_alloc = ((strSize==0)?1:(strSize-1)/blockSize+1);
    free(numerator->_mp_d);
    numerator->_mp_d=(mp_limb_t*)malloc(numerator->_mp_alloc * blockSize);
    memcpy(numerator->_mp_d,str.c_str(),strSize);
};

infonData::infonData(const char* numStr, int base):mpq_class(numStr,base), refCnt(0){};

string pureInfon::toString(UInt size) {
    if((flags&(mType+mFormat))==(tNum+fLiteral)) return dataHead->get_str();
    else return string(((char*)dataHead->get_num_mpz_t()->_mp_d) + (UInt)offset.get_d(), size);
}

pureInfon& pureInfon::operator=(const pureInfon &pInf){
    if(this != &pInf){
        if(PureIsInListMode(*this)) recover(listHead);
        offset=pInf.offset;
        flags=pInf.flags;
        listHead=pInf.listHead;
        dataHead=pInf.dataHead;
    }
    return *this;
}

pureInfon& pureInfon::operator=(const BigFrac &num){
    if(PureIsInListMode(*this)) recover(listHead);
    offset=0;
    flags=tNum+fLiteral;
    listHead=0;
    dataHead=infDataPtr(new infonData(num));
    return *this;
}

pureInfon& pureInfon::operator=(const UInt &num){
    if(PureIsInListMode(*this)) recover(listHead);
    offset=0;
    flags=tNum+fLiteral;
    listHead=0;
    dataHead=infDataPtr(new infonData(num));
    return *this;
}

pureInfon& pureInfon::operator=(const string &str){
    if(PureIsInListMode(*this)) recover(listHead);
    offset=0;
    flags=tString+fLiteral;
    listHead=0;
    dataHead=infDataPtr(new infonData(str));
    return *this;
}

pureInfon::pureInfon(infDataPtr Head, UInt flag, BigFrac offSet):flags(flag), offset(offSet), dataHead(Head), listHead(0){};

pureInfon::pureInfon(char* str, int base):listHead(0) {
    if (base==-1) { // str is a string
        flags|=tString+fLiteral;
        dataHead=infDataPtr(new infonData(str));
    } else { // str is a number
        char *pch=strchr(str,'.');
        if (pch=='\0') {flags=tNum+fLiteral; dataHead=infDataPtr(new infonData(str,base));}
        else{
            (*pch)='\0';
            flags=tNum+fFloat;
        }
    }
}

pureInfon::~pureInfon(){if(PureIsInListMode(*this)) recover(listHead);}

WordS::~WordS(){if(sysType>=wstNumOrd && sysType<=wstNumInfonic) delete definition;};
