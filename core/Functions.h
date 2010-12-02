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

#include "Proteus.h"
#include <stdlib.h>
#include <math.h>
#include <ctime>

#define pi 3.14159
#define Zto1(gInt) (float)(((float)gInt)  /* / 1024.0*/)

int getIntArg(infon* i, int* Int1, agent* a){
    int sign;
    i->spec1->top=i;
    a->normalize(i->spec1);
    getInt(i->spec1, *Int1, sign);
    if(sign) *Int1 = -*Int1;
    return 1;
}

void setIntVal(infon* CI, int i){
    ptrdiff_t tmpFlags=CI->flags&0xff000000;
    CI->size=CI->spec1->size;
    CI->value=(infon*) abs(i);
    if (i<0)tmpFlags|=fInvert;
    CI->flags=tmpFlags + (tUInt<<goSize)+asNone+tUInt;
}

void setIntSize(infon* CI, int i){
    ptrdiff_t tmpFlags=CI->flags&0xff000000;
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
    } else return 0;

    return 1;
}

#endif
