#define CATCH_CONFIG_MAIN  // This tell CATCH to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "Proteus.h"

/*  FROM https://github.com/philsquared/Catch/wiki/Assertion-Macros

*** Natural Expressions ***

The REQUIRE family of macros tests an expression and aborts the test case if it fails. The CHECK family are equivalent but execution continues in the same test case even if the assertion fails. This is useful if you have a series of essentially orthoginal assertions and it is useful to see all the results rather than stopping at the first failure.

REQUIRE( expression ) and
CHECK( expression )
Evaluates the expression and records the result. If an exception is thrown it is caught, reported, and counted as a failure.
These are the macros you will use most of the time

REQUIRE_FALSE( expression ) and
CHECK_FALSE( expression )
Evaluates the expression and records the logical NOT of the result.
If an exception is thrown it is caught, reported, and counted as a failure. (these forms exist as a workaround for the fact that ! prefixed expressions cannot be decomposed).

*** Exceptions ***

REQUIRE_THROWS( expression ) and
CHECK_THROWS( expression )
Expects that an exception (of any type) is be thrown during evaluation of the expression.

REQUIRE_THROWS_AS( expression and exception type ) and
CHECK_THROWS_AS( expression, exception type )
Expects that an exception of the specified type is thrown during evaluation of the expression.

REQUIRE_NOTHROW( expression ) and
CHECK_NOTHROW( expression )
Expects that no exception is thrown during evaluation of the expression.
Matcher expressions

To support Matchers a slightly different form is used. Matchers will be more fully documented elsewhere.

REQUIRE_THAT( lhs, matcher call ) and
CHECK_THAT( lhs, matcher call )

*/

/* FROM https://github.com/philsquared/Catch/wiki/Logging-Macros

Note that the initial << is skipped - instead the insertion sequence is placed in parentheses. These macros come in three forms:

INFO( message expression )
The message is logged to a buffer, but only reported if a subsequent failure occurs within the same test case.
This allows you to log contextual information in case of failures which is not shown during a successful test run.

WARN( message expression )
The message is always reported.

FAIL( message expression )
The message is reported and the test case fails.

SCOPED_INFO( message expression )
As INFO, but is only in effect during the current scope. If a failure occurs beyond the end of the scope the message is not logged.

CAPTURE( expression )

*/

#include <strstream>
#include <iostream>
#include <fstream>

using namespace std;

#include <string>
string entryStr;
infon *topInfon, *Entry;
int AutoEval(infon* CI, agent* a);
bool IsHardFunc(string tag);

void RunCore(){
    char* resourceDir="../resources";
    char* dbName="proteusData.db";
    if(initializeProteusCore(resourceDir, dbName)) throw "Could not initialize the Proteus Engine";

    // Load World
    agent a(0, IsHardFunc, AutoEval);
    a.locale.createCanonical(locale("").name().c_str());
    if(a.loadInfon("world.pr", &a.world, true)) exit(1);
    topInfon=a.world;  // use topInfon in the ddd debugger to view World

    if(sizeof(int)!=4) cout<<"WARNING! int size is "<<sizeof(int)<<" bytes.\n\n";
    while(!cin.eof()){

entryStr="<% { " + entryStr + " \n} %>";
        istrstream fin(entryStr.c_str());
        QParser q(fin); q.agnt=&a;
        Entry=q.parse(); // cout <<"Parsed.\n";
        if (Entry) try{

            infon* outerList=Entry;  // This functionality would be better implemented by streamed parsing of world.
            Entry=Entry->value.listHead; outerList->value.listHead=0; delete outerList;

            if(Entry){
                Entry->top=0; Entry->next=Entry->prev=0;
                //a.normalize(Entry); // cout << "Normalizd\n";
                Entry=a.append(Entry, a.world);
            } else continue;
        } catch (char const* errMsg){cout<<errMsg<<"\n";}

        if (Entry) cout<<"\n"<<a.printInfon(Entry)<<"\n\n";
        else {cout<<"\nError: "<<q.buf<<"\n\n";}
    }
    shutdownProteusCore();
}


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


#define PARSETEST(NAME, MSG, IN, OUT) TEST_CASE(NAME, MSG){agent a; infon *in; string s;  INFO(MSG); REQUIRE( a.loadInfonFromString(IN, &in, 0) != 0 ); s=a.printInfon(in); CAPTURE(s); CHECK(s == OUT);}

PARSETEST("parse/num",     "parse a number",       "*2589+654321", "*2589+654321");
PARSETEST("parse/hexnum",  "parse a hex number",   "0xabcd1234", "*1+2882343476");
PARSETEST("parse/binnum",  "parse a binary number","0b11001010101010101010101000001111", "*1+3400182287");
PARSETEST("parse/largenum","parse a large number", "12345678900987654321123456789009876543211234567890", "*1+12345678900987654321123456789009876543211234567890");
PARSETEST("parse/string",  "Parse a string",       "+\"HELLO\"", "\"HELLO\"");
PARSETEST("parse/list",    "parse a list",         "+{123, 456, \"Hello there!\", {789, 321}}", "{*1+123 *1+456 \"Hello there!\" {*1+789 *1+321 } }");


#define NORMTEST(NAME, MSG, IN, OUT) TEST_CASE(NAME, MSG){agent a; infon *in; string s; REQUIRE( a.loadInfonFromString(IN, &in, 1) != 0 ); s=a.printInfon(in); CAPTURE(s); CHECK(s == OUT);}

NORMTEST("merge/num/typed1", "typed int merge 1", "*16+10 = *16+10", "*16+10");
NORMTEST("merge/num/typed2", "typed int merge 2", "*16+_ = *16+9", "*16+9");
NORMTEST("merge/num/typed3", "typed int merge 3", "_ = *16+10", "*16+10");
NORMTEST("merge/num/typed4", "typed int merge 4", "*_+_ = *16+9", "*16+9");
NORMTEST("merge/num/typed5", "typed int merge 5", "*_+8 = *16+8", "*16+8");
NORMTEST("merge/num/typed6", "typed int merge 6", "*16+10 = *16+_", "*16+10");
NORMTEST("merge/num/typed7", "typed int merge 7", "*16+_ = *_+9", "*16+9");
//            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
//            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

NORMTEST("merge/str/typed1", "typed str merge 1", "*5+\"Hello\" = *5+\"Hello\"", "\"Hello\"");
NORMTEST("merge/str/typed2", "typed str merge 2", "*5+$ = \"Hello\"", "\"Hello\"");
NORMTEST("merge/str/typed3", "typed str merge 3", "$ = \"Hello\"", "\"Hello\"");
NORMTEST("merge/str/typed4", "typed str merge 4", "*_+$ = \"Hello\"", "\"Hello\"");
NORMTEST("merge/str/typed5", "typed str merge 5", "\"Hello\" = *5+$", "\"Hello\"");
//            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
//            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

NORMTEST("merge/list/typed1", "typed list merge 1", "*4+{3 _ $ *3+{...}} = *4+{3 4 \"hi\" {5 6 7}}", "{*1+3 *1+4 \"hi\" {*1+5 *1+6 *1+7 } }");
NORMTEST("merge/list/typed2", "typed list merge 2", "*3+{...} = {1 2 3}", "{*1+1 *1+2 *1+3 }");
NORMTEST("merge/list/typed3", "typed list merge 3", "{} = {}", "{}");
NORMTEST("merge/list/typed4", "typed list merge 4", "*_+{...} = {2 3 4}", "{*1+2 *1+3 *1+4 }");
NORMTEST("merge/list/typed5", "typed list merge 5", "{...} = {2 3 4 ...}", "{*1+2 *1+3 *1+4 ...}");
//            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
//            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

////////// TODO: TESTS OF SIMPLE UN-TYPED MERGE: "=="

///////////////////////////////////////////////////////////

NORMTEST("strCat1", "Test string concatenation", "('Hello' ' THere!' (' How' ' are' (' you' ' Doing') '?'))", "\"Hello THere! How are you Doing?\"");
NORMTEST("parse3n4", "Parse: 3 char then 4 char strings", "+{*3+$ *4+$} == 'CatDogs'", "{\"Cat\" \"Dogs\" }");
NORMTEST("anonFunc1", "test anon functions", "[_,456,789,] <: +123", "*1+789");
NORMTEST("anonFunc2", "Try a bigger function", "[+_ ({555, 444, [_] := %\\\\},)] <: +7000", "{*1+555 *1+444 *1+7000 }");
NORMTEST("revrseFuncSyntax", "Try reverse func syntax", "7000:>[_ ({555, 444, [_] := %\\\\})]", "{*1+555 *1+444 *1+7000 }");
NORMTEST("catHatDogPig", "Parse 4 three char strings", "*4 +{*3+$|...} == +'catHatDogPig'", "{\"cat\" \"Hat\" \"Dog\" \"Pig\" }");
NORMTEST("nestedRefs", "test nested references", "{1 2 {'hi' 'there'} 4 [$ $] := [_ _ {...}] :== %\\ 6}", "{*1+1 *1+2 {\"hi\" \"there\" } *1+4 \"there\" *1+6 }");
NORMTEST("add", "Addition", "+(+3+7)", "*1+10");
NORMTEST("nestedAdd", "Nested Addition", "{ 2 (3 ( 4 5)) 6}", "{*1+2 *1+12 *1+6 }");
NORMTEST("addRefd", "Addition with references", "{{4, 6, ([_] := %\\\\ [_, _] := %\\\\ )}}", "{{*1+4 *1+6 *1+10 } }"); // TODO: shouldn't need external {} here.
NORMTEST("addRevRefs", "Addition with reverse references", "{{4, 6, (%\\\\:[_] %\\\\:[_, _] )}}", "{{*1+4 *1+6 *1+10 } }"); // TODO: shouldn't need external {} here.
NORMTEST("TwoArgFunc", "A two argument function", "[+{_, _} +{[+_]:=%\\\\ [+_ +_]:=%\\\\  [+_]:=%\\\\} ]<:+{9,4}", "{*1+9 *1+4 *1+9 }");

//TEST2("define and use a tag", "&color=#{*_+_ *_+_ *_+_}  &size=#*_+_", "<{}>");
//TEST2("nested empty tags", "&frame = {?|...}  &portal = {frame|...}", "<{}>");
//TEST2("Two argument function defined with a tag", "&func={+{_, _} +{%\\:[_] %\\:[_, _]  %\\:[_]}}  ", "<{}>");
NORMTEST("simpleParse1", "Simple parsing 1", "{*_ +{'A'|...} 'AARON'} ==  'AAAARON' // This is a comment", "{{\"A\" \"A\" } \"AARON\" }");
NORMTEST("ParseSelect2nd", "Parse & select option 2", "[...]='ARONdacks' :== {'AARON' 'ARON'} ", "\"ARON\"");
NORMTEST("ParseSelect1st", "Parse & select option 1", "[...]='AARONdacks' :== {'AARON' 'ARON'} ", "\"AARON\"");
NORMTEST("parse2Itms", "Two item parse", "{[...] :== {'AARON' 'BOBO' 'ARON' 'AAAROM'}   'dac'} ==  'ARONdacks'", "{\"ARON\" \"dac\" }");
NORMTEST("parse2ItmsGet1st", "Two item parse, get first option", "{[...] :== {'ARON' 'BOBO' 'AARON' 'CeCe'}   'dac'} ==  'ARONdacks'", "{\"ARON\" \"dac\" }");

//   #('1', 'Two item parse; error 1', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "ARONjacks"', '<ERROR>'), #NEXT-TASK // No dac, only jac
//   #('1', 'Two item parse; error 2', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "slapjacks"', '<ERROR>'), #NEXT-TASK // slap doesn't match.
//   #('1', 'int and strings in function comprehensions', r'{[ ? {555, 444, \\[?]}]<:{"slothe", "Hello", "bob", 65432}|...}', '{ | {*1+555 *1+444 "slothe" } {*1+555 *1+444 "Hello" } {*1+555 *1+444 "bob" } {*1+555 *1+444 *1+65432 } }'),  #FAIL
//   # Add the above test but with a list in the comprehension yeild.
NORMTEST("simpleParse2", "Simple Parsing 2", "{*3+$|...}=='CatHatBatDog' ", "{\"Cat\" \"Hat\" \"Bat\" \"Dog\" }");
NORMTEST("innerParse1", "Inner parsing 1", "{ {*3+$}|...}='CatHatBatDog' ", "{{\"Cat\" } {\"Hat\" } {\"Bat\" } {\"Dog\" } }");
// /*FAILS*/ NORMTEST("ParseConcat", "Parse a concatenated string", "[*4+$ *10+$] :== ('DO' 'gsTin' 'tinabulation')", "\"Tintinabul\"");  // FAILS until better concat support
NORMTEST("fromHereIdx1", "fromHere indexing string 1", "{111, '222' %^:[_, _, $] 444, '555', 666, {'hi'}}", "{*1+111 \"222\" \"555\" *1+444 \"555\" *1+666 {\"hi\" } }");
NORMTEST("fromHereIdx2", "fromHere indexing string 2", "{111, 222, %^:*3+[...] 444, 555, 666, {'hi'}}", "{*1+111 *1+222 *1+555 *1+444 *1+555 *1+666 {\"hi\" } }");
NORMTEST("fromHereIdxNeg", "fromHere indexing negative", "{111, 222, %^:/3+[...] 444, 555, 666, 777}", "{*1+111 *1+222 *1+777 *1+444 *1+555 *1+666 *1+777 }");
NORMTEST("simpleAssoc", "Test lists with simple associations", "{ {5, 7, 3, 8} {%\\\\:[_]~ | ...}}", "{{*1+5 *1+7 *1+3 *1+8 } {*1+5 *1+7 *1+3 *1+8 } }");
/*FAILS*/ NORMTEST("internalAssoc", "Test internal associations", "[ {5, 7, 3, 8} {2 (+(%\\\\\\:[_]~ %\\\\:[_]~) | ...)}]", "{*1+2 *1+7 *1+14 *1+17 *1+25 }");  // FAIL: Fails when small ListBufCutOff is used.
//NORMTEST("SeqFuncPass", "Test sequential func argument passing", "{{ {5, 7, 3, 8} {addOne<:(%\\\\:[_]~) | ...}}}", "{{{*1+5 *1+7 *1+3 *1+8 } {*1+6 *1+8 *1+4 *1+9 } } }");
//NORMTEST("Select2ndItem", "Select 2nd item from list", "*2+[...] := {8 7 6 5 4 3}", "*1+7");
//TEST2("Select item by concept tag: 'third item of ...'", "&thirdItem=*3+[...]", "<{}>");
NORMTEST("simpleFilter", "Test simple filtering", "{[_ _]|...} ::= {8 7 6 5 4 3}", "{*1+7 *1+5 *1+3 }");
//TEST2("filtering with a concept-tag", "&everyOther={*2+[...]|...}", "<{}>");
//NORMTEST("filterList", "Filtering with a list", "{[? ?]|...} ::= {111, '222', '333', 444, {'hi'}, {'a', 'b', 'c'}}", "{\"222\" *1+444 {\"a\" \"b\" \"c\" } }");  //FAIL: {"222" *1+444 0; "b" 0; }
NORMTEST("internalWrite", "Test internal find-&-write", "{4 5 _ 7} =: [_ _ 6]", "{*1+4 *1+5 *1+6 *1+7 }");
NORMTEST("externalWrite", "Test external find-&-write", "{4 5 _ 7} =: ([???]=6)", "{*1+4 *1+5 *1+6 *1+7 }");
/*TEST2("Test tagged find-&-write", "&setToSix=([???]=6)", "{; }");
TEST2("Test chained find-by-type", "&partX=44 \n &partZ=88 \n &obj={partX, partZ}", "");
TEST2("Test chained write-by-type", "&partX=_  &partZ=_  &obj={partX, partZ}", "");
TEST2("Test set-by-type", "&partX=44  &partZ=_  &obj={partX, partZ==(%\\\^:<partX>)}", "");
TEST2("Test write&set-by-type", "&partX=_  &partZ=_  &obj={partX, partZ==(%\\\^:<partX>)}", "");

    # TEST: Find (big red bike)
    # TEST: Find (very red bike)

    # TEST: ('2', "Find nth item based on tag", r'{%testTag=[{?|...} <TAG>]}', '<{; }>', 'testTag := {8 7 6 5 4 3}', '*1+7'),
    # TEST: (find all of <tag>): filtering with a tag as descriptor: {[_ TAG]|...} ::= {8 7 6 5 4 3}

    # TEST: [5 _ <sum-of-those-two>] :=: ([_ _]=6)   ==> 11
    # TEST: Like the previous but tag :=: tag
    # TEST:        xxx =:: yyy
    # TEST:        xxx =:: tag
*/
