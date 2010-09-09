import re
import pexpect

#def setup_func():
#   child=pexpect.spawn('../proteus');
#
#def teardown_func():
#   child.send "5\ndone\n!\n";

def test_engine():
  #  inf=open('../tests.pr')
  #  testsStr=inf.read()
  #  testsLst= re.findall(r'^\s*(\d)\s+([\S ]*)\s*<%\s*(.*?)\s*%>', testsStr,re.M);

   testsLst=[
   ('0', 'parse a number', '*2589+654321',r'<*2589+654321>'),
   ('0', 'Parse a string', '+"HELLO"', '<"HELLO">'),
   ('0', 'parse a list', '+{123, 456, "Hello there!", {789, 321}}', '<{*1+123 *1+456 "Hello there!" {*1+789 *1+321 } }>'),
   ('0', 'parse nested empty lists', '+{{},{}}', '<{{} {} }>'),
   ('1', 'test list/string', '+{*3+$ *4+$}=+"CatDogs"', '<{"Cat" "Dogs" }>'),
   ('1', 'test anon functions', '[456,789,]:+123', '<*1+789>'),
   ('1', 'Try a bigger function', r'[+_ ({555, 444, \\[_]},)]: +700000000', r'<({*1+555 *1+444 *1+700000000 } )>'),
   ('1', 'test rep$', '*4 +{*3+$|...} = +"catHatDogPig"', '<{"cat" "Hat" "Dog" "Pig" }>'),
   ('1', 'Addition', '+(+3+7)', '<*2+10>'),
   ('1', 'A two argument function', '[+{_, _} +{\\[+_] \\[+_ +_]  \\[+_]} ]:+{9,4}', '<{*1+9 *1+4 *1+9 }>'),
   ('2', 'define and use a tag', '+{color=@+{*_+_ *_+_ *_+_} size=@*_+_}', '<{color   size   }>', 'color', '@{_, _, _, }'),
   ('2', 'Two argument function defined with a tag', '+{func={+{_, _} +{\\[_] \\[_, _]  \\[_]}}  }', '<{func   }>', 'func: +{9,4}', '{*1+9 *1+4 *1+9 }'),
   ('1', 'test rep$', '{*_ +{"A"|...} "AARON"} =  +\'AAAARON\' // This is a comment', '<{{"A" "A" } "AARON" }>'),
   ('1', 'test indexing', '%{111, 222, 333, 444}*2+[...]', '<*1+222>'),
   ('1', 'Indexing, unknown index 1', r'%{"AARON", "ARON"}*_+[...]  =  +"ARONdacks" ', '<"ARON">'),
   ('1', 'Indexing, unknown index 2', r'%{"AARON", "ARON"}*_+[...]  =  +"AARONdacks" ', '<"AARON">'),
 #  ('1', 'Indexing, unknown index 3', r'{%{"AARON", "ARON"}*_+[...]  "dac"}  =  +"ARONdacks" ', '<{"ARON" "dac" }>'),
   ('1', 'int and strings in function comprehensions', r'{[ ? {555, 444, \[?]}]: {"slothe", "Hello", "bob", 65432}|...}', '<{ | {*1+555 *1+444 "slothe" } {*1+555 *1+444 "Hello" } {*1+555 *1+444 "bob" } {*1+555 *1+444 *1+65432 } }>'),
# The above test but with a list in the comprehension yeild.     ALSO, run through this whole thing to make sure it isn't doing things too many times.
   ('1', 'test 1 of repeated indexing (i.e., filtering)', "{%{111, '222', '333', 444, {'hi'}, {'a', 'b', 'c'}}*2+[...]|...}", '<{"222" *1+444 {"a" "b" "c" } }>')
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
       child=pexpect.spawn('./proteus',timeout=2);
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
       assert child.after==t[3]
       print "Found: ", child.after;
       child.send ("5\ndone\n!\n")
       pass
   finally:
       assert child.after==t[3]

def ChkNorm(t):
   print "NORM TEST:", t[1],'   ',t[2]
   try:
       child=pexpect.spawn('./proteus',timeout=4);
       print "N1"
       child.send('1\n')
       child.sendline(t[1])
       child.send('<%\n');  child.send(t[2]); child.send('\n%>\n'); print "N2";
       child.expect(r'Parsing\s*\[<%\s*'); child.expect_exact(t[2]); child.expect(r'\s*%>\]\s*');  print "N3";
       child.expect(r'\s*Parsed.\s*'); print  "N4";
       child.expect_exact('Norming World...');  print "N5";
       child.expect(r'\s*Normed\s*'); print "N6";
       print "Looking For:",t[3]; print "N7"; #print "Found: ", child.after;      print "N9";
       try:
           child.expect_exact(t[3]);  print "N8";
       except:
		   print str(child);
       print "Found: ", child.after;      print "N9";
       assert child.after==t[3];   print "N10";

       child.send ("5\ndone\n!\n")
       pass
   finally:
       assert child.after==t[3]
       child.expect( pexpect.EOF )

def ChkWorld(t):
   print "WORLD TEST:", t[1]
   try:
       child=pexpect.spawn('./proteus',timeout=4);
       print "W1"
       child.send('2\n')
       child.sendline(t[1])
       child.send('<%\n');  child.send(t[2]); child.send('\n%>\n'); print "W2a";
       child.send('<%\n');  child.send(t[4]); child.send('\n%>\n'); print "W2b";
       child.expect(r'Parsing\s*\[<%\s*'); child.expect_exact(t[2]); child.expect(r'\s*%>\]\s*');  print "W3";
       child.expect(r'\s*Parsed.\s*'); print  "W4";
       child.expect_exact('Norming World...');   child.expect(r'\s*Normed\s*'); print "W5";

       print "Looking For ",t[3]
       child.expect_exact(t[3]);
       assert child.after==t[3]
       print "Found: ", child.after;

       child.expect(r'\s*Parsing query\s*\[\s*<%\s*'); child.expect_exact(t[4]); child.expect(r'\s*%>\s*\]\s*');  print "W6";
       child.expect(r'\s*parsed.\s*<<\['); child.expect(r'.*?'); child.expect(r'\s*\]>>\s*'); print  "W7";
       child.expect_exact('Norming query...');   child.expect(r'\s*Normed\s*'); print "W8";

       print "Looking for:",t[5]
       child.expect_exact(t[5])
       print "Found: ", child.after;
       assert child.after==t[5]

       child.send ("5\ndone\n!\n")
       pass
   finally:
       assert child.after==t[5]


