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

#define infsString(inf) ((inf)->value.toString((inf)->getSize().get_ui()))

using namespace std;

bool IsHardFunc(string tag){
        return((tag=="addOne") || (tag=="loadInfon") || (tag=="imageOf") || (tag=="textInfon")
            || (tag=="time") || (tag=="cos") || (tag=="sin") || (tag=="textLine") || (tag=="timestr"));
}

int getStrArg(infon* i, string* str, agent* a){
    i->spec2->top=i;
    a->normalize(i->spec2);
    getStng(i->spec2, str);
    return 1;
}

void setString(infon* CI, string &s){
    CI->size = s.length();
    CI->value=s;
    SetBits(CI->wFlag, mFindMode, iNone);
}

int getIntArg(infon* i, BigInt* Int1, agent* a){
    int sign;
    i->spec2->top=i;
    a->normalize(i->spec2);
    getInt(i->spec2, Int1, &sign);
    if(sign) *Int1 = -*Int1;
    return 1;
}

int getRealArg(infon* i, BigFrac* frac, agent* a){
    i->spec2->top=i;
    a->normalize(i->spec2);
    if(!(InfsType(i->spec2)==tNum && FormatIs(i->spec2->value.flags,fLiteral))) return 0;
    *frac=getReal(i->spec2);
    return 1;
}

void setIntVal(infon* CI, BigFrac i){
    UInt tmpFlags=CI->wFlag&mListPos;
    CI->size=CI->spec2->size;
    CI->value = i; //abs(i);
    if (i<0)tmpFlags|=fInvert;
    SetPureTypeForm(CI->size, tNum, fLiteral); SetPureTypeForm(CI->value, tNum, fLiteral);
    SetBits(CI->wFlag, mFindMode, iNone);
}

void setRealVal(infon* CI, BigFrac num){
    CI->size=CI->spec2->size;
    CI->value=num;
    SetValueFormat(CI,fFract);
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
    if(InfsType(type)==tString){ mode=1;}
    else if(InfsType(type)==tNum){mode=2;}
    else if(InfsType(type)==tList){mode=3;}
    infon* head=new infon((tNum/* UNDO: <<goSize */)+tList,0); throw "TODO: REPAIR SYS CMD LOAD";
    stream = popen(cmd.c_str(), "r");
    while ( fgets(buffer, MAX_SYSCMD_BUFFER, stream) != NULL ){
        infon* i=new infon;
        ++count;
        a->deepCopy(type, i);
        if(mode==1){
            i->size=strlen(buffer);
 //UNDO:           i->value=(infon*)malloc((UInt)i->size+1);
 //UNDO:           strcpy((char*)i->value, buffer);
        } else if(mode==2){
            i->size=1;
 //UNDO:           i->value=buffer;
        } else if(mode==3){ //////////// MAKE THIS WORK
        }
        if(head->value.listHead){i->next=head->value.listHead; i->prev=head->value.listHead->prev; i->top=head->value.listHead; head->value.listHead->prev=i; i->prev->next=i;}
        else {head->value.listHead=i->next=i->prev=i; i->top=head; i->wFlag|=isFirst+isTop;}
    }
    pclose(stream);
    head->size=count;
    return 0;
}

int AutoEval(infon* CI, agent* a){
    int EOT;
    BigFrac bigFrac;
    BigInt bigNum;
    string funcName=CI->type->tag;
 //  cout << "EVAL:"<<funcName.S<<"\n";
    if (funcName=="sin"){
        if (!getRealArg(CI, &bigFrac, a)) {CI->wFlag=iNone; return 0;}
 //UNDO:       setRealVal(CI, mp_sin(bigFrac));  // *pi/180
    } else if (funcName=="cos"){
        if (!getRealArg(CI, &bigFrac, a)) {CI->wFlag=iNone; return 0;}
 //UNDO:       setRealVal(CI, mp_cos(bigFrac));  // *pi/180
    } else if (funcName=="time"){
        tm now;
        time_t rawtime;
        time(&rawtime);
        now=*localtime(&rawtime);
        if (!getIntArg(CI, &bigNum, a)) return 0;
        if (bigNum==0)  setIntVal(CI, now.tm_sec);
    } else if (funcName=="imageOf"){
        string majorType, minorType;
        infon* args=CI->spec2;
        args->top=CI;
        a->normalize(args);
        infon* foundMajorType=0;
        infon* foundMinorType=0;
        infon* objectToImage=0;
        UInt argsSize=args->getSize().get_ui();
        if (InfsType(args) != tList) {cout<<"Error: Argument to imageOf is not a list\n";  exit (0); return 0;}
        objectToImage=args=args->value.listHead;
 //     cout << printInfon(objectToImage) << "---[" << args->type << "]\n";
        if(objectToImage->type==0){
            switch(InfsType(objectToImage)){
                case tNum: majorType="tNum";         break;
                case tString: majorType="tString";   break;
                case tList: majorType="tList";       break;
                case tUnknown: majorType="tUnknown"; break;
            }
        } else {majorType=objectToImage->type->tag;}
        if(argsSize>1){
            args=args->next;
            if (InfsType(args) != tString) {cout<<"Error: minorType in imageOf is not a string\n"; exit (0); return 0;}
            else {minorType=infsString(args);}
        }
        args=args->next;
        infon* i=0;
        infon* theme=(infon*)a->utilField;
        if (theme == 0) {cout<<"Error: Theme is not defined\n";  exit (0);  return 0;}
        string tagStr;
        for (EOT=a->StartTerm(theme, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if (i->value.listHead==0 || InfsType(i->value.listHead) != tString) continue;
            tagStr=infsString(i->value.listHead);
            if (tagStr==majorType){
                foundMajorType = i;
                break;
            }
        }
 //       cout << "majorType="<<majorType<<"\n";
        if (foundMajorType == 0) {cout<<"Error: majorType '"<<majorType<<"' not found in Theme\n";  exit (0);  return 0;}
        i=0;
        for (EOT=a->StartTerm(foundMajorType, &i); !EOT; EOT=a->getNextTerm(&i)) {
            if (i->value.listHead==0 || InfsType(i->value.listHead) != tString) continue;
            tagStr=infsString(i->value.listHead);
            if (argsSize==1 || (tagStr==minorType)) {
                foundMinorType=i;
                break;
            }
        }
        if (foundMinorType == 0) {cout<<"Error: no associated minorType ("<<minorType<<") found in Theme\n"; exit (0);  return 0;}
        UInt tmpFlags=(CI->wFlag&mListPos); a->deepCopy(foundMinorType->value.listHead->next, CI,0,0); CI->wFlag=(CI->wFlag& (~mListPos))+tmpFlags;
        CI->spec2=args;
    } else if (funcName=="loadInfon"){
        string fileName; infon* I;
        if (!getStrArg(CI, &fileName, a)) return 0;
        a->loadInfon(fileName.c_str(), &I, 1);
        copyTo(I,CI);
    } else if (funcName=="timestr"){ //cout << "DOING TIME\n";
        string s="Time:"; char l[30]; itoa(time(0),l); s+=l;
        setString(CI, s);
    } else if (funcName=="infonToText"){
        infon* inf1;
        getInfonArg(CI, &inf1, a);
        string s=printInfon(inf1);
        setString(CI, s);
    } else if (funcName=="textInfon"){
        infon* inf1;
        getInfonArg(CI, &inf1, a);
        string s=printPure(&inf1->value, 0, 0);
        setString(CI, s);
    } else if (funcName=="textLine"){
        infon* inf1, *i; string s="";
        getInfonArg(CI, &inf1, a);
        for (EOT=a->StartTerm(inf1->value.listHead, &i); !EOT; EOT=a->getNextTerm(&i)){
            if (InfsType(i) == tString) s.append(i->value.toString(i->getSize().get_ui()));
            else if (InfsType(i) == tNum) s+=printPure(&i->value, 0, 0);
        }
        setString(CI, s);
    } else if (funcName=="addOne"){
        if (!getIntArg(CI, &bigNum, a)) return 0;
        setIntVal(CI, bigNum+1);
    } else return 0;
    CI->type=0;
    return 1;
}

#endif
