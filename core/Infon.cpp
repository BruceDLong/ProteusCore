/////////////////////////////////////////////////////
// Proteus Infon Class and Related Classes  Copyright (c) 1997-2012 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Proteus.h"
//#include <iostream>
//#include <string.h>
#include <stdlib.h>
//#include <math.h>
//#include <cstdio>

//#include "remiss.h"

//#include <unicode/putil.h>
//#include <unicode/uniset.h>
//#include <unicode/unistr.h>
//#include <unicode/normalizer2.h>

#include <stddef.h>
#include <ctype.h>
#include <inttypes.h>



using namespace std;

const int blockSize=sizeof(mp_limb_t);

infonData::infonData(char* str):mpq_class(0), refCnt(0){
    int strSize=strlen(str)+1;
    mpz_ptr numerator=get_num_mpz_t();
    numerator->_mp_size = numerator->_mp_alloc = ((strSize==0)?1:(strSize-1)/blockSize+1);
    free(numerator->_mp_d);
    numerator->_mp_d=(mp_limb_t*)malloc(numerator->_mp_alloc * blockSize);
    memcpy(numerator->_mp_d,str,strSize);
};

infonData::infonData(char* numStr, int base):mpq_class(numStr,base), refCnt(0){};

infon::infon(UInt pf, UInt wf, pureInfon* s, pureInfon* v, infNode*ID,infon*s1,infon*s2,infon*n):
        pFlag(pf), wFlag(wf), next(n), pred(0), spec1(s1), spec2(s2), wrkList(ID) {
    prev=0; top=0; top2=0; type=0; pos=0;
    if(s) size=*s;
    if(v) value=*v;
};

std::string pureInfon::toString(UInt size) {
    if((flags&(tType+mFormat))==(tNum+fLiteral)) return dataHead->get_str();
    else return string(((char*)dataHead->get_num_mpz_t()->_mp_d) + (UInt)offset.get_d(), size);
}

void pureInfon::setValUI(const UInt &num){
    offset=0;
    if(PureIsInListMode(*this)) recover(listHead);
    dataHead=infDataPtr(new infonData(num));
    flags = tNum+fLiteral;
    listHead=0;
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

int infValueCmp(infon* dest, infon* src){
    // dest is where the size-to-compare comes from.
    UInt dest_flag=dest->value.flags&(tType+mFormat), src_flag=src->value.flags&(tType+mFormat);
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

pureInfon::pureInfon(infDataPtr Head, UInt flag, mpq_class offSet):flags(flag), offset(offSet), dataHead(Head), listHead(0){};

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

mpz_class& infon::getSize(){
    if(size.offset!=0) throw "Size needs updateing. Account for offset.";
    if((size.flags&(tNum+fLiteral))!=(tNum+fLiteral)) cout<< "Size needs updating. Account for non-literal sizes.\n\n";
    if(size.dataHead->get_den().get_ui()!=1) throw "Size needs updating. Account for fractional sizes.";
    return size.dataHead->get_num();
}
