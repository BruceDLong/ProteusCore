///////////////////////////////////////////////////
//  XlaterENGLISH.h 1.0  Copyright (c) 2012 Bruce Long
//  xlaterENGLISH is an English language extension to Proteus
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _XlaterENGLISH
#define _XlaterENGLISH

#include "xlater.h"
#include "Proteus.h"

using namespace std;

struct WordChain;

class XlaterENGLISH:public xlater{
public:
    WordS* ReadLanguageWord(QParser *parser, icu::Locale &language); // Reads a 'word' consisting of a number or an alphabetic+hyphen+appostrophies tag
    WordS* ReadTagChain(QParser *parser, icu::Locale &language); // Reads a phrase that ends at a non-matching character or a period that isn't in a number.
    infon* tags2Proteus(WordS* tags);     // Converts a list of tags read by ReadTagChain() into an infon and returns a pointer to it.
    WordS* proteus2Tags(infon* proteus);  // Converts an infon to a tag chain.
    virtual bool loadLanguageData(string dataFilename);
    virtual bool unloadLanguageData();
    ~XlaterENGLISH(){unloadLanguageData();};

private:
    void findDefinitions(WordS *tags);
    void stitchAndDereference(WordS *tags);
    infon* infonate(WordS *tags);
};

#endif
