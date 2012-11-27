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

struct BRule{
    string head;
    vector<string> rhs;

    inline BRule(char* ruleHead, char* item1=0, char* item2=0, char* item3=0, char* item4=0){
        head=ruleHead;
        if(item1) rhs.push_back(item1);
        if(item2) rhs.push_back(item2);
        if(item3) rhs.push_back(item3);
        if(item4) rhs.push_back(item4);
    };
    inline friend bool operator==(BRule const& left, BRule const& right) { // TODO: use an ID to track matching rules.
        return (left.head == right.head) && (left.rhs.size() == right.rhs.size()) && (std::equal(left.rhs.begin(), left.rhs.end(), right.rhs.begin()));
        }
    friend inline std::ostream& operator<<(std::ostream& out, BRule const& r) {
            out << r.head << " --> [ ";
            for(vector<string>::const_iterator si=r.rhs.begin(); si != r.rhs.end(); ++si){
                out << (*si) <<" ";
            }
            out << "]";
            return out;
        }
};

typedef multimap<string, BRule> Rules;
typedef Rules::iterator RuleItr;
typedef pair<RuleItr,RuleItr> RuleRange;
struct Grammar{
    Rules rules;
};

struct WordChain;

class XlaterENGLISH:public xlater{
public:
    WordS* ReadLanguageWord(QParser *parser, icu::Locale &language); // Reads a 'word' consisting of a number or an alphabetic+hyphen+appostrophies tag
    void ReadTagChain(QParser *parser, icu::Locale &language, WordS& result); // Reads a phrase that ends at a non-matching character or a period that isn't in a number.
    infon* tags2Proteus(WordS& tags);     // Converts a list of tags read by ReadTagChain() into an infon and returns a pointer to it.
    void proteus2Tags(infon* proteus, WordS& WordsOut);  // Converts an infon to a tag chain.
    virtual bool loadLanguageData(string dataFilename);
    virtual bool unloadLanguageData();
    XlaterENGLISH(){language.createCanonical("en");};
    ~XlaterENGLISH(){unloadLanguageData();};

private:
    Grammar EnglishGrammarRules;
    void findDefinitions(WordS& tags);
    void stitchAndDereference(WordS& tags);
    infon* infonate(WordS& tags);
};

#endif
