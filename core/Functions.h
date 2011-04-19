/////////////////////////////////////////////////
// Functions.h  Copyright Bruce Long
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
#ifndef _Functions
#define _Functions

#include <strstream>
#include <iostream>
#include <fstream>
#include "Proteus.h"
#include <stdlib.h>
#include <math.h>
#include <ctime>

#define pi 3.14159
#define Zto1(gInt) (float)(((float)gInt)  /* / 1024.0*/)

#define cpFlags(from, to) {to->flags=(to->flags&0xff000000)+(from->flags&0x00ffffff);}
#define copyTo(from, to) {if(from!=to){to->size=from->size; to->value=from->value; cpFlags(from,to);}}

int getStrArg(infon* i, stng* str, agent* a){
    i->spec1->top=i;
    a->normalize(i->spec1);
    getStng(i->spec1, str);
    return 1;
}

void setString(infon* CI, stng *s){
    UInt tmpFlags=CI->flags&0xff000000;
    CI->size=(infon*) s->L;
    CI->value=(infon*)s->S;
    CI->flags=tmpFlags + (tUInt<<goSize)+asNone+tString;
}

int getIntArg(infon* i, int* Int1, agent* a){
    int sign;
    i->spec1->top=i;
    a->normalize(i->spec1);
    getInt(i->spec1, *Int1, sign);
    if(sign) *Int1 = -*Int1;
    return 1;
}

void setIntVal(infon* CI, int i){
    UInt tmpFlags=CI->flags&0xff000000;
    CI->size=CI->spec1->size;
    CI->value=(infon*) abs(i);
    if (i<0)tmpFlags|=fInvert;
    CI->flags=tmpFlags + (tUInt<<goSize)+asNone+tUInt;
}

void setIntSize(infon* CI, int i){
    UInt tmpFlags=CI->flags&0xff000000;
    CI->size=(infon*) abs(i);
    CI->value=0;
    CI->flags=tmpFlags + (tUInt<<goSize)+asNone+tUInt;
}

int autoEval(infon* CI, agent* a){
    int int1;
    if((CI->spec2->flags&mRepMode)!=asTag) return 0;
    stng funcName=*(stng*)(CI->spec2->spec1);
   std::cout << "EVAL:"<<funcName.S<<"\n";
    if (strcmp(funcName.S, "sin")==0){
        if (!getIntArg(CI, &int1, a)) return 0;
        float f=Zto1(int1)*pi/180;
        float a=sin(f);
        setIntVal(CI, a*1024);
    } else if (strcmp(funcName.S, "cos")==0){
         if (!getIntArg(CI, &int1, a)) return 0;
        float f=Zto1(int1)*pi/180;
        setIntVal(CI, cos(f)*1024);
    } else if (strcmp(funcName.S, "time")==0){
        tm now;
        time_t rawtime;
        time(&rawtime);
        now=*localtime(&rawtime);
        if (!getIntArg(CI, &int1, a)) return 0;
        if (int1==0)  setIntVal(CI, now.tm_sec);
    } else if (strcmp(funcName.S, "draw")==0){
		// get list {mainType, subType, item, sizeLoc, args}
		// search theme for mainType/subType
		// call the function, return the results.
		if (!getIntArg(CI, &int1, a)) return 0;
        setIntVal(CI, int1+1);
		
    } else if (strcmp(funcName.S, "loadInfon")==0){
		stng str1;
		if (!getStrArg(CI, &str1, a)) return 0;
		str1.S[str1.L]=0;
		std::fstream fin(str1.S);
//		while (!fin.eof() && !fin.fail()) {std::cout<<(char)fin.get();} exit(0);
		QParser q(fin);
		infon* I=q.parse();// std::cout <<"P "; std::cout<<"<"<<printInfon(I)<<"> \n";
		agent a;
		a.normalize(I); // std::cout << "N ";
		if (I==0) {std::cout<<"Error: "<<q.buf<<"\n";}
		
        copyTo(I,CI);
		
    }else return 0;

    return 1;
}

#endif
