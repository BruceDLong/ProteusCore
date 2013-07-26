/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2012 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "wordLibrary.h"
#include "xlater.h"
#include <sys/stat.h>
#include <sstream>
#include <iostream>
#include <fstream>

WordLibrary::WordLibrary(sqlite3 *DB){
    const char *tail;
    db=DB;
    query="select locale,word,senseID,pos,gloss from words WHERE locale like ?1 || '%' AND (word = ?2 OR word like ?2 || ' %') order by word,senseID;";
    int err = sqlite3_prepare_v2(db, query.c_str(), -1, &res, &tail);
    if (err != SQLITE_OK) {cout<<"Error in query:"<<query<<"\n";}
}

WordSMap::iterator WordLibrary::wrappedLowerBound(wordKey &word, xlater* xlatr){
//cout<<"LOWER_BOUND:'"<<word<<"'\n";
    // If (crntWrd not from the database) load it and all like "it %"
    WordSMap::iterator trialItr=lower_bound(word);
    if(trialItr != end()){
        if(trialItr->second->flags1 & wfIsFromDB) { return trialItr;}
        else trialItr->second->flags1 |= wfIsFromDB;
    }
//return trialItr; // Uncomment this line to deactivate database.
    // Add any words found in the database that match.
    if(sqlite3_bind_text(res, 1, xlatr->localeID.c_str(), -1, SQLITE_TRANSIENT)
    || sqlite3_bind_text(res, 2, word.c_str(), -1, SQLITE_TRANSIENT))
        {cout<<"    Error binding '"<<word.c_str()<<"' to query\n"; return trialItr;}
    string locale, resWord, senseID, pos, gloss;
    while (sqlite3_step(res) == SQLITE_ROW) {
        locale  = (char*)sqlite3_column_text(res, 0);
        resWord = (char*)sqlite3_column_text(res, 1);
        senseID = (char*)sqlite3_column_text(res, 2);
        pos     = (char*)sqlite3_column_text(res, 3);
        gloss   = (char*)sqlite3_column_text(res, 4);

        wordKey wKey=resWord+"%U";
        uint POS=0;
        if     (pos=="noun") POS=wfNoun;
        else if(pos=="verb") POS=wfVerb;
        else if(pos=="adj") POS=wfAdj;
        else if(pos=="adv") POS=wfAdv;
        else if(pos=="func") POS=wfMiscFunc;
        WordSPtr wordPtr=WordSPtr(new WordS(wKey, 0, 0, xlatr));
        wordPtr->flags2=POS;
        wordPtr->flags1|=wfIsFromDB;
        wordPtr->senseID=senseID;
        wordPtr->attributes.insert(pair<string,string>("gloss",gloss));
        insert(pair<wordKey, WordSPtr>(wKey, wordPtr));
cout<<"INSERTING:"<<resWord<<"  "<<wKey<<"   "<<pos<<POS<<"  ("<<gloss<<")\n";
    }
    sqlite3_reset(res);
    return lower_bound(word);
}

WordSPtr WordLibrary::chkExists(wordKey &word, string &senseID){
    WordSRange nextWordS=equal_range(word);
    if(nextWordS.first == nextWordS.second) return 0;
    for(WordSItr wi=nextWordS.first; wi!=nextWordS.second; ++wi) {
        if((*wi).second->senseID==senseID) return (*wi).second;
    }
    return 0;
}

WordSPtr WordLibrary::insertWord(){
    return 0;
}

// Repository functions
extern int do_clone(const char* url, const char* path);

InfonSource::InfonSource(string srcRoot, string SourceSpec, uint interval):sourceSpec(SourceSpec){
	// SourceSpec format like: git://github.com/BruceDLong/NewsTest.git:World.pr
	if(srcRoot!=""){ // Using as already vetted source tree
		if(sourceSpec.find("..")!=string::npos){errorDesc="Invalid path to file"; errorState=1; return;}
		filePath=sourceSpec;
		URI_path=srcRoot;
		srcType="file";
	} else {	
		int pos1=sourceSpec.find("://");
		srcType=sourceSpec.substr(0,pos1);
		if (srcType!="git" && srcType!="file" && srcType!="string"){errorDesc="Invalid source type: "+srcType; errorState=1; return;}
		
		pos1+=3;
		int pos2=sourceSpec.find(":", pos1);
		URI_path=sourceSpec.substr(pos1,pos2-pos1);
		if(URI_path.find("..")!=string::npos){errorDesc="Invalid repository spec"; errorState=1; return;}
		
		filePath=sourceSpec.substr(pos2+1);
		if(srcType!="string" && filePath.find("..")!=string::npos){errorDesc="Invalid path to file"; errorState=1; return;}
	}
	relativePath=URI_path+'/'+filePath;
	URI=srcType+"://"+URI_path;
	sourceID=""; // hash name for this source
	crntHash="";
	assertionCode_pr="";
	sub_sources="";
	lastUpdate=0;
	updateInterval=interval;  // in seconds.
	errorState=0;   // 0=OK
	errorDesc="";
}

istream* InfonManager::cachedStream(string srcRoot, string srcSpec, bool &doCache){
	doCache=false;
	InfonSourcePtr infSrc(new InfonSource(srcRoot, srcSpec));
	if(infSrc->errorState) {cout<<"\n"<<infSrc->errorDesc<<"\n"; return 0;}
	string repoDir=dataFolder+'/'+infSrc->URI_path;
	sources[infSrc->sourceSpec] = infSrc;	
	if(infSrc->srcType=="git"){
		struct stat buffer; int rc;
		if(stat(repoDir.c_str(), &buffer)==-1){
			cout<<"Cloning "<<infSrc->URI<<" into "<<repoDir<<"\n";
			rc=do_clone(infSrc->URI.c_str(), repoDir.c_str());
			if(rc!=0){return 0;}
		} else { // TODO: update the repository here.
	//			if(it's time to update this repo)
		}
		// if (cache for this stream exists) {
			// Get hash codes of git version and cached version.
			// if(crnt Git hash == cashed hash){
				// append fileSpec to activate cached version
				// fetch code.pr from db
				// return new istrstream(code_pr.c_str());
			// } else {doCache=true; delete items from cache} // later just modify the changed items.
		// } else doCache=true;
		return new fstream(string(repoDir+'/'+infSrc->filePath).c_str());
	} else if(infSrc->srcType=="string"){ return new istringstream(infSrc->filePath.c_str());
	} else if(infSrc->srcType=="file")  { return new fstream(string(repoDir+'/'+infSrc->filePath).c_str());
	} else if(infSrc->srcType=="stdin"){ cout<<"stdin unsupported\n"; return 0;
	} else if(infSrc->srcType=="https"){ cout<<"https unsupported\n"; return 0;
	}
	return 0;
}

void InfonManager::updateRepositories(){
//	for (each repo){
//		update
//	}
}
