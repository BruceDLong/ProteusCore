#include <strstream>
#include <iostream>
#include <fstream>

using namespace std;
fstream log;

#include "Proteus.h"
#include <stdlib.h>
#include "unicode/utypes.h"


// maybe wrap this whole in a IFDEF clause?

#include <readline/readline.h>
#include <readline/history.h>

static char *line_read = (char *)NULL;
string HIST_FILE(getenv("HOME"));

string rl_gets (const char* prompt) { //Read a string, and return a pointer to it. returns 'quit' on EOF (ctrl D)

  if (line_read){ // Free the last string
      free (line_read);
      line_read = (char*)NULL;
    }

  line_read = readline (prompt);  // Get a line from the user.

  if (line_read && *line_read && (strcmp(line_read, "quit")!=0)){ // Save valid lines in history.
    add_history (line_read);
    write_history (HIST_FILE.c_str());
  }
  if (line_read == NULL) line_read = (char*)"quit";
  return (string(line_read));
}

void initReadline() {
  HIST_FILE=".";    // Store history in the current directory, not 'home'
  HIST_FILE.append("/.proteus.clip.hist");
  read_history (HIST_FILE.c_str());
  stifle_history (1000);
}

string readln(string prompt){
    string str="";
    str = rl_gets(prompt.c_str());
    return str;
}
#include <signal.h>

static void reportFault(int Signal){cout<<"\nSegmentation Fault.\n"; fflush(stdout); abort();}

infon *topInfon, *Entry;
int AutoEval(infon* CI, agent* a);
bool IsHardFunc(string tag);

int main(int argc, char **argv){
    string resourceDir="../../../resources";
    string dbName="proteusData.db";
char* NewsURL="git://github.com/BruceDLong/NewsTest.git";
    
	if(initializeProteusCore(resourceDir, dbName, NewsURL)) {cout<< "Could not initialize the Proteus Engine\n\n"; exit(1);}
    initReadline();
    signal(SIGSEGV, reportFault);

    // Load World
    agent a(0, IsHardFunc, AutoEval);
    cout<<"Locale: "<<localeString(&a.locale)<<"\n";
    if(a.loadInfon(resourceDir+"/world.pr", &a.world, 0)) exit(1);
    topInfon=a.world;  // use topInfon in the ddd debugger to view World

    cout<<"\nThe Proteus CLI. Type some infons or 'quit'\n\n";
    if(sizeof(int)!=4) cout<<"WARNING! int size is "<<sizeof(int)<<" bytes.\n\n";
    while(!cin.eof()){
        string entry= readln("Proteus: ");
        if (entry=="quit") break;
        if (entry=="dict") {
            cout<<"Locale: "<<localeString(&a.locale)<<"\n";
            xlater* Xlater = fetchXlater(&a.locale);
            if(!Xlater) {cout<<"Translater for this locale wasn't found.\n"; continue;}
            WordSMap *wordLib=Xlater->wordLibrary;
            for(WordSMap::iterator tagPtr=wordLib->begin(); tagPtr!=wordLib->end(); tagPtr++){
                cout<<tagPtr->second->locale<< ":" << tagPtr->second->norm << "\t= " <<a.printInfon(tagPtr->second->definition)<<"\n";
            }
            continue;
        } else if (entry.substr(0,7)=="locale=") {
            a.setLocale(entry.substr(7));
            cout<<"Locale: "<<localeString(&a.locale)<<"\n";
            continue;
        } if (entry=="") continue;
        //char ch='x', pr; do {pr=ch; ch=getCH(); entry+=ch;} while (!(pr=='%' && ch=='>'));
        //cout << "Parsing ["<<entry<<"]\n";
        entry="<% { " + entry + " \n} %>";
        istrstream fin(entry.c_str());
        QParser q(&fin); q.agnt=&a;
        Entry=q.parse(); // cout <<"Parsed.\n";
        if (Entry) try{

            infon* outerList=Entry;  // This functionality would be better implemented by streamed parsing of world.
            Entry=Entry->value.listHead; outerList->value.listHead=0; delete outerList;

            if(Entry){
                Entry->top=0; Entry->next=Entry->prev=0;
                //a.normalize(Entry); // cout << "Normalizd\n";
                Entry=a.append(&Entry, a.world);
            } else continue;
        } catch (char const* errMsg){cout<<errMsg<<"\n";}

        if (Entry) cout<<"\n"<<a.printInfon(Entry)<<"\n\n";
        else {cout<<"\nError: "<<q.buf<<"\n\n";}
    }
    shutdownProteusCore();
    return 0;
}
