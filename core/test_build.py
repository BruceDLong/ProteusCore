import re
import pexpect
_multiprocess_can_split_ = True

#def setup_func():
#   child=pexpect.spawn('../ptest');
#
#def teardown_func():
#   child.send "5\ndone\n!\n";

def test_engine():
  #  inf=open('../tests.pr')
  #  testsStr=inf.read()
  #  testsLst= re.findall(r'^\s*(\d)\s+([\S ]*)\s*<%\s*(.*?)\s*%>', testsStr,re.M);

   testsLst=[

   # PARSER TESTS
   ('0', 'parse a number', '*2589+654321',r'<*2589+654321>'),
   ('0', 'Parse a string', '+"HELLO"', '<"HELLO">'),
   ('0', 'parse a list', '+{123, 456, "Hello there!", {789, 321}}', '<{*1+123 *1+456 "Hello there!" {*1+789 *1+321 } }>'),
   ('0', 'parse nested empty lists', '+{{},{}}', '<{{} {} }>'),

   # TESTS OF SIMPLE TYPED MERGE: "="
   ('1', 'typed int merge 1', '*16+10 = *16+10', '*16+10'),
   ('1', 'typed int merge 2', '*16+_ = *16+9', '*16+9'),
   ('1', 'typed int merge 3', '_ = *16+10', '*16+10'),
   ('1', 'typed int merge 4', '*_+_ = *16+9', '*16+9'),
   ('1', 'typed int merge 5', '*_+8 = *16+8', '*16+8'),
   ('1', 'typed int merge 6', '*16+10 = *16+_', '*16+10'),
   ('1', 'typed int merge 7', '*16+_ = *_+9', '*16+9'),
   #            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
   #            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

   ('1', 'typed str merge 1', '*5+"Hello" = *5+"Hello"', 'Hello'),
   ('1', 'typed str merge 2', '*5+$ = "Hello"', '"Hello"'),
   ('1', 'typed str merge 3', '$ = "Hello"', '"Hello"'),
   ('1', 'typed str merge 4', '*_+$ = "Hello"', '"Hello"'),
   ('1', 'typed str merge 5','"Hello" = *5+$', 'Hello'),
   #            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
   #            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

   ('1', 'typed list merge 1', '*4+{3 _ $ *3+{...}} = *4+{3 4 "hi" {5 6 7}}', '{*1+3 *1+4 "hi" {*1+5 *1+6 *1+7 } }'),
   ('1', 'typed list merge 2', '*3+{...} = {1 2 3}', '{*1+1 *1+2 *1+3 }'),
   ('1', 'typed list merge 3', '{} = {}', '{}'),
   ('1', 'typed list merge 4', '*_+{...} = {2 3 4}', '{*1+2 *1+3 *1+4 }'),
   ('1', 'typed list merge 5','{...} = {2 3 4 ...}', '{*1+2 *1+3 *1+4 ...}'),
   #            Add tests with zero size / value, value larger than size, later: negative, fractional, expression.
   #            Add Rainy day tests: Mis-matched types, mis-matched sizes, mis-matched values

   # TESTS OF SIMPLE UN-TYPED MERGE: "=="

   # TESTS OF MISC
   ('1', 'Parse: 3 char then 4 char strings', '+{*3+$ *4+$} == "CatDogs"', '<{"Cat" "Dogs" }>'),
   ('1', 'test anon functions', '[_,456,789,] <: +123', '<*1+789>'),
   ('1', 'Try a bigger function', r'[+_ ({555, 444, [_] := %\\\},)] <: +7000', r'<{*1+555 *1+444 *1+7000 }>'),
   ('1', 'Try reverse func syntax', r'7000:>[_ ({555, 444, [_] := %\\\})]', r'<{*1+555 *1+444 *1+7000 }>'),
   ('1', 'Parse 4 three char strings', '*4 +{*3+$|...} == +"catHatDogPig"', '<{"cat" "Hat" "Dog" "Pig" }>'),
   ('1', 'test nested references', r'{1 2 {"hi" "there"} 4 [$ $] := [_ _ {...}] :== %\ 6}', '<{*1+1 *1+2 {"hi" "there" } *1+4 "there" *1+6 }>'),
   ('1', 'Addition', '+(+3+7)', '<*1+10>'),
   ('1', 'Addition with references', r'{4, 6, ([_] := %\\ [_, _] := %\\ )}', '<{*1+4 *1+6 *1+10 }>'),
   ('1', 'Addition with reverse references', r'{4, 6, (%\\:[_] %\\:[_, _] )}', '<{*1+4 *1+6 *1+10 }>'),
   ('1', 'A two argument function', r'[+{_, _} +{[+_]:=%\\ [+_ +_]:=%\\  [+_]:=%\\} ]<:+{9,4}', '<{*1+9 *1+4 *1+9 }>'),
   ('2', 'define and use a tag', '{&color=#{*_+_ *_+_ *_+_} &size=#*_+_}', '<{}>', 'color', '#{_, _, _, }'),
   ('2', "nested empty tags", r'{&frame = {?|...} &portal = {frame|...}}', '<{}>', 'portal=*4+{frame|...}', '{{...} {...} {...} {...} }'),
   ('2', 'Two argument function defined with a tag', r'+{&func={+{_, _} +{%\\:[_] %\\:[_, _]  %\\:[_]}}  }', '<{}>', 'func<: +{9,4}', '{*1+9 *1+4 *1+9 }'),
   ('1', 'Simple parsing', '{*_ +{"A"|...} "AARON"} ==  \'AAAARON\' // This is a comment', '<{{"A" "A" } "AARON" }>'),
   ('1', 'Parse & select option 2', r'[...]="ARONdacks" :== {"AARON" "ARON"} ', '<"ARON">'),
   ('1', 'Parse & select option 1', r'[...]="AARONdacks" :== {"AARON" "ARON"} ', '<"AARON">'),
   ('1', 'Two item parse', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "ARONdacks"', '<{"ARON" "dac" }>'),
   ('1', 'Two item parse; error 1', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "ARONjacks"', '<ERROR>'), #NEXT-TASK // No dac, only jac
   ('1', 'Two item parse; error 2', r'{[...] :== {"AARON" "BOBO" "ARON" "AAAROM"}   "dac"} ==  "slapjacks"', '<ERROR>'), #NEXT-TASK // slap doesn't match.
   ('1', 'Two item parse, get first option', r'{[...] :== {"ARON" "BOBO" "AARON" "CeCe"}   "dac"} ==  "ARONdacks"', '<{"ARON" "dac" }>'),
   #('1', 'int and strings in function comprehensions', r'{[ ? {555, 444, \\[?]}]<:{"slothe", "Hello", "bob", 65432}|...}', '<{ | {*1+555 *1+444 "slothe" } {*1+555 *1+444 "Hello" } {*1+555 *1+444 "bob" } {*1+555 *1+444 *1+65432 } }>'),  #FAIL
   # Add the above test but with a list in the comprehension yeild.
   #('1', "Adding prep for 'reduce'", r'{[ ? {%\\:[?] (%\\:[?] *1+22)}]<: {*1+5 *2+7 *3+9 *4+13}|...}', '<{ | {*1+5 *1+27 } {*2+7 *2+29 } {*3+9 *3+31 } {*4+13 *4+35 } }>'),

    ('1', 'Simple Parsing', r'{*3+$|...}=="CatHatBatDog" ','{"Cat" "Hat" "Bat" "Dog" }'),
    ('1', 'Inner parsing 1', r'{ {*3+$}|...}="CatHatBatDog" ','{{"Cat" } {"Hat" } {"Bat" } {"Dog" } }'),

    ('1', "fromHere indexing string 1", "{111, '222' %^:[_, _, $] 444, '555', 666, {'hi'}}", '<{*1+111 "222" "555" *1+444 "555" *1+666 {"hi" } }>'),
    ('1', "fromHere indexing string 2", "{111, 222, %^:*3+[...] 444, 555, 666, {'hi'}}", '<{*1+111 *1+222 *1+555 *1+444 *1+555 *1+666 {"hi" } }> '),
    ('1', "fromHere indexing negative", "{111, 222, %^:/3+[...] 444, 555, 666, 777}", '<{*1+111 *1+222 *1+777 *1+444 *1+555 *1+666 *1+777 }> '),

    ('1', "Test lists with simple associations", r'{ {5, 7, 3, 8} {%\\:[_]~ | ...}}', r'<{{*1+5 *1+7 *1+3 *1+8 } {*1+5 *1+7 *1+3 *1+8 } }> '),
    ('1', "Test internal associations", r'{ {5, 7, 3, 8} ({0} {+(%\\\\:[_]~ %\\\:[_]~) | ...})}', r'<{{*1+5 *1+7 *1+3 *1+8 } ({*1+0 } {*1+5 *1+12 *1+15 *1+23 } ) }>'), # FAIL: Fails when small ListBufCutOff is used.
    ('1', "Test sequential func argument passing", r'{ {5, 7, 3, 8} {addOne<:(%\\\:[_]~) | ...}}', '<{{*1+5 *1+7 *1+3 *1+8 } {*1+6 *1+8 *1+4 *1+9 } }>'),

    ('1', "Select 2nd item from list", r'*2+[...] := {8 7 6 5 4 3}', '*1+7'),
    ('2', "Select item by concept tag: 'third item of ...'", r'{&thirdItem=*3+[...]}', '<{}>', 'thirdItem := {8 7 6 5 4 3}', '*1+6'),

    ('1', "Test simple filtering", r'{[_ _]|...} ::= {8 7 6 5 4 3}', '<{*1+7 *1+5 *1+3 }>'),
    ('2', "filtering with a concept-tag", r'{&everyOther={*2+[...]|...}}', '<{}>', 'everyOther ::= {8 7 6 5 4 3}', '{*1+7 *1+5 *1+3 }'),
    ('1', 'Filtering with a list', "{[? ?]|...} ::= {111, '222', '333', 444, {'hi'}, {'a', 'b', 'c'}}", '<{"222" *1+444 {"a" "b" "c" } }>'), #FAIL: {"222" *1+444 0; "b" 0; }

    ('1', "Test internal find-&-write", r'{4 5 _ 7} =: [_ _ 6]', '<{*1+4 *1+5 *1+6 *1+7 }>'),
    ('1', "Test external find-&-write", r'{4 5 _ 7} =: ([???]=6)', '<{*1+4 *1+5 *1+6 *1+7 }>'),
#***  ('2', "Test tagged find-&-write", r'{%setTo6=([???]=6)}', '{; }', r'{4 5 _ 7} =: setTo6', '<{*1+4 *1+5 *1+6 *1+7 }>')

#***  ('2', "Find nth item based on tag", r'{%testTag=[{?|...} <TAG>]}', '<{; }>', 'testTag := {8 7 6 5 4 3}', '*1+7'),
#***  (find all of <tag>): filtering with a tag as descriptor: {[_ TAG]|...} ::= {8 7 6 5 4 3}

    # TEST: Find an item based on 2+ tags (big red bike)
    # TEST: Complex Repeats // These work but have no test.

    # TEST: [5 _ <sum-of-those-two>] :=: ([_ _]=6)   ==> 11
    # TEST: Like the previous but tag :=: tag
    # TEST:        xxx =:: yyy
    # TEST:        xxx =:: tag
   ]

   for t in testsLst:
      #print t;
      if(t[0]=='0'):
         yield (ChkParser, t)
      elif(t[0]=='1'):
         yield (ChkNorm, t)
      elif(t[0]=='2'):
         yield (ChkWorld, t)


def ChkParser(t):
   try:
       print "PARSE TEST:", t[1]
       child=pexpect.spawn('./ptest',timeout=2);
       print "SPAWNED..."
       child.send('0\n')
       child.sendline(t[1])
       child.send('<%\n')
       child.send(t[2])
       child.send('\n%>\n')
       child.expect(r'Parsing \[<%.*?%>\]\s*');
       child.expect(r'Parsed\.\s*');
       print "Looking for ",t[3]
       child.expect_exact(t[3])
       #assert child.after==t[3]
       print "Found: ", child.after;
       child.send ("5\ndone\n!\n")
       pass
   finally:
       print "Found: ", child.after;
       assert child.after==t[3]

def ChkNorm(t):
   print "NORM TEST:", t[1],'   ',t[2]
   try:
       child=pexpect.spawn('./ptest',timeout=4);
       #print "N1"
       child.send('1\n')
       child.sendline(t[1])
       child.send('<%\n');  child.send(t[2]); child.send('\n%>\n'); #print "N2";
       child.expect(r'Parsing\s*\[<%\s*'); child.expect_exact(t[2]); child.expect(r'\s*%>\]\s*');  #print "N3";
       child.expect(r'\s*Parsed.\s*'); #print  "N4";
       child.expect_exact('Norming World...');  #print "N5";
       child.expect(r'\s*Normed\s*'); #print "N6";
       print "Looking For:",t[3]; #print "N7"; #print "Found: ", child.after;      print "N9";
       try:
           child.expect_exact(t[3]); # print "N8";
       except:
           print ""; #str(child);  # uncomment for more details.
       print "Found: ", child.after;     # print "N9";
       assert child.after==t[3];   #print "N10";

       child.send ("5\ndone\n!\n")
       pass
   finally:
       assert child.after==t[3]
     #  child.expect( pexpect.EOF )

def ChkWorld(t):
   print "WORLD TEST:", t[1]
   try:
       child=pexpect.spawn('./ptest',timeout=4);
       #print "W1"
       child.send('2\n')
       child.sendline(t[1])
       child.send('<%\n');  child.send(t[2]); child.send('\n%>\n'); # print "W2a";
       child.send('<%\n');  child.send(t[4]); child.send('\n%>\n'); # print "W2b";
       child.expect(r'Parsing\s*\[<%\s*'); child.expect_exact(t[2]); child.expect(r'\s*%>\]\s*'); # print "W3";
       child.expect(r'\s*Parsed.\s*'); #print  "W4";
       child.expect_exact('Norming World...');   child.expect(r'\s*Normed\s*');# print "W5";

       print "Looking For ",t[3]
       child.expect_exact(t[3]);
       assert child.after==t[3]
       print "Found: ", child.after;

       child.expect(r'\s*Parsing query\s*\[\s*<%\s*'); child.expect_exact(t[4]); child.expect(r'\s*%>\s*\]\s*');  #print "W6";
       child.expect(r'\s*parsed.\s*<<\['); child.expect(r'.*?'); child.expect(r'\s*\]>>\s*'); #print  "W7";
       child.expect_exact('Norming query...');   child.expect(r'\s*Normed\s*'); #print "W8";

       print "Looking for:",t[5]
       child.expect_exact(t[5])
       print "Found: ", child.after;
       assert child.after==t[5]

       child.send ("5\ndone\n!\n")
       pass
   finally:
       assert child.after==t[5]


