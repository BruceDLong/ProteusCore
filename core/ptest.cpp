#include <strstream>
#include <iostream>
#include <fstream>

std::fstream log;
#include "Proteus.h"
#include <stdlib.h>

char getCH(){
    char ch;
    if (std::cin.eof()||std::cin.fail()) {std::cout << "END OF FILE\n"; exit(1);}
    std::cin.get(ch);
    return ch;
}

void readln(std::string &str){
    char ch; str="";
    for(ch=getCH(); ch!='\n';ch=getCH())str+=ch;
}
#include <signal.h>
static void reportFault(int Signal){OUT("Segmentation Fault."); fflush(stdout); abort();}

infon* topInfon;
int AutoEval(infon* CI, agent* a);
bool IsHardFunc(char* tag);

int main(int argc, char **argv)
{
signal(SIGSEGV, reportFault);
std::string desc, world, query, line; world=""; query="";
int mode;
agent a(0, IsHardFunc, AutoEval);
do{
    std::cin >> mode;
    if(mode==5) return 0;
    desc=""; readln(desc);readln(desc);
    world =""; query=""; char ch='x', pr;
    do {
        pr=ch; ch=getCH(); world+=ch;
    } while (!(pr=='%' && ch=='>'));

    if (mode==2) do {
        pr=ch; ch=getCH(); query+=ch;
    } while (!(pr=='%' && ch=='>'));

//std::cerr << "#####################\nBeginning " << desc << " ("<< mode<<")\n";
    // Load World
    std::cout << "Parsing ["<<world<<"]\n";
    std::istrstream fin(world.c_str());
    QParser q(fin);
    a.world=q.parse(); std::cout <<"Parsed.\n";
    if (mode>0){
        std::cout <<"Norming World...\n";
        topInfon=a.world;
        a.normalize(a.world);  std::cout << "Normed\n";
    }
    if (a.world) std::cout<<"<"<<printInfon(a.world)<<"> \n";
    else {std::cout<<"Error: "<<q.buf<<"\n"; exit(0);}
if (mode==2){
    // Load DispList
    std::cout<<"Parsing query ["<<query<<"]\n";

    std::istrstream fin2(query.c_str());
    QParser D(fin2);
    infon* queryinf=D.parse(); std::cout << "parsed\n";
    if(queryinf) std::cout<<"<<["<<printInfon(queryinf).c_str()<<"]>>\n";
    else {std::cout<<"Error: "<<D.buf<<"\n"; exit(0);}
    std::cout<<"Norming query...";
    topInfon=queryinf;
    a.normalize(queryinf); std::cout<<"Normed\n";
    std::cout<<printInfon(queryinf)<<"\n";
}
 }while (mode!=5 && ! std::cin.eof());

 return 0;
}
/*
int main(int argc, char **argv)
{
  //log.open("log.txt");
    // Load World
    std::cout << "Loading world.pr\n";
    std::fstream fin("../slyp/world.pr");
    QParser q(fin);
    a.world=q.parse();
    if (a.world) std::cout<<"["<<printInfon(a.world)<<"]\n";
    else {std::cout<<"Error:"<<q.buf<<"\n"; exit(1);}
    agent a;
    a.normalize(a.world);

    // Load DispList
    std::cout << "Loading display.pr\n";
    std::fstream fin2("testScanConcat.pr");   //("../slyp/display.pr");
    QParser D(fin2);
    infon* displayList=D.parse(); std::cout << "parsed\n";
    if(displayList) std::cout<<"["<<printInfon(displayList).c_str()<<"]\n";
    else {std::cout<<"Error:"<<D.buf<<"\n"; exit(1);}

    a.normalize(displayList);
//    OUT("Normed.");
//    OUT(printInfon(displayList))

    int count=0;
    int EOT=0;
    infon* i=0;
    infon* CI=displayList; topInfon=displayList;
    EOT=StartTerm(CI, &i);
    while(!EOT){
        std::cout << count<<":[" << printInfon(i).c_str() << "]\n";
        if (++count==20) return 0;
        EOT= getNextTerm(&i);
}
std::cout << "DONE\n";

    return 1;
}
*/
