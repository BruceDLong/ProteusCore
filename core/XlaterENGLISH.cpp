///////////////////////////////////////////////////
//  XlaterENGLISH.cpp 1.0  Copyright (c) 2012 Bruce Long
//  xlaterENGLISH is an English language extension to Proteus
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "XlaterENGLISH.h"
#include <unicode/rbnf.h>


const uint maxWordsInCompound=10;
UErrorCode uErr=U_ZERO_ERROR;
const icu::Normalizer2 *tagNormalizer=Normalizer2::getNFKCCasefoldInstance(uErr);

bool isEngWordChar(char ch){
    return (isalpha(ch) || ch=='-' || ch=='\'');
}

bool isCardinalOrOrdinal(char ch){
    return (isdigit(ch) || ch=='.'  || ch=='-' || ch=='s' || ch=='t' || ch=='h' || ch=='n' || ch=='r' || ch=='d');
}

bool validNumberSyntax(string* n){
   // if(n[0]!='-' && !isdigit(n[0])) return 1;
   return 0;
}

void tagChainToString(WordSPtr tags, UnicodeString &strOut){
    strOut="";
    for(WordListItr itr=tags->words.begin(); itr!=tags->words.end(); ++itr){
        if(strOut!="") strOut+=" ";
        (*itr)->sourceStr = &strOut;
        (*itr)->offsetInSource=strOut.length();
        strOut+= strOut.fromUTF8((*itr)->norm);
    }
}

WordSPtr XlaterENGLISH::ReadLanguageWord(QParser *parser, icu::Locale &locale){ // Reads a 'word' consisting of a number or an alphabetic+hyphen+appostrophies tag
    char tok=parser->Peek();
    if(isdigit(tok) || tok=='-') {getBufs(isCardinalOrOrdinal(parser->peek()),parser);}
    else if(isalpha(tok)){getBufs(isEngWordChar(parser->peek()),parser);}
    else return 0;
    if(parser->buf[0]==0) return 0;
    WordSPtr tag=WordSPtr(new WordS);
    tag->asRead=parser->buf; tag->locale=locale.getBaseName();
    if(isalpha(tok)){
        tagNormalizer->normalize(UnicodeString::fromUTF8(tag->asRead), uErr).toUTF8String(tag->norm);
        if(tagIsBad(tag->norm, tag->locale.c_str())) throw "Illegal tag being defined";
        if(tag->norm.at(0)=='-'){tag->wordFlags|=wfAsSuffix; tag->norm=tag->norm.substr(1);}
        if(tag->norm.at(tag->norm.length()-1)=='-'){tag->wordFlags|=wfAsPrefix; tag->norm=tag->norm.substr(0,tag->norm.length()-1);}
        if(tag->norm.find('-')!=string::npos) {tag->wordFlags|=wfWasHyphenated;}
    } else {
        tag->sysType=wstNumCard;
        if(!validNumberSyntax(&tag->asRead)) throw "Invalid number syntax";
    }
    return tag;
}

WordSPtr XlaterENGLISH::ReadTagChain(QParser *parser, icu::Locale &locale){ // Reads a phrase that ends at a non-matching character or a period that isn't in a number.
    WordSPtr head=0, nxtTag;
    while((nxtTag=ReadLanguageWord(parser, locale))){
        if(head==0) {head = WordSPtr(new WordS);} else head->norm+="-";
        head->words.push_back(nxtTag);
        head->norm+=nxtTag->norm;
    }
    parser->nxtTokN(1,".");
    return head;
}

infon* XlaterENGLISH::tags2Proteus(WordSPtr words){   // Converts a list of words read by ReadTagChain() into an infon and returns a pointer to it.
    findDefinitions(words);
    stitchAndDereference(words);
    infon* proteusCode = infonate(words);
    return proteusCode;
}

WordSPtr XlaterENGLISH::proteus2Tags(infon* proteus){  // Converts an infon to a tag chain.
    return 0;
}

typedef map<string, int> WordMap;
typedef WordMap::iterator WordMapIter;

enum funcWordFlags {wcContentWord=0, wcDeterminer=1, wcQuantifier=2, wcNumberStarter=8, wcVerbHelper=16, wcPronoun=32, wcPreposition=64, wcConjunction=128, wcNegotiator=256};
WordMap functionWords = {   // int is a WordClass
    // Definite determiners
        {"the", 1}, {"this", 1}, {"that", 1}, {"these", 1}, {"those", 1},   // These and Those are plural of this and that
        {"which", 1}, {"whichever", 1}, {"what", 1}, {"whatever", 1},       // These usually start questions
        // Other definite determiners are possessives inclusing possessive pronouns: my, your, his, her, its, our, their, whose.

    // Indefinite determiners
        {"a", 1}, {"an", 1},       // Use with singular, countable nouns.
        {"some", 1}, {"any", 1},   // Use with plural and non-countable nouns.

    // Quantifying determiners // I need to define a "quantifier phrase"
        {"much", 2}, {"many", 2}, {"more", 2}, {"most", 2}, {"little", 2}, {"few", 2}, {"less", 2}, {"fewer", 2}, {"least", 2}, {"fewest", 2},
        {"all", 2}, {"both", 2}, {"enough", 2}, {"no", 2},
        {"each", 2}, {"every", 2}, {"either", 2}, {"neither", 2},   // 'every' can be 'almost every' and some other forms.

        {"zero", 8}, {"one", 8}, {"two", 8}, {"three", 8}, {"four", 8}, {"five", 8}, {"six", 8}, {"seven", 8}, {"eight", 8}, {"nine", 8},
        {"ten", 8}, {"eleven", 8}, {"twelve", 8}, {"thirteen", 8}, {"fourteen", 8}, {"fifteen", 8}, {"sixteen", 8}, {"seventeen", 8}, {"eighteen", 8}, {"nineteen", 8},
        {"twenty", 8}, {"thirty", 8}, {"forty", 8}, {"fifty", 8}, {"sixty", 8}, {"seventy", 8}, {"eighty", 8}, {"ninety", 8},

        // when compound function words are supported:
        {"all the", 4}, {"some of the", 4}, {"some of the many", 4},  // also: "5 liters of"
        {"a pair of", 4}, {"half", 4}, {"half of", 4}, {"double", 4}, {"more than", 4}, {"less than", 4}, // twice as many as... ,  a dozen,
        {"a lot of", 4}, {"lots of", 4}, {"plenty of", 4}, {"a great deal of", 4}, {"tons of", 4}, {"a few", 4}, {"a little", 4},
        {"several", 4}, {"a couple", 4}, {"a number of", 4}, {"a whole boat load of", 4}, //... I need to find a pattern for these

    // Verb Helpers
        {"be", 16}, {"am", 16}, {"are", 16}, {"aren't", 16}, {"is", 16}, {"isn't", 16}, {"was", 16}, {"wasn't", 16}, {"were", 16}, {"weren't", 16}, {"being", 16}, {"been", 16},
        {"have", 16}, {"haven't", 16}, {"has", 16}, {"hasn't", 16}, {"had", 16}, {"hadn't", 16}, {"having", 16}, {"ain't", 16},
        {"do", 16}, {"don't", 16}, {"did", 16}, {"didn't", 16}, {"does", 16}, {"doesn't", 16}, {"doing", 16},
        {"can", 16}, {"can't", 16}, {"cannot", 16}, {"could", 16}, {"couldn't", 16}, {"should", 16}, {"shouldn't", 16}, {"would", 16}, {"wouldn't", 16},
        {"will", 16}, {"won't", 16}, {"shall", 16}, {"shan't", 16}, {"may", 16}, {"mayn't", 16}, {"might", 16}, {"mightn't", 16}, {"must", 16}, {"mustn't", 16},
        // consider: contracted is, has, am, are, have, had and will (e.g, he's, I've, I'll)
        // consider: better, had better, and various compound items

    // Pronouns
        {"I", 32},  {"me", 32},  {"my", 32},   {"mine", 32},  {"myself", 32},
        {"you", 32},            {"your", 32},  {"yours", 32}, {"yourself", 32},
        {"one", 32},                           {"one's", 32}, {"oneself", 32},
        {"he", 32}, {"him", 32}, {"his", 32},                {"himself", 32},
        {"she", 32},{"her", 32},              {"hers", 32},  {"herself", 32},
        {"it", 32},             {"its", 32},                {"itself", 32},

        {"we", 32}, {"us", 32},  {"our", 32},  {"ours", 32},  {"ourselves", 32},
                                                          {"yourselves", 32},
        {"they", 32},{"them", 32},{"their", 32},{"theirs", 32},{"themselves", 32},

        {"who", 32}, {"whom", 32},             {"whose", 32},

        // consider: this, that, these, and those can occur without a noun and thus act like pronouns.
        // consider: one in "I ate one"

    // Prepositions
    {"aboard", 64}, {"about", 64}, {"above", 64}, {"across", 64}, {"after", 64}, {"against", 64}, {"along", 64}, {"alongside", 64}, {"amid", 64}, {"amidst", 64},
    {"among", 64}, {"amongst", 64}, {"anti", 64}, {"around", 64}, {"as", 64}, {"astride", 64}, {"at", 64}, {"atop", 64}, {"bar", 64}, {"barring", 64},
    {"before", 64}, {"behind", 64}, {"below", 64}, {"beneath", 64}, {"beside", 64}, {"besides", 64}, {"between", 64}, {"beyond", 64}, {"but", 64}, {"by", 64},
    {"circa", 64}, {"concerning", 64}, {"considering", 64}, {"counting", 64}, {"despite", 64}, {"down", 64}, {"during", 64}, {"except", 64}, {"excepting", 64}, {"excluding", 64},
    {"following", 64}, {"for", 64}, {"from", 64}, {"given", 64}, {"gone", 64}, {"in", 64}, {"including", 64}, {"inside", 64}, {"into", 64}, {"like", 64},
    {"near", 64}, {"of", 64}, {"off", 64}, {"on", 64}, {"onto", 64}, {"opposite", 64}, {"outside", 64}, {"over", 64}, {"past", 64}, {"since", 64},
    {"than", 64}, {"through", 64}, {"thru", 64}, {"throughout", 64}, {"to", 64}, {"touching", 64}, {"toward", 64}, {"towards", 64}, {"under", 64}, {"underneath", 64},
    {"unlike", 64}, {"until", 64}, {"up", 64}, {"upon", 64}, {"versus", 64}, {"via", 64}, {"with", 64}, {"within", 64}, {"without", 64}, {"worth", 64},

    {"according to", 64}, {"ahead of", 64}, {"along with", 64}, {"apart from", 64}, {"as for", 64}, {"aside from", 64}, {"as per", 64}, {"as to", 64}, {"as well as", 64}, {"away from", 64},
    {"because of", 64}, {"but for", 64}, {"by means of", 64}, {"close to", 64}, {"contrary to", 64}, {"depending on", 64}, {"due to", 64}, {"except for", 64}, {"forward of", 64}, {"further to", 64},
    {"in addition to", 64}, {"in between", 64}, {"in case of", 64}, {"in face of", 64}, {"in favor of", 64}, {"in front of", 64}, {"in lieu of", 64}, {"in spite of", 64}, {"instead of", 64}, {"in view of", 64},
    {"irrespective of", 64}, {"near to", 64}, {"next to", 64}, {"on account of", 64}, {"on behalf of", 64}, {"on board", 64}, {"on to", 64}, {"on top of", 64}, {"opposite to", 64}, {"opposite of", 64},
    {"other than", 64}, {"out of", 64}, {"outside of", 64}, {"owing to", 64}, {"prior to", 64}, {"regardless of", 64}, {"thanks to", 64}, {"together with", 64}, {"up against", 64}, {"up to", 64},
    {"up until", 64}, {"one", 64}, {"two", 64}, {"three", 64}, {"four", 64}, {"five", 64}, {"six", 64}, {"seven", 64}, {"eight", 64}, {"nine", 64},
    {"zero", 64}, {"with reference to", 64}, {"with regard to", 64},

    // Conjunctions
    {"and", 128}, {"but", 128}, {"or", 128},  // consider either/or, neither/nor
    {"although", 128}, {"after", 128}, {"as", 128}, {"before", 128}, {"if", 128}, {"since", 128}, {"that", 128},
    {"unless", 128}, {"until", 128}, {"when", 128}, {"whenever", 128}, {"whereas", 128}, {"while", 128},

    {"as long as", 128}, {"as soon as", 128}, {"as though", 128}, {"except that", 128},
    {"in order that", 128}, {"provided that", 128}, {"so long as", 128}, {"such that", 128}, {"four", 128}, {"five", 128}, {"six", 128}, {"seven", 128}, {"eight", 128}, {"nine", 128},

    // Negotiators
    {"hi", 256}, {"hey", 256}, {"hey there", 256}, {"greetings", 256}, {"good-morning", 256},
    {"bye", 256}, {"bye-bye", 256}, {"goodbye", 256}, {"goodnight", 256}, {"see-ya", 256},
    {"excuse me", 256}, {"sorry", 256}, {"thanks", 256}, {"no thank you", 256}, {"oops", 256}, {"wow", 256}, {"yeah", 256}, {"oh", 256}, {"pardon me", 256}, {"cheers", 256},
    {"um", 256}, {"er", 256}, {"umm-hmm", 256}, {"ouch", 256}

};

// These are mostly from http://en.wikipedia.org/wiki/English_prefixes
map<string, infon*> prefixes = {
    {"deci", 0}, {"centi", 0}, {"milli", 0}, {"micro", 0}, {"nano", 0}, {"pico", 0}, {"femto", 0}, {"atto", 0}, {"zepto", 0}, {"yocto", 0}, {"hillo", 0},
    {"deka", 0}, {"hecto", 0}, {"kilo", 0}, {"mega", 0}, {"giga", 0}, {"tera", 0}, {"peta", 0}, {"exa", 0}, {"zetta", 0}, {"yotta", 0}, {"hella", 0},
    {"kibi", 0}, {"mebi", 0}, {"gibi", 0}, {"tebi", 0}, {"pebi", 0}, {"exbi", 0}, {"zebi", 0}, {"yobi", 0}, {"hebi", 0},

    {"a", 0}, {"anti", 0}, {"arch", 0}, {"be", 0}, {"co", 0}, {"counter", 0}, {"de", 0}, {"dis", 0}, {"dis", 0}, {"en", 0}, {"em", 0},
    {"ex", 0}, {"fore", 0}, {"hind", 0}, {"mal", 0}, {"mid", 0}, {"midi", 0}, {"mini", 0}, {"mis", 0}, {"out", 0}, {"over", 0}, {"post", 0},
    {"pre", 0}, {"pro", 0}, {"re", 0}, {"self", 0}, {"step", 0}, {"trans", 0}, {"twi", 0}, {"un", 0}, {"un", 0}, {"under", 0}, {"up", 0}, {"with", 0},

    {"Afro", 0}, {"ambi", 0}, {"amphi", 0}, {"an", 0}, {"a", 0}, {"ana", 0}, {"an", 0}, {"Anglo", 0}, {"ante", 0}, {"anti", 0}, {"apo", 0}, {"astro", 0}, {"auto", 0},
    {"bi", 0}, {"bio", 0}, {"circum", 0}, {"cis", 0}, {"con", 0}, {"com", 0}, {"col", 0}, {"cor", 0}, {"co", 0}, {"contra", 0}, {"cryo", 0}, {"crypto", 0},
    {"de", 0}, {"demi", 0}, {"demo", 0}, {"di", 0}, {"dia", 0}, {"dis", 0}, {"di", 0}, {"dif", 0}, {"du", 0}, {"duo", 0}, {"eco", 0}, {"electro", 0},
    {"en", 0}, {"el", 0}, {"em", 0}, {"epi", 0}, {"Euro", 0}, {"ex", 0}, {"extra", 0}, {"fin", 0}, {"Franco", 0}, {"geo", 0}, {"gyro", 0},
    {"hetero", 0}, {"hemi", 0}, {"homo", 0}, {"hydro", 0}, {"hyper", 0}, {"hypo", 0}, {"ideo", 0}, {"idio", 0}, {"in", 0}, {"Indo", 0}, {"in", 0},
    {"il", 0}, {"im", 0}, {"ir", 0}, {"infra", 0}, {"inter", 0}, {"intra", 0}, {"iso", 0}, {"macr", 0}, {"macro", 0}, {"maxi", 0}, {"mega", 0},
    {"megalo", 0}, {"meta", 0}, {"micro", 0}, {"mono", 0}, {"mon", 0}, {"multi", 0}, {"mult", 0}, {"neo", 0}, {"non", 0}, {"omni", 0}, {"ortho", 0},
    {"paleo", 0}, {"pan", 0}, {"pera", 0}, {"ped", 0}, {"per", 0}, {"peri", 0}, {"photo", 0}, {"pod", 0}, {"poly", 0}, {"preter", 0}, {"pros", 0},
    {"proto", 0}, {"pseudo", 0}, {"pyro", 0}, {"quasi", 0}, {"retro", 0}, {"semi", 0}, {"socio", 0}, {"sub", 0}, {"sup", 0}, {"super", 0}, {"supra", 0},
    {"sur", 0}, {"syn", 0}, {"sy", 0}, {"syl", 0}, {"sym", 0}, {"tele", 0}, {"trans", 0}, {"tri", 0}, {"ultra", 0}, {"uni", 0}, {"vice", 0}
};

// These are mostly from http://en.wiktionary.org/wiki/Appendix:English_suffixes
WordSMap suffixesFront = {  // non-plural-related suffixes
    {"able", 0}, {"ably", 0}, {"ac", 0}, {"acea", 0}, {"ad", 0},
    {"ade", 0}, {"aemia", 0}, {"age", 0}, {"agog", 0}, {"agogue", 0}, {"aholic", 0}, {"al", 0}, {"ally", 0},
    {"ana", 0}, {"anae", 0}, {"ance", 0}, {"ancy", 0}, {"ane", 0}, {"ant", 0}, {"ar", 0}, {"arch", 0},
    {"ard", 0}, {"aria", 0}, {"art", 0}, {"ary", 0}, {"ase", 0}, {"ate", 0}, {"athon", 0},
    {"cade", 0}, {"caine", 0}, {"carp", 0},
    {"cele", 0}, {"cide", 0}, {"clast", 0}, {"cline", 0},
    {"crat", 0}, {"cy", 0}, {"cyte", 0}, {"dale", 0}, {"derm", 0}, {"derma", 0},
    {"dom", 0}, {"drome", 0}, {"eae", 0}, {"eaux", 0}, {"ed", 0}, {"ee", 0}, {"eer", 0},
    {"ein", 0}, {"eme", 0}, {"en", 0}, {"ence", 0}, {"ency", 0}, {"ene", 0}, {"ent", 0}, {"er", 0},
    {"es", 0}, {"ese", 0}, {"esque", 0}, {"ess", 0}, {"est", 0}, {"et", 0},
    {"eth", 0}, {"ette", 0}, {"ey", 0}, {"fer", 0},
    {"fid", 0}, {"fold", 0}, {"form", 0}, {"fuge", 0}, {"ful", 0}, {"fy", 0},
    {"gate", 0}, {"gen", 0}, {"gene", 0},
    {"gram", 0}, {"graph", 0}, {"gyne", 0},
    {"ia", 0}, {"ial", 0}, {"ian", 0}, {"iana", 0}, {"ible", 0},
    {"ic", 0}, {"id", 0}, {"idae", 0}, {"ide", 0}, {"ie", 0}, {"ify", 0}, {"ile", 0}, {"in", 0},
    {"ina", 0}, {"inae", 0}, {"ine", 0}, {"ing", 0}, {"ini", 0}, {"ion", 0}, {"ise", 0}, {"ish", 0},
    {"ism", 0}, {"ist", 0}, {"ite", 0}, {"itis", 0}, {"ity", 0}, {"ium", 0},
    {"ive", 0}, {"ix", 0}, {"ize", 0}, {"kin", 0}, {"le", 0}, {"less", 0},
    {"let", 0}, {"like", 0}, {"lith", 0}, {"log", 0}, {"logue", 0},
    {"ly", 0}, {"lyse", 0}, {"lyte", 0}, {"mania", 0},
    {"ment", 0}, {"mer", 0}, {"mere", 0}, {"meter", 0}, {"mire", 0},
    {"mo", 0}, {"morph", 0}, {"most", 0},
    {"n't", 0}, {"nasty", 0}, {"ness", 0}, {"nik", 0}, {"o", 0},
    {"ode", 0}, {"odon", 0}, {"oholic", 0}, {"oic", 0}, {"oid", 0}, {"ol", 0}, {"ole", 0},
    {"oma", 0}, {"ome", 0}, {"on", 0}, {"one", 0}, {"ont", 0}, {"onym", 0},
    {"or", 0}, {"orama", 0}, {"ory", 0}, {"ose", 0}, {"ous", 0}, {"para", 0},
    {"path", 0}, {"ped", 0}, {"pede", 0}, {"petal", 0}, {"phage", 0},
    {"phane", 0}, {"phil", 0}, {"phobe", 0},
    {"phone", 0}, {"phony", 0}, {"phore", 0}, {"phyll", 0},
    {"plasm", 0}, {"plast", 0},
    {"plex", 0}, {"ploid", 0}, {"pod", 0}, {"pode", 0}, {"pter", 0},
    {"punk", 0}, {"ry", 0}, {"s", 0}, {"scape", 0}, {"scope", 0}, {"scribe", 0}, {"script", 0}, {"sect", 0},
    {"ship", 0}, {"sis", 0}, {"some", 0}, {"st", 0}, {"stat", 0},
    {"ster", 0}, {"stome", 0}, {"tend", 0}, {"th", 0},
    {"thon", 0}, {"tion", 0}, {"tome", 0}, {"tron", 0},
    {"tude", 0}, {"ture", 0}, {"ty", 0},
    {"ule", 0}, {"ure", 0},
    {"ware", 0}, {"ways", 0}, {"wear", 0}, {"wide", 0}, {"wise", 0}, {"y", 0}, {"zyme", 0}
};

WordSMap EnglishSuffixes;

char letterMap[127]; // Help distinguish vowels, consonants, etc.
#define isVowel(ch) (letterMap[(uint)(ch)])

bool XlaterENGLISH::loadLanguageData(string dataFilename){
    string reversedKey;
    // Here we copy suffixes into a new map where they are spelled backwards.
    // This makes it faster to search for things at the end of the word because when backwards they are at the beginning.
    EnglishSuffixes.clear();
    for(WordSMap::iterator S = suffixesFront.begin(); S!=suffixesFront.end(); ++S){
        string reversedKey=string(S->first.rbegin(), S->first.rend());
        WordSPtr wsp=WordSPtr(new WordS(S->first, 0, 0, this));
        EnglishSuffixes.insert(pair<string, WordSPtr>(reversedKey+"%U", wsp));
        wordLibrary.insert(pair<wordKey, WordSPtr>(S->first+"%U", wsp));
    }

    for(map<string, infon*>::iterator S = prefixes.begin(); S!=prefixes.end(); ++S){
        WordSPtr wsp=WordSPtr(new WordS(S->first, 0, 0, this));
        wordLibrary.insert(pair<wordKey, WordSPtr>(S->first+"%U", wsp));
    }

    memset(&letterMap, 0, 127);
    letterMap['A']=1; letterMap['a']=1; letterMap['E']=1; letterMap['e']=1; letterMap['I']=1; letterMap['i']=1;
    letterMap['O']=1; letterMap['o']=1; letterMap['U']=1; letterMap['u']=1; letterMap['Y']=1; letterMap['y']=1;
    return 0;
}

bool XlaterENGLISH::unloadLanguageData(){
    return 0;
}

enum apostropheModes {amNormal=0, amEndingInApostrophieS=1, amEndsInS_MayBePossessive=2};
WordMap apostrophics = {
    // First, list contractions that end in 's so they can be detected.
    {"all's", 1}, {"he's", 1}, {"she's", 1}, {"here's", 1}, {"how's", 1}, {"it's", 1}, {"let's", 1},
    {"when's", 1}, {"where's", 1}, {"which's", 1}, {"what's", 1}, {"who's", 1}, {"that's", 1}, {"there's", 1}, {"this's", 1},
    {"getting's", 1}, {"I's", 1}, {"O's", 1}, {"0's", 1}, {"A's", 1}, {"S's", 1},   // Also, lowercase letter are pluralized with apostrophies
    {"everybody's", 2}, {"everyone's", 2}, {"nobody's", 2}, {"noone's", 2}, {"no-one's", 2}, {"nothing's", 2}, {"something's", 2}, {"someone's", 2}, {"somebody's", 2},

    {"'00s", 0}, {"'20s", 0}, {"'30s", 0}, {"'40s", 0}, {"'50s", 0}, {"'60s", 0}, {"'70s", 0}, {"'80s", 0}, {"'90s", 0}
};

bool tagIsMarkedPossessive(WordSPtr &tag){
    int tagLen=tag->baseForm.length();
    if(tagLen>2){
        string ending=tag->baseForm.substr(tagLen-2, 2);
        if(ending=="'s"){
            WordMapIter w = apostrophics.find(tag->baseForm);
            if(w->second==amEndingInApostrophieS){return false;}
            else if(w->second==amEndsInS_MayBePossessive){return false;} // These could be possessives. TODO: try using grammer/models to decide.
            tag->baseForm=tag->baseForm.substr(0,tagLen-2);
            tag->wordFlags|=wfIsMarkedPossessive;
        }else if(ending=="s'"){
            tag->baseForm=tag->baseForm.substr(0,tagLen-1);
            tag->wordFlags|=wfIsMarkedPossessive;
        }
    }
    return true;
}

int FunctionWordClass(WordSPtr tag){
    WordMapIter w = functionWords.find(tag->baseForm);
    if(w!=functionWords.end())  return w->second;
    return 0;
}

int examineMatchingStatus(string wrdToParse, string trial){
    char t, w; uint i, maxLen=min(wrdToParse.length(), trial.length());
    for(i=0; i<maxLen; ++i){
        t=trial[i]; w=wrdToParse[i];
        if(w != t){
            if(w < t) return -i;
            else return i;
        }
        if(t=='%') return i;
    }
    if(trial.length()>maxLen && trial[maxLen]!='%') return -i;
    return i;
}

void lookUpAffixesFromCharPos(WordSMap* wordLib, const string &wrdToParse, int pos, const string &scopeID, vector<WordList> *resultList){
    uint searchLen=1, choiceLen=0, scopeScore, crntWrdLen=wrdToParse.length(); WordSPtr crntChoice=0; int matchLen, prevLen;
cout<<"  TRY: "<<wrdToParse<<"   \n";
    while(searchLen<=crntWrdLen){
        string partKey=""+wrdToParse.substr(0,searchLen);
        WordSMap::iterator trialItr=wordLib->lower_bound(partKey);
        if(trialItr==wordLib->end()) {break;}
        string trial=trialItr->first;
        crntChoice=0; prevLen=0; scopeScore=0;
        while((matchLen=examineMatchingStatus(wrdToParse, trial)) > 0){
            if(crntChoice && matchLen>prevLen) {break;} // Size has changed.
            if(trial[matchLen]=='%'){
                uint newScopeScore=calcScopeScore(scopeID, trial.substr(matchLen+1));
                if(newScopeScore > scopeScore) {
                    scopeScore=newScopeScore;
                    crntChoice=trialItr->second;
                    choiceLen=prevLen=matchLen;
                }
            } else break;
            if(++trialItr != wordLib->end()) trial=trialItr->first; else break;
        }
        if(crntChoice){
            cout<<"     PUSHING:"<<crntChoice->norm<<"\n";
            (*resultList)[pos].push_back(crntChoice);
            searchLen=choiceLen+1;
        } else searchLen=matchLen+1;
        if(matchLen <= 0) {break;}

    }
}

typedef vector<WordList> wordListVec;
typedef wordListVec::iterator wordListVecItr;

int parseWord(WordSMap *wordLib, const string &wrdToParse, const string &scopeID, wordListVec *resultList){
    cout<<"\nFIND: "<<wrdToParse<<"   \n";
    int furthestPos=0;
    int wrdLen=wrdToParse.length();
    lookUpAffixesFromCharPos(wordLib, wrdToParse, 0, scopeID, resultList);
    for(int searchPos=0; searchPos<wrdLen; ++searchPos){
        if(! resultList[searchPos].empty())
            for(WordListItr WLi=(*resultList)[searchPos].begin(); WLi != (*resultList)[searchPos].end(); ++WLi){
                int newPos=searchPos+(*WLi)->norm.length();
                if (newPos>furthestPos) furthestPos=newPos;
                if((*resultList)[newPos].empty() && newPos<wrdLen)
                    lookUpAffixesFromCharPos(wordLib, wrdToParse.substr(newPos), newPos, scopeID, resultList);
            }
    }
    return furthestPos;
}

#define tryParse(str, msg) {   \
    if(!foundPath){            \
        cout<<"\nSPELLING-RULE: "<<msg<<"\n";   \
        modToParse=(str); modLen=modToParse.length();                             \
        for(sPos=0; sPos<=modLen; ++sPos) alts[sPos].clear();                      \
        farPos=parseWord(&wordLibrary, modToParse, scopeID, &alts);               \
        if(farPos==modLen) foundPath=true;                                        \
    } }

void XlaterENGLISH::findDefinitions(WordSPtr words){
    UnicodeString txt=""; // UErrorCode err=U_ZERO_ERROR; int32_t Result=0;
    UnicodeString ChainText; tagChainToString(words, ChainText);
    WordSPtr crntChoice=0;
    WordListItr WLi;
    string scopeID=words->key.substr(words->key.find('%')+1);
    for(WordListItr crntWrd=words->words.begin(); crntWrd!=words->words.end();){
        WordListItr tmpWrd;
        uint numWordsInCompound=0, numWordsInChosen=0, scopeScore=0; crntChoice=0;
        string trial="", wordKey="";
        for(tmpWrd=crntWrd; tmpWrd!=words->words.end() && numWordsInCompound++ < maxWordsInCompound; tmpWrd++){
            if(numWordsInCompound>1) wordKey.append("-");
            wordKey.append((*tmpWrd)->norm);
            cout << "#######>"<<wordKey<<"\t\t"<<scopeID<<"\n";
            WordSMap::iterator trialItr=wordLibrary.lower_bound(wordKey);
            if(trialItr==wordLibrary.end()) break;
            trial=trialItr->first;
            bool stopSearch=true;
            int keyLen=wordKey.length();
            while(trial.substr(0,keyLen) == wordKey){
                stopSearch=false;
                if(trial[keyLen]=='%'){cout << "\tMATCH!\n";
                    uint newScopeScore=calcScopeScore(scopeID, trial.substr(keyLen+1));
                    if(newScopeScore > scopeScore) {
                        scopeScore=newScopeScore;
                        crntChoice=trialItr->second;
                        numWordsInChosen=numWordsInCompound;
                    }
                }
                if(++trialItr != wordLibrary.end()) trial=trialItr->first; else break;
            }
            if(stopSearch) break;
        }

        if(crntChoice==0){
            (*crntWrd)->baseForm=(*crntWrd)->norm;
            if(tagIsMarkedPossessive(*crntWrd)){}
            int wordClass=FunctionWordClass(*crntWrd);
            if(wordClass) {
                if(wordClass==wcNumberStarter){
                    // load number
                    // crntChoice=XXX;
                    // numWordsInChosen=YYY;
                } else if(wordClass==wcDeterminer){ (*crntWrd)->wordClass=cDeterminer;
                } else if(wordClass==wcPronoun)   { (*crntWrd)->wordClass=cPronoun;
                } else if(wordClass==wcQuantifier){ (*crntWrd)->wordClass=cDeterminer;
                } else if(wordClass==wcVerbHelper){ (*crntWrd)->wordClass=cVerbHelper;
                } else if(wordClass==wcPreposition){ (*crntWrd)->wordClass=cPreposition;
                } else if(wordClass==wcConjunction){ (*crntWrd)->wordClass=cConjunction;
                } else if(wordClass==wcNegotiator){} (*crntWrd)->wordClass=cNegotiator;
            }else { // Try parsing inside the word for prefixes, suffixes, etc.
            // Here we look for possessives, plurals, verb-forms, affixes, NVAJF, Contracted verbs.
            // We should handle alternate spellings such as UK/US, before a suffix, defined "runs" but uses "running".
                string wrdToParse=(*crntWrd)->baseForm;
                uint wrdLen=wrdToParse.length();
                string modToParse; uint foundPath=0, sPos, modLen, farPos=0; // These are used in tryParse macro.
                wordListVec alts(wrdLen+5,WordList()); // The 5 is to have room for modified spellings.
                uint furthestPos=parseWord(&wordLibrary, wrdToParse, scopeID, &alts);
cout<<"furthest:"<<furthestPos<<"  wrdLen:"<<wrdLen<<"\n";
                if(furthestPos<wrdLen){ // If we didn't find a path thru wrdToParse try other spellings:
                    // TODO: verify more rigerously that there are no important exceptions to this logic:
                    string reversedWrd=string(wrdToParse.rbegin(), wrdToParse.rend());
                    wordListVec backAlts(reversedWrd.length(),WordList());
                    parseWord(&EnglishSuffixes, reversedWrd, scopeID, &backAlts);

                    // Find potential word-suffix breaks:
                    for(uint bPos=0; bPos<wrdLen; ++bPos){
                        if(!backAlts[bPos].empty()) {
                            for(WLi=backAlts[bPos].begin(); WLi != backAlts[bPos].end(); ++WLi){
                                int newPos=bPos+(*WLi)->norm.length();
                                reversedWrd[wrdLen-newPos]='*';
                            }
                        }
                    }

                    // Try common modified spellings
                    for(uint brkPos=1; brkPos<wrdLen; brkPos++){
                        if (reversedWrd[brkPos]!='*') continue;
                        char suf1Char=wrdToParse[brkPos];
                        string suf2Char=""; if(brkPos<(wrdLen-1)) {suf2Char+=suf1Char;suf2Char+=wrdToParse[brkPos+1];}
                        char pre1Char=wrdToParse[brkPos-1];
                        string pre2Char=""; if(brkPos>1) {pre2Char+=wrdToParse[brkPos-2]; pre2Char+=pre1Char;}
                        string prePart=wrdToParse.substr(0,brkPos); string postPart=wrdToParse.substr(brkPos);
                        cout<<pre2Char<<", "<<pre1Char<<", "<<suf1Char<<", "<<suf2Char<<" ["<<prePart<<"-"<<postPart<<"]  ";
                        modLen=0; foundPath=0; farPos=0;
                        if(isVowel(suf1Char)){
                            if(pre1Char=='v' && suf2Char=="es"){
                                tryParse(prePart.substr(0,brkPos-1)+"f"+postPart, "Wolves");   // Wolves -> Wolf
                                tryParse(prePart.substr(0,brkPos-1)+"fe"+postPart, "Wives");   // Wives  -> Wife
                            }
                            if(pre2Char=="ll" && suf2Char=="ed"){tryParse(prePart.substr(0,brkPos-1)+postPart, "LL-ed");}
                            if(pre1Char=='y' && suf2Char=="in" && brkPos<(wrdLen-2) && wrdToParse[brkPos+2]=='g'){
                                tryParse(prePart.substr(0,brkPos-1)+"ie"+postPart, "Y-ing"); // dying, lying, tying -> die, lie, tie
                            }
                            if(pre2Char!="" && !isVowel(pre1Char) && pre2Char[0]==pre2Char[1]){tryParse(prePart.substr(0,brkPos-1)+postPart, "DblCons");} // Stopped
                            else if(!isVowel(pre1Char)) {tryParse(prePart+"e"+postPart, "Silent-e");}  // Cared -> Care,  Bluer -> Blue
                        } else {
                            if(suf2Char=="ly"){
                                if(!isVowel(pre1Char)){tryParse(prePart+"le"+postPart, "LY->LE");}   // Terribly -> Terrible
                                if(pre1Char=='l'){tryParse(prePart+"l"+postPart, "LY->L");}          // Fully -> Full
                            }
                        }
                        if(pre1Char=='i'){tryParse(prePart.substr(0,brkPos-1)+"y"+postPart, "Pony");} // Ponies -> Pony
                        if(foundPath) cout<<"FOUND-PATH\n"; else cout<<"No path\n";
                    }
                } else {modToParse=wrdToParse; modLen=wrdLen; foundPath=true;}
                if(foundPath) {// Extract a path
                    cout << "Extracting A Path: \n";
                    uint curEnd=modLen;
                    while(curEnd){
                        for(uint chPos=0; chPos<curEnd; ++chPos){
                            if(!alts[chPos].empty()) {
                                for(WLi=alts[chPos].begin(); WLi != alts[chPos].end(); ++WLi){
                                    if(chPos+(*WLi)->norm.length() == curEnd){
                                        curEnd=chPos;
                                        (*crntWrd)->words.push_front(*WLi);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    WordS &crntWord= *(*crntWrd);
                    for(WLi=crntWord.words.begin(); WLi != crntWord.words.end(); ++WLi){cout <<(*WLi)->norm << "-";}
                    string lastAffix=(*crntWord.words.back()).norm;
                    if(lastAffix=="s" || lastAffix=="es"){crntWord.wordForm=form_S_ES;}
                    else if(lastAffix=="ing"){crntWord.wordForm=form_ING;}
                    else if(lastAffix=="ed"){crntWord.wordForm=form_ED;}

                    else if(lastAffix=="est") {crntWord.wordDegree=dSuperlative;}
                    else if(lastAffix=="er")  {crntWord.wordDegree=dComparative;}

                    else if(lastAffix=="'ll"){} // -will
                    else if(lastAffix=="'d"){}  // -had -would
                    else if(lastAffix=="'ve"){}  // -have
                    else if(lastAffix=="'re"){}  // -are
                    else if(lastAffix=="'m"){}  // I'm
                    else if(lastAffix=="'ll've"){} // -will have
                    else if(lastAffix=="'d've"){}  // -had have -would have
                    // NOW Construct definition infon
                }
            }
        }
        if((*crntWrd)->wordClass <= cAdv){ // If this isn't a function word...
            if(crntChoice==0) {words->wordFlags|=wfErrorInAChildWord; cout<< ((string)"\n\nMESG: What does "+(*crntWrd)->norm+" mean?\n"); crntWrd++;}
            else {
                if(numWordsInChosen>1){ // If more than one word was consumed, move them into a new parent.
                    WordSPtr parent=WordSPtr(new WordS);
                    parent->definition=crntChoice->definition;
                    words->words.insert(crntWrd, parent);
                    WordListItr wrd=crntWrd;
                    for(uint i=0; i<numWordsInChosen; ++i){
                        if(i>0) parent->norm+=" ";
                        parent->norm+=(*wrd)->norm;
                        parent->words.push_back(*wrd);
                        wrd=words->words.erase(wrd);
                    }
                } else {(*crntWrd)->definition=crntChoice->definition; ++crntWrd;}
            }
        }
    }
}

void XlaterENGLISH::stitchAndDereference(WordSPtr text){
    text->definition=(*text->words.begin())->definition;
}

infon* XlaterENGLISH::infonate(WordSPtr text){
    return text->definition;
}

///////////////////////////////////////////////////////////
//            E N G L I S H   P A R S E R                //
///////////////////////////////////////////////////////////

parseResult EnglishParser::ParseEnglish(ParserArgList){
    parseResult result;
    // if((result=pNegotiator(WordSystem, crntWrd, context)) != prNoMatch) {} else
    if((result=pSentence(WordSystem, crntWrd, context)) != prNoMatch) {/* reduce(WordSystem); */}
    else result = prNoMatch;
    return result;
}

parseResult EnglishParser::pSentence(ParserArgList){
    parseResult result;
    // Check for "it BE" of Cleft Sentence (3)
    // Check for "there BE" of "existential there" (2)
    // Check for a verb-phrase start due to a postponed subject  (4)
    // Check for adjunct / adverb / other clause (5)
    if((result=pNounPhrase(WordSystem, crntWrd, context)) != prNoMatch) {}
    else result = prNoMatch;
    return result;
    }

parseResult EnglishParser::pNounPhrase(ParserArgList){
    parseResult result;
    if((result=pDeterminer(WordSystem, crntWrd, context)) != prNoMatch) {} else return result;

    if((result=pSelectorSeq(WordSystem, crntWrd, context)) != prNoMatch) {}
    else result = prNoMatch;
    return result;
    }

parseResult EnglishParser::pSelectorConjunct(ParserArgList){
    parseResult result;
    if((result=pDeterminer(WordSystem, crntWrd, context)) != prNoMatch) {} else return result;

    if((result=pSelectorSeq(WordSystem, crntWrd, context)) != prNoMatch) {}
    else result = prNoMatch;
    return result;
    }

parseResult EnglishParser::pDeterminer(ParserArgList){
    parseResult result;
  //  if((result=pDUMMY(WordSystem, crntWrd, context)) != prNoMatch) {}
  //  else result = prNoMatch;
    return result;
}

parseResult EnglishParser::pSelectorSeq(ParserArgList){
    parseResult result;
    // if((result=pNegotiator(WordSystem, crntWrd, context)) != prNoMatch) {} else
 //   if((result=pSentence(WordSystem, crntWrd, context)) != prNoMatch) {/* reduce(WordSystem); */}
 //   else result = prNoMatch;
    return result;
}
