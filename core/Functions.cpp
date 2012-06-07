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
#include "remiss.h"
#include <ctime>

#define pi 3.14159
#define Zto1(gInt) (float)(((float)gInt)  /* / 1024.0*/)

#define cpFlags(from, to, mask) {to->pFlag=(to->pFlag& ~(mask))+((from)->pFlag&mask); to->wFlag=from->wFlag; {if(to->type==0) to->type=from->type;}}
#define copyTo(from, to) {if(from!=to){to->size=(from)->size; to->value=(from)->value; cpFlags((from),to,0x00ffffff);}}

#define copyInfonString2charBuf(inf, buf) {memcpy(buf, (char*)inf->value, (UInt)inf->size); buf[(UInt)inf->size]=0;}

using namespace std;

bool IsHardFunc(string tag){
        return((tag=="addOne") || (tag=="loadInfon") || (tag=="imageOf") || (tag=="textInfon")
            || (tag=="time") || (tag=="cos") || (tag=="sin") || (tag=="textLine") || (tag=="timestr"));
}

int getStrArg(infon* i, stng* str, agent* a){
    i->spec2->top=i;
    a->normalize(i->spec2);
    getStng(i->spec2, str);
    return 1;
}

void setString(infon* CI, stng *s){
    UInt tmpFlags=CI->pFlag&mListPos;
    CI->size=(infon*) s->L;
    CI->value=(infon*)s->S;
    CI->pFlag=tmpFlags + (tNum<<goSize)+tString;
    CI->wFlag=iNone;
}

int getIntArg(infon* i, int* Int1, agent* a){
    int sign;
    i->spec2->top=i;
    a->normalize(i->spec2);
    getInt(i->spec2, Int1, &sign);
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
    UInt tmpFlags=CI->pFlag&mListPos;
    CI->size=CI->spec2->size;
    CI->value=(infon*) abs(i);
    if (i<0)tmpFlags|=fInvert;
    CI->pFlag=tmpFlags + ((tNum+fLiteral)<<goSize)+(tNum+fLiteral);
    CI->wFlag=iNone;
}

void setRealVal(infon* CI, fix16_t i){
    UInt tmpFlags=CI->pFlag&mListPos;
    CI->size=CI->spec2->size;
    CI->value=(infon*) i;
    CI->pFlag=tmpFlags + (tNum<<goSize)+tNum+tReal;
    CI->wFlag=iNone;
}

void setIntSize(infon* CI, int i){
    UInt tmpFlags=CI->pFlag&mListPos;
    CI->size=(infon*) abs(i);
    CI->value=0;
    CI->pFlag=tmpFlags + (tNum<<goSize)+tNum;
    CI->wFlag=iNone;
}

int getInfonArg(infon* i, infon** infOut, agent* a){
    i->spec2->top=i;
    a->normalize(i->spec2);
    *infOut=i->spec2;
    return 1;
}

const int MAX_SYSCMD_BUFFER=2*1024;
int LoadFromSystemCmd(agent* a, infon* type, string cmd){
    FILE *stream; string line; int mode=0; int count=0;
    char buffer[MAX_SYSCMD_BUFFER];
    if((type->pFlag&tType)==tString){ mode=1;}
    else if((type->pFlag&tType)==tNum){mode=2;}
    else if((type->pFlag&tType)==tList){mode=3;}
    infon* head=new infon((tNum<<goSize)+tList,0);
    stream = popen(cmd.c_str(), "r");
    while ( fgets(buffer, MAX_SYSCMD_BUFFER, stream) != NULL ){
        infon* i=new infon;
  //      i->pFlag&=~((fUnknown<<goSize)+fUnknown);
        ++count;
        a->deepCopy(type, i);
        if(mode==1){
            i->size=(infon*)strlen(buffer);
            i->value=(infon*)malloc((UInt)i->size+1);
            strcpy((char*)i->value, buffer);
        } else if(mode==2){
            i->size=(infon*)1;
            i->value=(infon*)atoi(buffer);
        } else if(mode==3){ //////////// MAKE THIS WORK
        }
        if(head->value){i->next=head->value; i->prev=head->value->prev; i->top=head->value; head->value->prev=i; i->prev->next=i;}
        else {head->value=i->next=i->prev=i; i->top=head; i->pFlag|=isFirst+isTop;}
    }
    pclose(stream);
    head->size=(infon*)count;
    return 0;
}

int AutoEval(infon* CI, agent* a){
    int int1, EOT;
    string funcName=CI->type->tag;
 //  std::cout << "EVAL:"<<funcName.S<<"\n";
    if (funcName=="sin"){
        if (!getRealArg(CI, &int1, a)) {CI->wFlag=iNone; return 0;}
        setRealVal(CI, fix16_sin(int1));  // *pi/180
    } else if (funcName=="cos"){
        if (!getRealArg(CI, &int1, a)) {CI->wFlag=iNone; return 0;}
        setRealVal(CI, fix16_cos(int1));  // *pi/180
    } else if (funcName=="time"){
        tm now;
        time_t rawtime;
        time(&rawtime);
        now=*localtime(&rawtime);
        if (!getIntArg(CI, &int1, a)) return 0;
        if (int1==0)  setIntVal(CI, now.tm_sec);
    } else if (funcName=="imageOf"){
        string majorType;
        char minorType[100];
        infon* args=CI->spec2;
        args->top=CI;
        a->normalize(args);
        infon* foundMajorType=0;
        infon* foundMinorType=0;
        infon* objectToImage=0;
        UInt argsSize=(UInt)args->size;
        if ((args->pFlag&tType) != tList) {cout<<"Error: Argument to imageOf is not a list\n";  exit (0); return 0;}
        objectToImage=args=args->value;
 //     cout << printInfon(objectToImage) << "---[" << args->type << "]\n";
        if(objectToImage->type==0){
            switch(objectToImage->pFlag&tType){
                case tNum: majorType="tNum";         break;
                case tString: majorType="tString";   break;
                case tList: majorType="tList";       break;
                case tUnknown: majorType="tUnknown"; break;
            }
        } else {majorType=objectToImage->type->tag;}
        if(argsSize>1){
            args=args->next;
            if ((args->pFlag&tType) != tString) {cout<<"Error: minorType in imageOf is not a string\n"; exit (0); return 0;}
            else {copyInfonString2charBuf(args, minorType);}
        }
        args=args->next;
        infon* i=0;
        infon* theme=(infon*)a->utilField;
        if (theme == 0) {cout<<"Error: Theme is not defined\n";  exit (0);  return 0;}
        char tagBuf[100];
        for (EOT=a->StartTerm(theme, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if ((i->value->pFlag&tType) != tString) continue;
            copyInfonString2charBuf(i->value, tagBuf);
            if (tagBuf==majorType){
                foundMajorType = i;
                break;
            }
        }
 //       cout << "majorType="<<majorType<<"\n";
        if (foundMajorType == 0) {cout<<"Error: majorType '"<<majorType<<"' not found in Theme\n";  exit (0);  return 0;}
        i=0;
        for (EOT=a->StartTerm(foundMajorType, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if ((i->value->pFlag&tType) != tString) continue;
            copyInfonString2charBuf(i->value, tagBuf);
            if (argsSize==1 || strcmp(tagBuf,minorType)==0) {
                foundMinorType=i;
                break;
            }
        }
        if (foundMinorType == 0) {cout<<"Error: no associated minorType ("<<minorType<<") found in Theme\n"; exit (0);  return 0;}
        UInt tmpFlags=(CI->pFlag&mListPos); a->deepCopy(foundMinorType->value->next, CI); CI->pFlag=(CI->pFlag& (~mListPos))+tmpFlags;
        CI->spec2=args;
    } else if (funcName=="loadInfon"){
        stng str1; infon* I;
        if (!getStrArg(CI, &str1, a)) return 0;
        str1.S[str1.L]=0;
        a->loadInfon(str1.S, &I, 1);
        copyTo(I,CI);
    } else if (funcName=="timestr"){ //cout << "DOING TIME\n";
        string s="Time:"; char l[30]; itoa(time(0),l); s+=l;
        stng sOut; stngCpy(sOut, s.c_str());
        setString(CI, &sOut);
    } else if (funcName=="infonToText"){
        infon* inf1;
        getInfonArg(CI, &inf1, a);
        string s=printInfon(inf1);
        stng sOut; stngCpy(sOut, s.c_str());
        setString(CI, &sOut);
    } else if (funcName=="textInfon"){
        infon* inf1;
        getInfonArg(CI, &inf1, a);
        string s=printPure(inf1->value, inf1->pFlag, 0, 0);
        stng sOut; stngCpy(sOut, s.c_str());
        setString(CI, &sOut);
    } else if (funcName=="textLine"){
        infon* inf1, *i; string s="";
        getInfonArg(CI, &inf1, a);
        for (EOT=a->StartTerm(inf1->value, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if ((i->pFlag&tType) == tString) s.append((char*)i->value, (UInt)i->size);
            else if ((i->pFlag&tType) == tNum) s+=printPure(i->value, i->pFlag, 0, 0);
        }
        stng sOut; stngCpy(sOut, s.c_str());
        setString(CI, &sOut);
    } else if (funcName=="addOne"){
        if (!getIntArg(CI, &int1, a)) return 0;
        setIntVal(CI, int1+1);
    } else return 0;
    CI->type=0;
    return 1;
}

#endif
