/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2012 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _InfonLibrary
#define _InfonLibrary

#include <map>
#include <list>

#include <iostream>
#include <string>
#include <stddef.h>
#include <unicode/locid.h>
#include <unicode/unistr.h>
#include <unicode/putil.h>
#include <unicode/ustream.h>
#include <boost/intrusive_ptr.hpp>
#include <sqlite3.h>

using namespace std;

struct infon;

///////////////////  TAG RELATED ITEMS  ///////////////////

enum WordSystemTypes {
    wstUnknown, wstUnparsed, wstAlternates,
    wstFunctionWord,                    // A function word for the given language.
    wstFullDescription,                 // A complete description: selects an item and constrains or defines its states.
    wstDescriptionalSelector,           // Narrow possibilities using a description
    wstNounSelector,                    // Narrow possibilities by constraining existing state (* __)
    wstVerbSelector,                    // Narrow possibilities in a sequence
    wstAdjSelector,                     // Narrow possibiites by the value of a state (+ __)
    wstAdvSelector,                     // Misc selectors.
    wstRelationalSelector,              // Narrow possibilities by relationship to another item.
    wstNumOrd, wstNumCard, wstNumNominal, wstNumInfonic,
    wstLastEnum         // Language modules can use this to extend the types of word-systems.
};

enum DescriptionTypes {
    dtMonation_shortForm,       // Sara laughed
    dtMonation,                 // Sara is laughing
    dtRelation,                 // Ryan dyed the shirt
    dtRelation_constraint,      // Ryan dyed the shirt blue
    dtTrilation                 // Ryan gave Sara the shirt
};

enum WordDegree {dComparative, dSuperlative};  // more, most
enum WordPositionStyle {sBeforeNoun, sAfterBoBo};
enum WordFlags1 {
        maskForm=0x03, wfForm_S_ES=1, wfForm_ING=2, wfForm_ED=3,
        maskGndr=0x0c, wfGndrMasc=0x04, wfGndrFem=0x08, wfGndrNeut=0x0c,
        maskPrsn=0x30, wf1stPrsn=0x10, wf2ndPrsn=0x20, wf3rdPrsn=0x30,
        wfErrorInAChildWord=0x40, wfWasHyphenated=0x80, wfAsPrefix=0x100, wfAsSuffix=0x200, wfIsFromDB=0x400,
        wfHasVerbSense=0x800, wfHasNounSense=0x1000, wfHasAdjSense=0x2000, wfHasAdvSense=0x4000, wfHasDetSense=0x8000,
        wfHasNumSense=0x10000, wfCanStartNum=0x20000, wfHasPrepositionSense=0x40000, wfHasIntesifierSense=0x80000,
        wfIsGradable=0x100000, wfIsMarkedPossessive=0x200000, wfIsPronoun=0x400000, wfVerbHelper=0x800000,
        wfIsPossessive=0x1000000, wfIsPlural=0x2000000, wfIsProperNoun=0x4000000, wfIsCountable=0x8000000,
        };

enum WordFlags2 {
        maskWordClass=0xf, wfUnknown=0x0, wfMiscFunc=0x1, wfNoun=0x2, wfVerb=0x3, wfAdj=0x4, wfAdv=0x5,
                           wfPrep=0x6,  wfConj=0x7, wfNegotiater=0x8
        };

struct WordS;

typedef boost::intrusive_ptr<WordS> WordSPtr;
typedef list<WordSPtr> WordList;
typedef WordList::iterator WordListItr;
typedef string wordKey;

typedef multimap<wordKey, WordSPtr> WordSMap;
typedef WordSMap::iterator WordSItr;
typedef pair<WordSItr,WordSItr> WordSRange;
class xlater;

struct WordS {  // Word System: single number, word or phrase / clause / sentence.
    string asRead;        // The word exactly as it was read in.
    string norm;          // The normalized form of the word.
    string baseForm;      // The word without inflection. (e.g., ing, ed, s, est, er are removed)
    string locale;        // The locale used for this word: e.g., "en" or "en_us"
    string senseID;       // A tag identifying the sense of the word. Either like #n#3 or a synonym or gloss-word.
    infon *definition;    // The definition/model of this word.
    map<string, string> attributes; // Attributes including pronunciation, word properties and model/author history.
    xlater *xLater;       // The translation module for this word / word system.
    wordKey key;          // The key into a library map.

    int offsetInSource;  // position of this word in the sourceStr.

    WordList words;      // The list of words before and during syntatic and semantic analysis.
    WordSystemTypes sysType;  // What kind of system? number, noun phrase, sentence, etc.
    WordSPtr item, itemsConstraints, metaConstraints; // After analysis: "head", "pre-modifiers", "post-modifiers"

    uint flags1, flags2;      // See Word Flags enum for bit meanings
    int rank;             // negotiator > verb > noun > adj (opinion, size, shape, condition, age, color, origin) > adv > det > pp

    WordDegree wordDegree;
    WordPositionStyle PositionStyle;

    WordList altDefs;    // Other possible meanings for this system.

    WordS(string tag="", int flags=0, infon* def=0, xlater *Xlater=0){
        asRead=tag; norm=tag; key=""; senseID="";
        definition=def; xLater=Xlater; flags1=flags; flags2=0; sysType=wstUnparsed; offsetInSource=0;
        item = itemsConstraints = metaConstraints = 0; refCnt=0;
    }
    ~WordS();
    uint refCnt;
};

inline void intrusive_ptr_add_ref(WordS* ws){++ws->refCnt;}
inline void intrusive_ptr_release(WordS* ws){if(--ws->refCnt == 0) delete ws;}

struct WordLibrary:WordSMap {
    sqlite3 *db;
    sqlite3_stmt *res;
    string query;
    WordSMap::iterator wrappedLowerBound(wordKey &word, xlater* xlatr);
    WordSPtr chkExists(wordKey &word, string &senseID);
    WordSPtr insertWord();
    WordLibrary(sqlite3 *DB);
    ~WordLibrary(){sqlite3_finalize(res);};
};

struct InfonSource{
	InfonSource(string SourceSpec, uint interval=30*60);
	istream *stream();
	string sourceSpec; // URI + file path.
	string sourceID; // hash name for this source
	string srcType;  // e.g. git
	string filePath;
	string relativePath; // Path to the file from dataDir/news/
	string URI, URI_path;
	string crntHash;
	string assertionCode_pr;
	string sub_sources;
	time_t lastUpdate;
	uint   updateInterval;  // in seconds.
	uint   errorState;      // 0=OK
	string errorDesc;       // If errorState is not OK, this describes why.
	uint refcnt;
};

typedef boost::intrusive_ptr<InfonSource> InfonSourcePtr;
inline void intrusive_ptr_add_ref(InfonSource* p){++p->refcnt;}
inline void intrusive_ptr_release(InfonSource* p){if(--p->refcnt == 0) delete p;}

struct RepoStatus{
	string name, URI, crntHash, repoType;
	int updateInterval;
	uint errorState;   // 0=OK
};

struct InfonManager{
	sqlite3 *db;       // The cache database.
	map<string, RepoStatus> repos;
	map<string, InfonSourcePtr> sources;
	string dataFolder;
	string activeSrcList;
	uint errorState;   // 0=OK
	
	InfonManager(string DataFolder, sqlite3 *DB):db(DB), dataFolder(DataFolder){};
	istream* cachedStream(string srcSpec, bool &doCache);    // Fetches an infon stream from either git/file or the cache
	void updateRepositories();
};


#endif
