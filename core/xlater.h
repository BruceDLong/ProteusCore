///////////////////////////////////////////////////
//  xlater.h 1.0  Copyright (c) 2012 Bruce Long
//  xlate is a base class for language extensions to Proteus
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _xlater
#define _xlater

#include <map>
#include <string.h>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/putil.h>
#include <unicode/normalizer2.h>
#include <boost/intrusive_ptr.hpp>
#include "remiss.h"

using namespace std;

#define streamPut(nChars) {for(int n=nChars; n>0; --n){stream.putback(textParsed[textParsed.size()-1]); textParsed.resize(textParsed.size()-1);}}
#define ChkNEOFs(stream) {if(stream.eof() || stream.fail()) throw "Unexpected End of file";}
#define getBufs(c,parser) {ChkNEOFs(parser->stream); int p=0; for(;(c);parser->buf[p++]=parser->streamGet()){if (p>=bufmax) throw "String Overflow";} parser->buf[p]=0;}
#define check(ch) {RmvWSC(); ChkNEOF; tok=streamGet(); if(tok != ch) {cout<<"Expected "<<ch<<"\n"; throw "Unexpected character";}}

struct WordS;
typedef boost::intrusive_ptr<WordS> WordSPtr;
struct infon;
struct QParser;

typedef string wordKey;
typedef map<wordKey, WordSPtr> WordSMap;

/* The class xlater is used to translate a (possibly) natural language to and from infons.
 * Subclass xlater for each language. Languages are identified by locale identifiers.
*/

class xlater{
public:

    xlater(){language.createCanonical("");} //locale("").name().c_str());;}

    icu::Locale language;   // In constructor, implementations set this to the locale ID (e.g., en or jp) for the language being provided.
    WordSMap wordLibrary;   // Here is where the tags to models for this language are stored

    //////////////////////////////////////////////////////////
    /* ReadLanguageWord() extends a QParser to read a 'word' in some language (possibly a natural language).
     *  Words differ from simple Proteus tags in language defined ways. For example:
     *      * Proteus tags are in Unicode.  Words here may restrict characters to those from the language being read.
     *      * Proteus tags cannot contain appostrophies or hyphens while words may be defined to to have them.
     *      * Proteus tags are case insensitive. But here case could be used to mark sentence beginnings etc.
     *      * "Words" could also be 'numbers' such as in "50 apples can't be left on Bob's mother-in-law's tree's branches."
     *  If no word is read from the stream the stream must be left intact so that parsing can continue.
     *  This function works when loading an infon and won't have access to the tag library.
     */

    virtual WordSPtr ReadLanguageWord(QParser *parser, icu::Locale &locale)=0;

    //////////////////////////////////////////////////////////
    /* ReadTagChain() extends a QParser to read a phrase in some language (possibly a natural language).
     *   It likely calls ReadWord() to read a chain of 'words' in the langauge in question.
     *   ReadTagChain may also read punctuation.
     *   ReadTagChain must leave the stream in the correct place on return. (Either having read nothing or an entire phrase but no more.)
     *   This function works when loading an infon and won't have access to the tag library.
     */

    virtual WordSPtr ReadTagChain(QParser *parser, icu::Locale &locale)=0;

    //////////////////////////////////////////////////////////
    /* Tags2Proteus() converts a list of tags read by ReadTagChain() into an infon and returns a pointer to it.
     */

    virtual infon* tags2Proteus(WordSPtr tags)=0;
    virtual WordSPtr proteus2Tags(infon* proteus)=0;


    // Implementations initialize the language (i.e., load any lists or structures) in loadLanguageData(). Free them in unloadLanguageData();
    virtual bool loadLanguageData(string dataFilename)=0;
    virtual bool unloadLanguageData()=0;
    virtual ~xlater(){}; // Implementeations should call unloadLanguageData()
};

#endif
