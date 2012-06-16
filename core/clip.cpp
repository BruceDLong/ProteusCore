#include <strstream>
#include <iostream>
#include <fstream>

std::fstream log;
#include "Proteus.h"
#include <stdlib.h>

using namespace std;


// maybe wrap this whole in a IFDEF clause?

#include <readline/readline.h>
#include <readline/history.h>

static char *line_read = (char *)NULL;
std::string HIST_FILE(getenv("HOME"));

std::string rl_gets (const char* prompt) { //Read a string, and return a pointer to it. returns 'quit' on EOF (ctrl D)

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
  return (std::string(line_read));
}

void rl_init() {
  HIST_FILE.append("/.proteus.clip.hist");
  read_history (HIST_FILE.c_str());
  stifle_history (1000);
}

std::string readln(std::string prompt){
    std::string str="";
    str = rl_gets(prompt.c_str());
    return str;
}
#include <signal.h>

static void reportFault(int Signal){cout<<"\nSegmentation Fault.\n"; fflush(stdout); abort();}

infon *topInfon, *Entry;
int AutoEval(infon* CI, agent* a);
bool IsHardFunc(string tag);

#include <unicode/putil.h>
#include <unicode/ustream.h>
#define PrntLocale(L) {icu::UnicodeString lang,country; cout<<L.getDisplayLanguage(L, lang) <<"-"<< L.getDisplayCountry(L, country) <<" (" << L.getBaseName()<<")\n"; }

int main(int argc, char **argv){
    u_setDataDirectory("/home/bruce/proteus/unicode/install/share/icu/49.1.1");
    rl_init();
    signal(SIGSEGV, reportFault);

    // Load World
    agent a(0, IsHardFunc, AutoEval);
    a.locale.createCanonical(std::locale("").name().c_str());
    cout<<"Locale: "; PrntLocale(a.locale);
  //UNDO:  if(a.loadInfon("world.pr", &a.world, true)) exit(1);
    topInfon=a.world;  // use topInfon in the ddd debugger to view World

    cout<<"\nThe Proteus CLI. Type some infons or 'quit'\n\n";
    while(!cin.eof()){
        std::string entry= readln("Proteus: ");
        if (entry=="quit") break;
        if (entry=="dict") {
            cout<<"Locale: "; PrntLocale(a.locale);
            for(std::map<Tag,infon*>::iterator tagPtr=tag2Ptr.begin(); tagPtr!=tag2Ptr.end(); tagPtr++){
                std::cout<<tagPtr->first.tag<< ":" << tagPtr->first.locale << "\t= " <<printInfon(tagPtr->second)<<"\n";
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
        entry="<%" + entry + "%>";
        istrstream fin(entry.c_str());
        QParser q(fin); q.locale=a.locale;
        Entry=q.parse(); // cout <<"Parsed.\n";
        if (Entry) try{
            a.normalize(Entry); // cout << "Normalizd\n";
            //Entry=a.append(Entry, a.world);
        } catch (char const* errMsg){std::cout<<errMsg<<"\n";}

        if (Entry) cout<<"\n"<<printInfon(Entry)<<"\n\n";
        else {cout<<"\nError: "<<q.buf<<"\n\n";}
    }
 return 0;
}

