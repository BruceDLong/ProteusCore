#include <strstream>
#include <iostream>
#include <fstream>

std::fstream log;
#include "Proteus.h"
#include <stdlib.h>

using namespace std;

char getCH(){
    char ch;
    if (cin.eof()||cin.fail()) {cout << "END OF FILE\n"; exit(1);}
    cin.get(ch);
    return ch;
}

void readln(std::string &str){
    char ch; str="";
    for(ch=getCH(); ch!='\n';ch=getCH())str+=ch;
}
#include <signal.h>

static void reportFault(int Signal){cout<<"\nSegmentation Fault.\n"; fflush(stdout); abort();}
extern infon *World;
infon* topInfon;

int main(int argc, char **argv){
	signal(SIGSEGV, reportFault);
	topInfon=World;  // use topInfon in the ddd debugger to view World
	cout<<"\nThe Proteus CLI. Type some infons or 'quit'\n\n";
	agent a;
	while(!cin.eof()){
		cout << "Proteus: ";
		string entry=""; readln(entry);
		if (entry=="quit") break;
		if (entry=="") continue;
		//char ch='x', pr; do {pr=ch; ch=getCH(); entry+=ch;} while (!(pr=='%' && ch=='>'));
		//cout << "Parsing ["<<entry<<"]\n";
		entry="<%" + entry + "%>";
		istrstream fin(entry.c_str());
		QParser q(fin);
		infon* Entry=q.parse(); // cout <<"Parsed.\n";
		a.normalize(Entry); // cout << "Normalizd\n";
	//	a.append(Entry, World);
		
		if (Entry) cout<<"\n"<<printInfon(Entry)<<"\n\n";
		else {cout<<"\nError: "<<q.buf<<"\n\n";}
	}

 return 0;
}

