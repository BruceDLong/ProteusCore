#include <strstream>
#include <iostream>
#include <fstream>

using namespace std;
fstream log;

#include "Proteus.h"
#include <stdlib.h>


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

void rl_init() {
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
    u_setDataDirectory("/home/bruce/proteus/unicode/install/share/icu/49.1.1");
    rl_init();
    signal(SIGSEGV, reportFault);

    // Load World
    agent a(0, IsHardFunc, AutoEval);
    populateLangExtentions();
    a.locale.createCanonical(locale("").name().c_str());
    cout<<"Locale: "; PrntLocale(a.locale);
    if(a.loadInfon("world.pr", &a.world, true)) exit(1);
    topInfon=a.world;  // use topInfon in the ddd debugger to view World

    cout<<"\nThe Proteus CLI. Type some infons or 'quit'\n\n";
    while(!cin.eof()){
        string entry= readln("Proteus: ");
        if (entry=="quit") break;
        if (entry=="dict") {
            cout<<"Locale: "; PrntLocale(a.locale);
            for(WordSMap::iterator tagPtr=topTag2Def.begin(); tagPtr!=topTag2Def.end(); tagPtr++){
                cout<<tagPtr->second->locale<< ":" << tagPtr->second->norm << "\t= " <<printInfon(tagPtr->second->definition)<<"\n";
            }
            continue;
        } else if (entry.substr(0,7)=="locale=") {
            icu::UnicodeString str,lang,country;
            a.locale=icu::Locale::createCanonical(entry.substr(7).c_str());
            cout<<"Locale: "; PrntLocale(a.locale);
            continue;
        } if (entry=="") continue;
        //char ch='x', pr; do {pr=ch; ch=getCH(); entry+=ch;} while (!(pr=='%' && ch=='>'));
        //cout << "Parsing ["<<entry<<"]\n";
        entry="<% { " + entry + " \n} %>";
        istrstream fin(entry.c_str());
        QParser q(fin); q.locale=a.locale;
        Entry=q.parse(); // cout <<"Parsed.\n";
        if (Entry) try{

{ // This functionality would be better implemented by streamed parsing of world.
            infon* outerList=Entry;
            if(Entry->tag2Ptr){
                if (a.world->tag2Ptr==0) a.world->tag2Ptr=new WordSMap;
                a.world->tag2Ptr->insert(Entry->tag2Ptr->begin(), Entry->tag2Ptr->end());
                delete Entry->tag2Ptr;
            }
            Entry=Entry->value.listHead; outerList->value.listHead=0; delete outerList;
}
            if(Entry){
                Entry->top=0; Entry->next=Entry->prev=0;
                //a.normalize(Entry); // cout << "Normalizd\n";
                Entry=a.append(Entry, a.world);
            } else continue;
        } catch (char const* errMsg){cout<<errMsg<<"\n";}

        if (Entry) cout<<"\n"<<printInfon(Entry)<<"\n\n";
        else {cout<<"\nError: "<<q.buf<<"\n\n";}
    }
 return 0;
}

