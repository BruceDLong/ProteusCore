// Proteus Core tests
// The uses testing code from https://github.com/philsquared/Catch/

#define CATCH_CONFIG_RUNNER  // This tell CATCH that we provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "Proteus.h"
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

string normToWorld(agent** a, string entryStr);
void multiNorm(agent** a, string entryStr);

////////////////////////////////////////////////////////////
//                  BEGINNING OF TESTS

#define ST_TEST(IN, OUT) REQUIRE( a.loadInfonFromString(IN, &in, 0) != 0 ); REQUIRE( a.StartTerm(in, &out) == 0 ); s=a.printInfon(out); CAPTURE(s); CHECK(s == OUT);
TEST_CASE( "agent/StartTerm", "Test agent::StartTerm()" ) {
    agent a; infon *in, *out; string s;
    ST_TEST("{3 4 5}", "*1+3");
    ST_TEST("{(3, 4) 5 6}", "*1+3");
    ST_TEST("{((3, 4) 5) 6}", "*1+3");
    ST_TEST("{({3, 4} {5, 6}) 7}", "*1+3");
    ST_TEST("{({3, 4}, {5, 6}) 7}", "{*1+3 *1+4 }");
    ST_TEST("({2 3 4} {5 6 7} 8 9)", "*1+2");
}

#define PARSETEST(NAME, MSG, IN, OUT) TEST_CASE(NAME, MSG){agent a; infon *in; string s; REQUIRE( a.loadInfonFromString(IN, &in, 0) != 0 ); s=a.printInfon(in); CAPTURE(s); CHECK(s == OUT);}
#define NORMTEST( NAME, MSG, IN, OUT) TEST_CASE(NAME, MSG){agent a; infon *in; string s; REQUIRE( a.loadInfonFromString(IN, &in, 1) != 0 ); s=a.printInfon(in); CAPTURE(s); CHECK(s == OUT);}
//#define NORMTEST2(NAME, MSG, IN1, IN2, OUT) TEST_CASE(NAME, MSG){agent *a=0; string s; string in=IN1; CAPTURE(in);s=normToWorld(&a, in);s=normToWorld(&a, IN2); REQUIRE(s == OUT); shutdownProteusCore(); delete a;}
#define MULTITEST(NAME, MSG, SPEC) TEST_CASE(NAME, MSG){agent *a=0; multiNorm(&a, SPEC);}

PARSETEST("parse/num",     "parse a number",       "*2589+654321", "*2589+654321");
PARSETEST("parse/hexnum",  "parse a hex number",   "0xabcd1234", "*1+2882343476");
PARSETEST("parse/binnum",  "parse a binary number","0b11001010101010101010101000001111", "*1+3400182287");
PARSETEST("parse/largenum","parse a large number", "12345678900987654321123456789009876543211234567890", "*1+12345678900987654321123456789009876543211234567890");
PARSETEST("parse/string",  "Parse a string",       R"(+"HELLO")", R"("HELLO")");
PARSETEST("parse/list",    "parse a list",         R"(+{123, 456, "Hello there!", {789, 321}})", R"({*1+123 *1+456 "Hello there!" {*1+789 *1+321 } })");


NORMTEST("merge/num/typed1", "typed int merge 1", "*16+10 = *16+10", "*16+10");
NORMTEST("merge/num/typed2", "typed int merge 2", "*16+_ = *16+9", "*16+9");
NORMTEST("merge/num/typed3", "typed int merge 3", "_ = *16+10", "*16+10");
NORMTEST("merge/num/typed4", "typed int merge 4", "*_+_ = *16+9", "*16+9");
NORMTEST("merge/num/typed5", "typed int merge 5", "*_+8 = *16+8", "*16+8");
NORMTEST("merge/num/typed6", "typed int merge 6", "*16+10 = *16+_", "*16+10");
NORMTEST("merge/num/typed7", "typed int merge 7", "*16+_ = *_+9", "*16+9");
//            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
//            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

NORMTEST("merge/str/typed1", "typed str merge 1", R"(*5+"Hello" = *5+"Hello")", R"("Hello")");
NORMTEST("merge/str/typed2", "typed str merge 2", R"(*5+$ = "Hello")", R"("Hello")");
NORMTEST("merge/str/typed3", "typed str merge 3", R"($ = "Hello")", R"("Hello")");
NORMTEST("merge/str/typed4", "typed str merge 4", R"(*_+$ = "Hello")", R"("Hello")");
NORMTEST("merge/str/typed5", "typed str merge 5", R"("Hello" = *5+$)", R"("Hello")");
//            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
//            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

NORMTEST("merge/list/typed1", "typed list merge 1", "*4+{3 _ $ *3+{...}} = *4+{3 4 'hi' {5 6 7}}", R"({*1+3 *1+4 "hi" {*1+5 *1+6 *1+7 } })");
NORMTEST("merge/list/typed2", "typed list merge 2", "*3+{...} = {1 2 3}", "{*1+1 *1+2 *1+3 }");
NORMTEST("merge/list/typed3", "typed list merge 3", "{} = {}", "{}");
NORMTEST("merge/list/typed4", "typed list merge 4", "*_+{...} = {2 3 4}", "{*1+2 *1+3 *1+4 }");
NORMTEST("merge/list/typed5", "typed list merge 5", "{...} = {2 3 4 ...}", "{*1+2 *1+3 *1+4 ...}");
//            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
//            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

////////// TODO: TESTS OF SIMPLE UN-TYPED MERGE: "=="

///////////////////////////////////////////////////////////

NORMTEST("concat/strCat", "Test string concatenation", R"(('Hello' ' THere!' (' How' ' are' (' you' ' Doing') '?')))", R"("Hello THere! How are you Doing?")");
/*FAIL*/ NORMTEST("concat/parseSrc", "Parse a concatenated string", "[*4+$ *10+$] :== ('DO' 'gsTin' 'tinabulation')", R"("Tintinabul")");  // FAILS until better concat support
NORMTEST("concat/numCat", "Test nested integer concatenation", "((*5+2 *6+3 (*7+4 *8+5 (*9+6 *10+7) 8)))", "*151200+5829248888");
/*FAIL*/ NORMTEST("concat/listCat", "Test nested list concatenation", "(({2 3} {} {3} ({4 5} ({} {6 7}) {8} {})))", "(*1+2 *1+3 *1+3 *1+4 *1+5 *1+6 *1+7 *1+8)"); // See issues on github
NORMTEST("concat/EmbedSeq", "Test Embedded Seq concatenation", "{2 3 (4,5,6) 7 8}", "{*1+2 *1+3 *1+4 *1+5 *1+6 *1+7 *1+8 }");
/*TODO*/ NORMTEST("concat/toNum", "Convert strings and lists to numbers", "", "");
/*TODO*/ NORMTEST("concat/toStr", "Convert numbers and lists to strings", "", "");
/*TODO*/ NORMTEST("concat/sizeTst","Test calculation of the size of concats", "", "");

NORMTEST("parse3n4", "Parse: 3 char then 4 char strings", R"(+{*3+$ *4+$} == 'CatDogs')", R"({"Cat" "Dogs" })");
NORMTEST("anonFunc1", "test anon functions", R"([_,456,789,] <: +123)", "*1+789");
NORMTEST("anonFunc2", "Try a bigger function", R"([+_ ({555, 444, [_] := %\\},)] <: +7000)", "{*1+555 *1+444 *1+7000 }");
NORMTEST("revrseFuncSyntax", "Try reverse func syntax", R"(7000:>[_ ({555, 444, [_] := %\\})])", "{*1+555 *1+444 *1+7000 }");
NORMTEST("catHatDogPig", "Parse 4 three char strings", R"(*4 +{*3+$|...} == +'catHatDogPig')", R"({"cat" "Hat" "Dog" "Pig" })");
NORMTEST("nestedRefs", "test nested references", R"({1 2 {'hi' 'there'} 4 [$ $] := [_ _ {...}] :== %\ 6})", R"({*1+1 *1+2 {"hi" "there" } *1+4 "there" *1+6 })");
NORMTEST("add", "Addition", "+(+3+7)", "*1+10");
NORMTEST("nestedAdd", "Nested Addition", "{ 2 (3 ( 4 5)) 6}", "{*1+2 *1+12 *1+6 }");
NORMTEST("addRefd", "Addition with references", R"({{4, 6, ([_] := %\\ [_, _] := %\\ )}})", "{{*1+4 *1+6 *1+10 } }"); // TODO: shouldn't need external {} here.
NORMTEST("addRevRefs", "Addition with reverse references", R"({{4, 6, (%\\:[_] %\\:[_, _] )}})", "{{*1+4 *1+6 *1+10 } }"); // TODO: shouldn't need external {} here.
NORMTEST("TwoArgFunc", "A two argument function", R"([+{_, _} +{[+_]:=%\\ [+_ +_]:=%\\  [+_]:=%\\} ]<:+{9,4})", "{*1+9 *1+4 *1+9 }");

MULTITEST("tags/defUse", "define and use a tag", "&color=#{*_+_ *_+_ *_+_}  &size=#*_+_  \n color  //:#{_ _ _ }");
MULTITEST("tags/nestedEmpty","nested empty tags", "&frame = {?|...}  &portal = {frame|...} \n portal=*4+{frame|...}  //:{{...} {...} {...} {...} }");
MULTITEST("tags/taggedFunc","Two argument function defined with a tag", R"(&func=[+{_, _} +{%\\:[_] %\\:[_, _]  %\\:[_]}]  )" "\nfunc<: +{9,4}  //:{*1+9 *1+4 *1+9 }");
MULTITEST("simpleParse1", "Simple parsing 1", "{*_ +{'A'|...} 'AARON'} ==  'AAAARON' // This is a comment" R"(//:{{"A" "A" } "AARON" })");
MULTITEST("ParseSelect2nd", "Parse & select option 2", "[...]='ARONdacks' :== {'AARON' 'ARON'} " R"(//:"ARON")");
NORMTEST("ParseSelect1st", "Parse & select option 1", "[...]='AARONdacks' :== {'AARON' 'ARON'} ", R"("AARON")");
NORMTEST("parse2Itms", "Two item parse", "{[...] :== {'AARON' 'BOBO' 'ARON' 'AAAROM'}   'dac'} ==  'ARONdacks'", R"({"ARON" "dac" })");
NORMTEST("parse2ItmsGet1st", "Two item parse, get first option", "{[...] :== {'ARON' 'BOBO' 'AARON' 'CeCe'}   'dac'} ==  'ARONdacks'", R"({"ARON" "dac" })");
// simple
//   #('1', 'Two item parse; error 1', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "ARONjacks"', '<ERROR>'), #NEXT-TASK // No dac, only jac
//   #('1', 'Two item parse; error 2', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "slapjacks"', '<ERROR>'), #NEXT-TASK // slap doesn't match.
//   # Test more 'failed-to-parse' situations.
//   #('1', 'int and strings in function comprehensions', r'{[ ? {555, 444, \\[?]}]<:{"slothe", "Hello", "bob", 65432}|...}', '{ | {*1+555 *1+444 "slothe" } {*1+555 *1+444 "Hello" } {*1+555 *1+444 "bob" } {*1+555 *1+444 *1+65432 } }'),  #FAIL
//   # Add the above test but with a list in the comprehension yield.
NORMTEST("simpleParse2", "Simple Parsing 2", "{*3+$|...}=='CatHatBatDog' ", R"({"Cat" "Hat" "Bat" "Dog" })");
NORMTEST("innerParse1", "Inner parsing 1", "{ {*3+$}|...}='CatHatBatDog' ", R"({{"Cat" } {"Hat" } {"Bat" } {"Dog" } })");
NORMTEST("fromHereIdx1", "fromHere indexing string 1", "{111, '222' %^:[_, _, $] 444, '555', 666, {'hi'}}", R"({*1+111 "222" "555" *1+444 "555" *1+666 {"hi" } })");
NORMTEST("fromHereIdx2", "fromHere indexing string 2", "{111, 222, %^:*3+[...] 444, 555, 666, {'hi'}}", R"({*1+111 *1+222 *1+555 *1+444 *1+555 *1+666 {"hi" } })");
NORMTEST("fromHereIdxNeg", "fromHere indexing negative", "{111, 222, %^:/3+[...] 444, 555, 666, 777}", "{*1+111 *1+222 *1+777 *1+444 *1+555 *1+666 *1+777 }");
NORMTEST("simpleAssoc", "Test lists with simple associations", R"({ {5, 7, 3, 8} {%\\:[_]~ | ...}})", "{{*1+5 *1+7 *1+3 *1+8 } {*1+5 *1+7 *1+3 *1+8 } }");
NORMTEST("internalAssoc", "Test internal associations", R"([ {5, 7, 3, 8} {2 (+(%\\\:[_]~ %\\:[_]~) | ...)}])", "{*1+2 *1+7 *1+14 *1+17 *1+25 }");  // Fails when small ListBufCutOff is used.
MULTITEST("seqFuncPass", "Test sequential func argument passing", R"({{ {5, 7, 3, 8} {addOne<:(%\\:[_]~) | ...}}} //:{{{*1+5 *1+7 *1+3 *1+8 } {*1+6 *1+8 *1+4 *1+9 } } })");
NORMTEST("select2ndItem", "Select 2nd item from list", "*2+[...] := {8 7 6 5 4 3}", "*1+7");
MULTITEST("tags/selByTag", "Select item by concept tag: 'third item of ...'", "&thirdItem=*3+[...] \n thirdItem := {8 7 6 5 4 3}  //:*1+6");
NORMTEST("simpleFilter", "Test simple filtering", "{[_ _]|...} ::= {8 7 6 5 4 3}", "{*1+7 *1+5 *1+3 }");
//MULTITEST("tags/FilterByTag","filtering with a concept-tag", "&everyOther={*2+[...]|...} \n everyOther ::= {8 7 6 5 4 3}  //:{*1+7 *1+5 *1+3 }");
/*FAIL*/ //NORMTEST("filterList", "Filtering with a list", "{[? ?]|...} ::= {111, '222', '333', 444, {'hi'}, {'a', 'b', 'c'}}", R"({"222" *1+444 {"a" "b" "c" } })");  //FAIL: {"222" *1+444 0; "b" 0; }
NORMTEST("internalWrite", "Test internal find-&-write", "{4 5 _ 7} =: [_ _ 6]", "{*1+4 *1+5 *1+6 *1+7 }");
NORMTEST("externalWrite", "Test external find-&-write", "{4 5 _ 7} =: ([???]=6)", "{*1+4 *1+5 *1+6 *1+7 }");
MULTITEST("tags/find-n-write","Test tagged find-&-write", "&setToSix=([???]=6) \n {4 5 _ 7} =: setToSix  //:{*1+4 *1+5 *1+6 *1+7 }");
MULTITEST("byType/findChain","Test chained find-by-type", "&partX=44 \n &partZ=88 \n &obj={partX, partZ} \n obj \n %W:<obj>:<partZ> //:*1+88");
MULTITEST("byType/writeChain","Test chained write-by-type", "&partX=_  &partZ=_  &obj={partX, partZ} \n obj \n %W:<obj>:(<partZ>=77) \n %W:<obj> //:{_ 77 }");
MULTITEST("byType/set","Test set-by-type", "&partX=44 \n &partZ=_ \n &obj={partX, partZ==(%\\\\^:<partX>)} \n obj \n {%W:<obj>} //:{{*1+44 44 } }");
MULTITEST("byType/write-n-set","Test write&set-by-type", "&partX=_ \n  &partZ=_ \n &obj={partX, partZ==(%\\\\^:<partX>)} \n obj \n %W:<obj>:(<partX>==123) \n {%W:<obj>} //:{{123 123 } }");

MULTITEST("repeat/withIdent","Test repetition with an ident", "&varx=_ \n *5+{varx=2|...}   //:{*1+2 *1+2 *1+2 *1+2 *1+2 }");
MULTITEST("repeat/innerIdent","Test repetition with inner ident", "&varx=_ \n *3+{{varx=2}|...}  //:{{*1+2 } {*1+2 } {*1+2 } }");

string causes_SP=R"(

&wheelPosA=_
&wheelPosB=_
&bike={wheelPosA, wheelPosB==(%\\^:<wheelPosA>)}
bike     //:{_ _ }
%W:<bike>:<wheelPosA> = 90  //:90
%W:<bike>:<wheelPosA>       //:90
%W:<bike>   //:{90 90 }

)";
MULTITEST("causes/simpleParts", "Two synchronized parts", causes_SP);

string causes_SP2=R"(

&wheelPosA=_
&wheelPosB=_
&bike={wheelPosA, wheelPosB==(%\\^:<wheelPosA> 5)}
bike     //:{_ _ }
%W:<bike>:<wheelPosA> = 90  //:90
%W:<bike>:<wheelPosA>       //:90
%W:<bike>   //:{90 95 }

)";
MULTITEST("causes/simpleParts2", "Two synchronized parts with offset", causes_SP2);


string time_constVelocity=R"(

&velocity=_
&tobject={ velocity {2 *4+(+(%\\\:[_] %\\:[_]~) | ...)}}
tobject={\<velocity>=3}   //:{*1+3 {*1+2 *1+5 *1+8 *1+11 *1+14 } }
*3+{tobject={\<velocity>=3} |...}  //:{{*1+3 {*1+2 *1+5 *1+8 *1+11 *1+14 } } {*1+3 {*1+2 *1+5 *1+8 *1+11 *1+14 } } {*1+3 {*1+2 *1+5 *1+8 *1+11 *1+14 } } }

)";
MULTITEST("time/constVelocity", "An item at constant velocity", time_constVelocity);


string time_accelList=R"(

&velocity=_
&tobject={ velocity {2 *4+(+(%\\\:[_] %\\:[_]~) | ...)}}
{{4 8 6} {tobject={\<velocity>=(%\\\:[_]~)} |...} }  //:{{*1+4 *1+8 *1+6 } {{*1+4 {*1+2 *1+6 *1+10 *1+14 *1+18 } } {*1+8 {*1+2 *1+10 *1+18 *1+26 *1+34 } } {*1+6 {*1+2 *1+8 *1+14 *1+20 *1+26 } } } }

)";
MULTITEST("time/accelList", "An item with list-based accelerations", time_accelList);


string tictactoe=R"(

&tictactoe={
    &tictactoe square={"", "X", "O"}:*_+[...]
    &tictactoe row   = *3+{square|...}
    &tictactoe board = *3+{row |...}
    // Squares start as "" then end with X or O.
    &tictactoe turn function = [XXXXX]
    &tictactoe start board == *9+{""|...}

    *2+{player|...}
    // players alternate turns
    // games ends when there are 3 in a row or all squares are used
    // winner is X or O or game is a tie.
    {tictactoe start board, ([tictactoe turn function]|...) }
}

tictactoe={\<players> = {Bruce Long, Xander Page} \<moves>={...} }

)";
MULTITEST("tictactoe", "A simple game", tictactoe);

/*
    # TEST: Find (big red bike)
    # TEST: Find (very red bike)

    # TEST: ('2', "Find nth item based on tag", r'{%testTag=[{?|...} <TAG>]}', '<{; }>', 'testTag := {8 7 6 5 4 3}', '*1+7'),
    # TEST: (find all of <tag>): filtering with a tag as descriptor: {[_ TAG]|...} ::= {8 7 6 5 4 3}

    # TEST: [5 _ <sum-of-those-two>] :=: ([_ _]=6)   ==> 11
    # TEST: Like the previous but tag :=: tag
    # TEST:        xxx =:: yyy
    # TEST:        xxx =:: tag
*/



////////////////////////////////////////////////////////////
//         END OF TESTS, BEGINNING OF UTILITY CODE
string entryStr;
infon *topInfon, *Entry;  // use topInfon in the ddd debugger to view World
int AutoEval(infon* CI, agent* a);
bool IsHardFunc(string tag);
extern InfonManager *informationSources;
void RegisterArticle(infon* articleInfon){};

string normToWorld(agent** a, string entryStr){
    string ret="NOT_INITD";
    entryStr="string:// TEST:{ " + entryStr + " \n}"; // TODO: Oddly, removing the '\n' is a problem for simpleParse1
    ProteusParser pp(entryStr, informationSources); pp.agnt=*a;
    Entry=pp.parse(); // cout <<"Parsed.\n";
    if (Entry) try{
        infon* outerList=Entry;
        Entry=Entry->value.listHead; outerList->value.listHead=0; delete outerList;
        if(Entry){
            Entry->top=0; Entry->next=Entry->prev=0;
          //  (*a)->normalize(Entry);
            Entry=(*a)->append(&Entry, (*a)->world);
        } else ret= "NULL ENTRY";
    } catch (char const* errMsg){ret= errMsg;}

    if(ret=="NOT_INITD"){
        if (Entry) ret= (*a)->printInfon(Entry);
        else {ret= pp.buf;}
    }
    return ret;
}

#include <sstream>
void multiNorm(agent** a, string entryStr){
    stringstream ss(entryStr);
    string item, in, out, ret="NULL";
    resetLanguageData();
    (*a) = new agent(0, IsHardFunc, AutoEval);
    (*a)->locale.createCanonical(locale("").name().c_str());
    (*a)->loadInfon("string://TEST:" "{'ONE' 'TWO' {33 44 55} ...}", &(*a)->world, 0); topInfon=(*a)->world;

    while(std::getline(ss, item)) {
        if(item.size()>0) {
            size_t pos=item.find("//:");
            if(pos!=string::npos){
                in=item.substr(0,pos);
                out=item.substr(pos+3);
            } else {in=item; out="";}
            ret=normToWorld(a,in);
            CAPTURE(ret);
            if(out.size()>0) {REQUIRE(ret==out);}
        }
    }
    delete (*a)->world; delete(*a);
}

int main (int argc, char* const argv[])
{
    // global setup...
    if(sizeof(int)!=4) {cout<<"WARNING! int size is "<<sizeof(int)<<" bytes.\n\n"; exit(1);}

    char* resourceDir="../../../resources";
    char* dbName="proteusData.db";
//    char* NewsURL="git://github.com/BruceDLong/NewsTest.git";
    if(initializeProteusCore(resourceDir, dbName)) {cout<< "Could not initialize the Proteus Engine"; exit(1);}

    // Run the tests
    int result = Catch::Main( argc, argv );

    // global clean-up...
    shutdownProteusCore();

    return result;
}
