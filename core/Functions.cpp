/////////////////////////////////////////////////
// Functions.h  Copyright Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _Functions
#define _Functions

#include <strstream>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <math.h>
#include "Proteus.h"
#include <ctime>

#define pi 3.14159
#define Zto1(gInt) (float)(((float)gInt)  /* / 1024.0*/)

#define cpFlags(from, to, mask) {to->pFlag=(to->pFlag& ~(mask))+((from)->pFlag&mask); to->wFlag=from->wFlag;}
#define copyTo(from, to) {if(from!=to){to->size=(from)->size; to->value=(from)->value; cpFlags((from),to,0x00ffffff);}}

#define copyInfonString2charBuf(inf, buf) {memcpy(buf, (char*)inf->value, (UInt)inf->size); buf[(UInt)inf->size]=0;}
infon* Theme;

int getStrArg(infon* i, stng* str, agent* a){
    i->spec2->top=i;
    a->normalize(i->spec2);
    getStng(i->spec2, str);
    return 1;
}

void setString(infon* CI, stng *s){
    UInt tmpFlags=CI->pFlag&0xff000000;
    CI->size=(infon*) s->L;
    CI->value=(infon*)s->S;
    CI->pFlag=tmpFlags + (tUInt<<goSize)+tString;
    CI->wFlag=iNone;
}

int getIntArg(infon* i, int* Int1, agent* a){
    int sign;
    i->spec2->top=i;
    a->normalize(i->spec2);
    getInt(i->spec2, *Int1, sign);
    if(sign) *Int1 = -*Int1;
    return 1;
}

int getRealArg(infon* i, fix16_t* Real1, agent* a){
    i->spec2->top=i;
    a->normalize(i->spec2);
    if(!(i->spec2->pFlag&tReal)) return 0;
    *Real1=getReal(i->spec2);
    return 1;
}

void setIntVal(infon* CI, int i){
    UInt tmpFlags=CI->pFlag&0xff000000;
    CI->size=CI->spec2->size;
    CI->value=(infon*) abs(i);
    if (i<0)tmpFlags|=fInvert;
    CI->pFlag=tmpFlags + (tUInt<<goSize)+tUInt;
    CI->wFlag=iNone;
}

void setFloatVal(infon* CI, fix16_t i){
    UInt tmpFlags=CI->pFlag&0xff000000;
    CI->size=CI->spec2->size;
    CI->value=(infon*) i;
    CI->pFlag=tmpFlags + (tUInt<<goSize)+tUInt+tReal;
    CI->wFlag=iNone;
}

void setIntSize(infon* CI, int i){
    UInt tmpFlags=CI->pFlag&0xff000000;
    CI->size=(infon*) abs(i);
    CI->value=0;
    CI->pFlag=tmpFlags + (tUInt<<goSize)+tUInt;
    CI->wFlag=iNone;
}

int autoEval(infon* CI, agent* a){
    int int1, EOT;
    stng funcName=*CI->type;
   std::cout << "EVAL:"<<funcName.S<<"\n";
    if (strcmp(funcName.S, "sin")==0){
        if (!getRealArg(CI, &int1, a)) {CI->wFlag=iNone; return 0;}
        setFloatVal(CI, fix16_sin(int1));  // *pi/180
    } else if (strcmp(funcName.S, "cos")==0){
        if (!getRealArg(CI, &int1, a)) {CI->wFlag=iNone; return 0;}
        setFloatVal(CI, fix16_cos(int1));  // *pi/180
    } else if (strcmp(funcName.S, "time")==0){
        tm now;
        time_t rawtime;
        time(&rawtime);
        now=*localtime(&rawtime);
        if (!getIntArg(CI, &int1, a)) return 0;
        if (int1==0)  setIntVal(CI, now.tm_sec);
    } else if (strcmp(funcName.S, "imageOf")==0){
        char majorType[100];
        char minorType[100];
        infon* args=CI->spec2;
        args->top=CI;
        a->normalize(args);
std::cout << "###ImageOf:" << printInfon(args) << "\n";
        infon* foundMajorType=0;
        infon* foundMinorType=0;
        infon* objectToImage=0;
        UInt argsSize=(UInt)args->size;
        if ((args->pFlag&tType) != tList) {std::cout<<"Error: Argument to imageOf is not a list\n"; return 0;}
        objectToImage=args=args->value;
        if(objectToImage->type==0){
            switch(objectToImage->pFlag&tType){
                case tUInt: strcpy(majorType, "tUInt"); break;
                case tString: strcpy(majorType, "tString"); break;
                case tList: strcpy(majorType, "tList"); break;
                case tUnknown: strcpy(majorType, "tUnknown"); break;
            }
        } else {memcpy(majorType,objectToImage->type->S, objectToImage->type->L); majorType[objectToImage->type->L]=0;}
        if(argsSize>1){
            args=args->next;
            if ((args->pFlag&tType) != tString) {
                std::cout<<"Error: minorType in imageOf is not a string\n";
                return 0;
            } else {copyInfonString2charBuf(args, minorType);}
        }
        args=args->next;
        infon* i=0;
        char tagBuf[100];
        for (EOT=a->StartTerm(Theme, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if ((i->value->pFlag&tType) != tString) continue;
            copyInfonString2charBuf(i->value, tagBuf);
            if (strcmp(tagBuf, majorType) == 0) {
                foundMajorType = i;
                break;
            }
        }
        if (foundMajorType == 0) {
            std::cout<<"Error: majorType not found in Theme\n";
            return 0;
        }
        i=0;
        for (EOT=a->StartTerm(foundMajorType, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if ((i->value->pFlag&tType) != tString) continue;
            copyInfonString2charBuf(i->value, tagBuf);
            if (argsSize==1 || strcmp(tagBuf, minorType) == 0) {
                foundMinorType=i;
                break;
            }
        }
        if (foundMinorType == 0) {
            std::cout<<"Error: no associated minorType found in Theme\n";
            return 0;
        }
        UInt tmpFlags=(CI->pFlag&0xff000000); a->deepCopy(foundMinorType->value->next, CI); CI->pFlag=(CI->pFlag&0x00ffffff)+tmpFlags;
        CI->spec2=args;
       // a->normalize(CI);

    } else if (strcmp(funcName.S, "loadInfon")==0){
        stng str1;
        if (!getStrArg(CI, &str1, a)) return 0;
        str1.S[str1.L]=0;
        std::fstream fin(str1.S);
        QParser q(fin);
        infon* I=q.parse();// std::cout <<"P "; std::cout<<"<"<<printInfon(I)<<"> \n";
        a->normalize(I); // std::cout << "N ";
        if (I==0) {std::cout<<"Error: "<<q.buf<<"\n";}

        copyTo(I,CI);

    } else if (strcmp(funcName.S, "addOne")==0){
        if (!getIntArg(CI, &int1, a)) return 0;
        setIntVal(CI, int1+1);
    } else return 0;

    return 1;
}

#endif
