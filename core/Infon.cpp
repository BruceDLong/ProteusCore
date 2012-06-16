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

void unRefInfon(infon* i){};
void unRefInfData(infonData* i){};

infonData::infonData(char* str):mpq_class(0), refCnt(1){
    int strSize=strlen(str)+1;
    mpz_ptr numerator=get_num_mpz_t();
    numerator->_mp_size = numerator->_mp_alloc = ((strSize==0)?1:(strSize-1)/blockSize+1);
    free(numerator->_mp_d);
    numerator->_mp_d=(mp_limb_t*)malloc(numerator->_mp_alloc * blockSize);
    memcpy(numerator->_mp_d,str,strSize);
};

pureInfon::operator string() {
    if((flagsZ&(tType+mFormat))==(tNum+fLiteral)) return dataHead->get_str();
    else return string((char*)dataHead->get_num_mpz_t()->_mp_d);
}

void pureInfon::setValUI(const UInt &num){
    offset=0;
    if(PureIsInListMode(*this)) unRefInfon(listHead);
    else unRefInfData(dataHead);
    dataHead=new infonData(num);
    flagsZ = tNum+fLiteral;
}

int infCmp(infon* A, infon* B){
    if(!infonSizeCmp(A,B)) return -2;

    // By here, types match, sizes are equal, literal numbers.
    UInt A_flag=A->value.flagsZ&(tType+mFormat), B_flag=B->value.flagsZ&(tType+mFormat);
    if(A_flag != (tNum+fLiteral) || A_flag != (tString+fLiteral)) return -2;
    if(B_flag != (tNum+fLiteral) || B_flag != (tString+fLiteral)) return -2;

    if(A_flag==(tNum+fLiteral) && B_flag==(tNum+fLiteral)) return cmp(*A->value.dataHead, *B->value.dataHead);
    if(A_flag==(tString+fLiteral) && B_flag==(tString+fLiteral))
        return memcmp(
            ((char*)(A->value.dataHead->get_num_mpz_t()->_mp_d))+A->value.offset.get_num().get_ui(),
            ((char*)(B->value.dataHead->get_num_mpz_t()->_mp_d))+B->value.offset.get_num().get_ui(),
            A->getSize().get_ui());
    throw "TODO: in infCmp(), handle mixed infon types (string/num)";
}

pureInfon::pureInfon(char* str, int base){
    if (base==-1) { // str is a string
        flagsZ|=tString+fLiteral;
        dataHead=new infonData(str);
    } else { // str is a number
        char *pch=strchr(str,'.');
        if (pch=='\0') {flagsZ=tNum+fLiteral; dataHead=new infonData(str,base);}
        else{
            (*pch)='\0';
            flagsZ=tNum+fFloat;
        }
    }
}

mpz_class& infon::getSize(){
    if(size.offset!=0) throw "Size needs updateing. Account for offset.";
    if((size.flagsZ&(tNum+fLiteral))!=(tNum+fLiteral)) throw "Size needs updating. Account for non-literal sizes.";
    if(size.dataHead->get_den().get_ui()!=1) throw "Size needs updating. Account for fractional sizes.";
    return size.dataHead->get_num();
}
