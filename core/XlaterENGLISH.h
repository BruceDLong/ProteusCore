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
#include <memory>
#include <cstdarg>

using namespace std;

struct bRule{
    string head;
    vector<string> rhs;

    bRule(char* ruleHead, int n, ...);
    string asString();
    friend bool operator==(bRule const& left, bRule const& right) {
            return (left.head == right.head) && (left.rhs.size()==right.rhs.size())
                && (std::equal(left.rhs.begin(), left.rhs.end(), right.rhs.begin()));
        }
};

typedef shared_ptr<bRule> bRulePtr;
typedef multimap<string, bRulePtr> Rules;
typedef Rules::iterator RuleItr;
typedef pair<RuleItr,RuleItr> RuleRange;

struct Grammar{
    Rules rules;     // Grammar rules
    Rules dynRules;  // Dynamically created rules
    bRulePtr addRule(bool mainLib, int n, ...);
    void loadRules();
};

struct WordChain;

class XlaterENGLISH:public xlater{
public:
    WordS* ReadLanguageWord(QParser *parser, icu::Locale &language); // Reads a 'word' consisting of a number or an alphabetic+hyphen+appostrophies tag
    void ReadTagChain(QParser *parser, icu::Locale &language, WordS& result); // Reads a phrase that ends at a non-matching character or a period that isn't in a number.
    infon* tags2Proteus(WordS& tags);     // Converts a list of tags read by ReadTagChain() into an infon and returns a pointer to it.
    void proteus2Tags(infon* proteus, WordS& WordsOut);  // Converts an infon to a tag chain.
    virtual bool loadLanguageData(sqlite3 *db);
    virtual bool unloadLanguageData();
    XlaterENGLISH(){localeID="en"; language.createCanonical(localeID.c_str());};
    ~XlaterENGLISH(){unloadLanguageData();};

private:
    Grammar EnglishGrammarRules;
    void findDefinitions(WordS& tags);
    void stitchAndDereference(WordS& tags);
    infon* infonate(WordS& tags);
};

#endif
