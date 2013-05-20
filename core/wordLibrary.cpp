/////////////////////////////////////////////////////
// Proteus Parser 6.0  Copyright (c) 1997-2012 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "wordLibrary.h"
#include "xlater.h"

WordLibrary::WordLibrary(sqlite3 *DB){
    const char *tail;
    db=DB;
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
return trialItr; // TODO: remove this line to reactivate database.
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
