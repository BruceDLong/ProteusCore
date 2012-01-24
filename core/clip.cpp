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

/* A static variable for holding the line. */
static char *line_read = (char *)NULL;

/* Read a string, and return a pointer to it.
   Returns "quit" on EOF. */
std::string
rl_gets (const char* prompt)
{
  /* If the buffer has already been allocated,
     return the memory to the free pool. */
  if (line_read)
    {
      free (line_read);
      line_read = (char*)NULL;
    }

  /* Get a line from the user. */
  line_read = readline (prompt);

  /* If the line has any text in it,
     save it on the history. */
  if (line_read && *line_read)
    add_history (line_read);

  if (line_read == NULL)
  {
    line_read = (char*)"quit";
  }
    std::string output(line_read);
  return (output);
}

std::string HIST_FILE(getenv("HOME"));
void rl_init() {
  HIST_FILE.append("/.proteus.clip.hist");
  int a = read_history (HIST_FILE.c_str());
  stifle_history (1000);
  std::cout << "History " << HIST_FILE  << " read with exit code " << a << std::endl;
}

void rl_finalize() {
  int a = write_history (HIST_FILE.c_str());
  std::cout << "History " << HIST_FILE << " written with exit code " << a << std::endl;
}

// up-to-here maybe wrap this whole in a IFDEF clause?

char getCH(){
    char ch;
    if (cin.eof()||cin.fail()) {cout << "END OF FILE\n"; exit(1);}
    cin.get(ch);
    return ch;
}

std::string readln(std::string prompt){
    // char ch;
    std::string str="";
    // cout << prompt;
    //for(ch=getCH(); ch!='\n';ch=getCH())str+=ch;
    str = rl_gets(prompt.c_str());
    return str;
}
#include <signal.h>

static void reportFault(int Signal){cout<<"\nSegmentation Fault.\n"; fflush(stdout); abort();}

infon *topInfon, *Entry;
int AutoEval(infon* CI, agent* a);
bool IsHardFunc(char* tag);

int main(int argc, char **argv){
        rl_init(); // with IFDEF readline?
    signal(SIGSEGV, reportFault);

    // Load World
    agent a(0, IsHardFunc, AutoEval);
    std::cout << "Loading world\n";
    std::fstream InfonInW("world.pr");
    QParser q(InfonInW);
    a.world=q.parse();
    if (!a.world) {std::cout<<"Error:"<<q.buf<<"\n"; exit(1);}
    topInfon=a.world;  // use topInfon in the ddd debugger to view World

    a.normalize (a.world);
    cout<<"\nThe Proteus CLI. Type some infons or 'quit'\n\n";
    while(!cin.eof()){
        std::string entry= readln("Proteus: ");
        if (entry=="quit") break;
        if (entry=="") continue;
        //char ch='x', pr; do {pr=ch; ch=getCH(); entry+=ch;} while (!(pr=='%' && ch=='>'));
        //cout << "Parsing ["<<entry<<"]\n";
        entry="<%" + entry + "%>";
        istrstream fin(entry.c_str());
        QParser q(fin);
        Entry=q.parse(); // cout <<"Parsed.\n";
        try{
            a.normalize(Entry); // cout << "Normalizd\n";
        } catch (char const* errMsg){std::cout<<errMsg<<"\n";}
    //  a.append(Entry, a.world);

        if (Entry) cout<<"\n"<<printInfon(Entry)<<"\n\n";
        else {cout<<"\nError: "<<q.buf<<"\n\n";}
    }
        rl_finalize(); // with IFDEF readline?
 return 0;
}

