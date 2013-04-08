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

extern sqlite3 *coreDatabase;

const uint maxWordsInCompound=10;
UErrorCode uErr=U_ZERO_ERROR;
const icu::Normalizer2 *tagNormalizer=Normalizer2::getNFKCCasefoldInstance(uErr);

bool isEngWordChar(char ch){
    return (isalpha(ch) || ch=='-' || ch=='\'');
}

bool isCardinalOrOrdinal(char ch){
    return (isdigit(ch) || ch=='.'  || ch=='-' || ch=='s' || ch=='t' || ch=='h' || ch=='n' || ch=='r' || ch=='d');
}

string parseSenseID(QParser *parser){
    string senseID="";
    if     (parser->chkStr("#n#")){senseID="#n#";}
    else if(parser->chkStr("#v#")){senseID="#v#";}
    else if(parser->chkStr("#a#")){senseID="#a#";}
    else if(parser->chkStr("#r#")){senseID="#r#";}
    else if(parser->chkStr("#f#")){senseID="#f#";}
    else{
        getBufs(isEngWordChar(parser->peek()),parser);
        senseID=parser->buf;
    }
    if(senseID[0]=='#') { // Read Wordnet sense identifier
        if(isdigit(parser->peek())){senseID += parser->streamGet();}
        else throw "Numeric Sense ID expected";
        if(isdigit(parser->peek())){senseID += parser->streamGet();}
    }
    return senseID;
}

WordSystemTypes validNumberSyntax(string &in){
    WordSystemTypes ret=wstNumCard;
    string n=in;
    string tail="";
    char ch=n[0];
    int start=0, len=n.size();
    if(ch=='-') {start=1;}
    else if(!isdigit(ch)) return wstUnknown;
    if(len-start >= 3) { // Check for st, th, nd, rd ending
        tail=n.substr(len-3, 3); cout<<"tail:"<<tail<<".\n";
        if(tail=="1st"||tail=="2nd"||tail=="3rd"||tail=="4th"||tail=="5th"||
           tail=="6th"||tail=="7th"||tail=="8th"||tail=="9th"||tail=="0th") {
            len-=2; ret=wstNumOrd;
        }
    }
    for(int p=start; p<len; ++p){
        ch=n[p];
        if(!isdigit(ch) && ch!='.') return wstUnknown;
    }
    return ret;
}

void tagChainToString(WordS *tags, UnicodeString *strOut){
    *strOut=""; WordS* n;
    for(WordListItr itr=tags->words.begin(); itr!=tags->words.end(); ++itr){
        n= (*itr).get();
        if((*strOut)!="") (*strOut)+=" ";
        n->offsetInSource=strOut->length();
        (*strOut) += strOut->fromUTF8(n->norm);
    }
}

WordS* XlaterENGLISH::ReadLanguageWord(QParser *parser, icu::Locale &locale){ // Reads a 'word' consisting of a number or an alphabetic+hyphen+appostrophies tag
    char tok=parser->Peek();
    if(isdigit(tok) || tok=='-') {getBufs(isCardinalOrOrdinal(parser->peek()),parser);}
    else if(isalpha(tok)){getBufs(isEngWordChar(parser->peek()),parser);}
    else return 0;
    if(parser->buf[0]==0) return 0;
    WordS* tag=new WordS;
    string &parsed=tag->asRead=parser->buf; tag->locale=locale.getBaseName();
    if(isalpha(tok)){
        tagNormalizer->normalize(UnicodeString::fromUTF8(parsed), uErr).toUTF8String(tag->norm);
        if(tagIsBad(tag->norm, tag->locale.c_str()))
            {cout<<"BAD TAG:"<<tag->norm<<"\n"; throw ("Problem with characters in word");}
        if(tag->norm.at(0)=='-'){tag->flags1|=wfAsSuffix; tag->norm=tag->norm.substr(1);}
        if(tag->norm.at(tag->norm.length()-1)=='-'){tag->flags1|=wfAsPrefix; tag->norm=tag->norm.substr(0,tag->norm.length()-1);}
        if(tag->norm.find('-')!=string::npos) {tag->flags1|=wfWasHyphenated;}

        if(parser->peek()=='#'){ // Read word sense identifier
            tag->senseID=parseSenseID(parser);
        }
    } else {
        WordSystemTypes sysType=validNumberSyntax(parsed);
        if(sysType==wstUnknown) throw "Invalid number syntax";
        tag->norm=parsed;
        if(sysType==wstNumOrd) {parser->buf[parsed.length()-1]=' '; parser->buf[parsed.length()-2]=' ';}
        tag->sysType=sysType;
        // Create number as model of this:
        UInt size=1;
        pureInfon iSize, iVal;
        iSize=pureInfon(size);
        numberFromString(parser->buf, &iVal, 10);
        iVal.flags|=tNum+dDecimal;
        infon* i=new infon(0, &iSize, &iVal);
        i->wSize=size;
        if (i->value.listHead)i->value.listHead->top=i;
        tag->definition=i;
    }
    return tag;
}

void XlaterENGLISH::ReadTagChain(QParser *parser, icu::Locale &locale, WordS& head){ // Reads a phrase that ends at a non-matching character or a period that isn't in a number.
    WordS* nxtTag;
    while((nxtTag=ReadLanguageWord(parser, locale))){
        if(head.words.size()==0) {head.norm="";} else head.norm+="-";
        head.norm+=nxtTag->norm;
        head.senseID=nxtTag->senseID;
        head.words.push_back(WordSPtr(nxtTag));
    }
    parser->nxtTokN(1,".");
    head.locale=locale.getBaseName();
}

infon* XlaterENGLISH::tags2Proteus(WordS& words){   // Converts a list of words read by ReadTagChain() into an infon and returns a pointer to it.
    findDefinitions(words);
  //  stitchAndDereference(words);
    infon* proteusCode = infonate(words);
    return proteusCode;
}

void XlaterENGLISH::proteus2Tags(infon* proteus, WordS& WordsOut){  // Converts an infon to a tag chain.
}

typedef map<string, uint> WordMap;
typedef WordMap::iterator WordMapIter;

/*  These Function Words can have more than one usage / sense:
 *      That / Which:   Demonstrative Pronoun:   THAT book is mine.  (Also see: this,that,these,those.)
 *                      Bare pronoun: THAT is mine.
 *                      Subordinating Conjunction: Everyone knows THAT smoking is dangerous.
 *                      Start of relative clause: The book THAT I am reading is on safety.
 *
 *      They, Them, Their, Theirs: 3rd person plural but sometimes used instead of 'he/she' as 3rd person singular pronouns.
 *
 *      Some quantifiers can be used alone: MANY are called. FEW are chosen.
 *
 *      One: "Number 1", "ONE ought not speak suchly during informal settings.", "I want ONE. I want ONE too. I want ONE two three."
 *
 *      You: This 2nd person pronoun can have 4 uses: It can be singular or plural, and either subject or object.
 *
 *      Your, Yours: These pronouns can be either singular or plural.
 *
 *      His: This possessive pronoun can be used dependently (It's HIS book.) or independently (It's HIS.).
 *
 *      Her: HER can be either an objective pronoun: "They congratulated HER" or a possessive-dependent pronoun: "It's HER book.".
 *
 *      It: The pronoun it can be either subjective or objective.
 * */

const uint Det=wfHasDetSense;
const uint Qnt=Det+wfHasNumSense;
const uint Num=Qnt+wfCanStartNum;
const uint ProN=wfIsPronoun;
const uint AuxV=wfVerbHelper;

const uint p1st=wf1stPrsn;
const uint p2nd=wf2ndPrsn;
const uint p3rd=wf3rdPrsn;
const uint pMas=wfGndrMasc;
const uint pFem=wfGndrFem;
const uint pNeut=wfGndrNeut;
const uint isPl=wfIsPlural;
const uint Posv=wfIsPossessive;

WordMap functionWords = {
    // Definite determiners
        {"the", Det}, {"this", Det}, {"that", Det}, {"these", Det+isPl}, {"those", Det+isPl},   // These and Those are plural of this and that
        {"which", Det}, {"whichever", Det}, {"what", Det}, {"whatever", Det},       // These usually start questions
        // Other definite determiners are possessives including the dependent possessive pronouns:
        //     my, your, his, her, its, our, their, whose. And maybe whoever/whomever and whosever.

    // Indefinite determiners
        {"a", Det}, {"an", Det},       // Use with singular, countable nouns.
        {"some", Det+isPl}, {"any", Det+isPl},   // Use with plural and non-countable nouns.

    // Quantifying determiners // I need to define a "quantifier phrase"
        {"much", Qnt}, {"many", Qnt}, {"more", Qnt}, {"most", Qnt}, {"little", Qnt}, {"few", Qnt}, {"less", Qnt}, {"fewer", Qnt}, {"least", Qnt}, {"fewest", Qnt},
        {"all", Qnt}, {"both", Qnt}, {"enough", Qnt}, {"no", Qnt},
        {"each", Qnt}, {"every", Qnt}, {"either", Qnt}, {"neither", Qnt},   // 'every' can be 'almost every' and some other forms. Likewise for 'all', 'enough', numbers, etc.

        {"zero", Num}, {"one", Num}, {"two", Num}, {"three", Num}, {"four", Num}, {"five", Num}, {"six", Num}, {"seven", Num}, {"eight", Num}, {"nine", Num},
        {"ten",Num},{"eleven",Num},{"twelve",Num},{"thirteen",Num},{"fourteen",Num},{"fifteen",Num},{"sixteen",Num},{"seventeen",Num},{"eighteen", Num},{"nineteen",Num},
        {"twenty", Num}, {"thirty", Num}, {"forty", Num}, {"fifty", Num}, {"sixty", Num}, {"seventy", Num}, {"eighty", Num}, {"ninety", Num},

        // when compound function words are supported:
        {"all the", Qnt}, {"some of the", Qnt}, {"some of the many", Qnt},  // also: "5 liters of"
        {"a pair of", Qnt}, {"half", Qnt}, {"half of", Qnt}, {"double", Qnt}, {"more than", Qnt}, {"less than", Qnt}, // twice as many as... ,  a dozen,
        {"a lot of", Qnt}, {"lots of", Qnt}, {"plenty of", Qnt}, {"a great deal of", Qnt}, {"tons of", Qnt}, {"a few", Qnt}, {"a little", Qnt},
        {"several", Qnt}, {"a couple", Qnt}, {"a number of", Qnt}, {"a whole boat load of", Qnt}, //... I need to find a pattern for these

    // Pronouns
        {"I",   ProN+p1st},     {"me", ProN+p1st},       {"my",  ProN+p1st+Posv+Det},       {"mine",  ProN+p1st+Posv},      {"myself",  ProN+p1st},
        {"you", ProN+p2nd},      /* you */               {"your",ProN+p2nd+Posv+Det},       {"yours", ProN+p2nd+Posv},      {"yourself",ProN+p2nd+Posv},
         /* one */               /* one */               {"one's", ProN+Posv+Det},                                          {"oneself", ProN},
        {"he",  ProN+p3rd+pMas},{"him", ProN+p3rd+pMas}, {"his", ProN+p3rd+pMas+Posv+Det},   /* his */                      {"himself", ProN+p3rd+pMas},
        {"she", ProN+p3rd+pFem},{"her", ProN+p3rd+pFem+Det},  /* her */                     {"hers", ProN+p3rd+pFem+Posv},  {"herself", ProN+p3rd+pFem},
        {"it",  ProN+p3rd+pNeut},/* it */                {"its", ProN+p3rd+pNeut+Posv+Det},                                 {"itself",  ProN+p3rd+pNeut},

        {"we", ProN+p1st+isPl}, {"us", ProN+p1st+isPl},  {"our", ProN+p1st+isPl+Posv+Det},  {"ours", ProN+p1st+isPl+Posv},  {"ourselves",  ProN+p1st+isPl},
         /* you */               /* you */                /* your */                         /* yours */                    {"yourselves", ProN+p2nd+isPl},
        {"they",ProN+p3rd+isPl},{"them", ProN+p3rd+isPl},{"their", ProN+p3rd+isPl+Posv+Det},{"theirs", ProN+p3rd+isPl+Posv},{"themselves", ProN+p3rd+isPl},

        {"who", ProN},          {"whom", ProN},          {"whose", ProN+Posv+Det},    /* which,   that */

        // consider: this, that, these, and those can occur without a noun and thus act like pronouns.
        // consider: one in "I ate one"

    // Verb Helpers
        {"be", 16}, {"am", 16}, {"are", 16}, {"aren't", 16}, {"is", 16}, {"isn't", 16}, {"was", 16}, {"wasn't", 16}, {"were", 16}, {"weren't", 16}, {"being", 16}, {"been", 16},
        {"have", 16}, {"haven't", 16}, {"has", 16}, {"hasn't", 16}, {"had", 16}, {"hadn't", 16}, {"having", 16}, {"ain't", 16},
        {"do", 16}, {"don't", 16}, {"did", 16}, {"didn't", 16}, {"does", 16}, {"doesn't", 16}, {"doing", 16},
        {"can", 16}, {"can't", 16}, {"cannot", 16}, {"could", 16}, {"couldn't", 16}, {"should", 16}, {"shouldn't", 16}, {"would", 16}, {"wouldn't", 16},
        {"will", 16}, {"won't", 16}, {"shall", 16}, {"shan't", 16}, {"may", 16}, {"mayn't", 16}, {"might", 16}, {"mightn't", 16}, {"must", 16}, {"mustn't", 16},
        // consider: contracted is, has, am, are, have, had and will (e.g, he's, I've, I'll)
        // consider: better, had better, and various compound items

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
    {"up until", 64}, {"with reference to", 64}, {"with regard to", 64},

    // Conjunctions
    {"and", 128}, {"but", 128}, {"or", 128},  // consider either/or, neither/nor
    {"although", 128}, {"after", 128}, {"as", 128}, {"before", 128}, {"if", 128}, {"since", 128}, /* that */
    {"unless", 128}, {"until", 128}, {"when", 128}, {"whenever", 128}, {"whereas", 128}, {"while", 128},

    {"as long as", 128}, {"as soon as", 128}, {"as though", 128}, {"except that", 128},
    {"in order that", 128}, {"provided that", 128}, {"so long as", 128}, {"such that", 128},

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
multimap<wordKey, WordSPtr> suffixesFront = {  // non-plural-related suffixes
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

bool XlaterENGLISH::unloadLanguageData(){
    delete wordLibrary;
    return 0;
}

bool XlaterENGLISH::loadLanguageData(sqlite3 *db){
    wordLibrary=new WordLibrary(db);
    string reversedKey;
    // Here we copy suffixes into a new map where they are spelled backwards.
    // This makes it faster to search for things at the end of the word because when backwards they are at the beginning.
    EnglishSuffixes.clear();
    for(WordSMap::iterator S = suffixesFront.begin(); S!=suffixesFront.end(); ++S){
        string reversedKey=string(S->first.rbegin(), S->first.rend());
        WordSPtr wsp=WordSPtr(new WordS(S->first, 0, 0, this)); wsp->senseID="auto";
        EnglishSuffixes.insert(pair<string, WordSPtr>(reversedKey+"%U", wsp));
        wordLibrary->insert(pair<wordKey, WordSPtr>(S->first+"%U", wsp));
    }

    for(map<string, infon*>::iterator S = prefixes.begin(); S!=prefixes.end(); ++S){
        WordSPtr wsp=WordSPtr(new WordS(S->first, 0, 0, this)); wsp->senseID="auto";
        wordLibrary->insert(pair<wordKey, WordSPtr>(S->first+"%U", wsp));
    }

    memset(&letterMap, 0, 127);
    letterMap['A']=1; letterMap['a']=1; letterMap['E']=1; letterMap['e']=1; letterMap['I']=1; letterMap['i']=1;
    letterMap['O']=1; letterMap['o']=1; letterMap['U']=1; letterMap['u']=1; letterMap['Y']=1; letterMap['y']=1;

    EnglishGrammarRules.loadRules();
    return 0;
}

enum apostropheModes {amNormal=0, amEndingInApostrophieS=1, amEndsInS_MayBePossessive=2};
WordMap apostrophics = {
    // First, list contractions that end in 's so they can be detected.
    {"all's", 1}, {"he's", 1}, {"she's", 1}, {"here's", 1}, {"how's", 1}, {"it's", 1}, {"let's", 1},
    {"when's", 1}, {"where's", 1}, {"which's", 1}, {"what's", 1}, {"who's", 1}, {"that's", 1}, {"there's", 1}, {"this's", 1},
    {"getting's", 1}, {"I's", 1}, {"O's", 1}, {"0's", 1}, {"A's", 1}, {"S's", 1},   // Also, lowercase letters are pluralized with apostrophies
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
            tag->flags1|=wfIsMarkedPossessive;
        }else if(ending=="s'"){
            tag->baseForm=tag->baseForm.substr(0,tagLen-1);
            tag->flags1|=wfIsMarkedPossessive;
        }
    }
    return true;
}

////////////////////////////////////////////////////////////
//  Parser Utilities

bRule::bRule(char* ruleHead, int id, int n, ...){
    head=ruleHead;
    ID=id;
    va_list ap; va_start(ap,n); int i;
    for(i=n; i; --i){
        char* item=va_arg(ap, char*);
        rhs.push_back(item);
    }
    va_end(ap);
};

string bRule::asString() {
    string ret=head; ret+=" --> [ ";
    for(vector<string>::const_iterator si=rhs.begin(); si != rhs.end(); ++si){
        ret+= (*si) + " ";
    }
    ret += "]";
    return ret;
}

bRulePtr Grammar::addRule(bool mainLib, int id, int n, ...){
    Rules *rLib = (mainLib)?&rules:&dynRules;
    va_list ap; ++n; va_start(ap,n); int i;
    char* ruleHead=va_arg(ap, char*);
    bRule *rule=new bRule(ruleHead,id,0);
    for(i=n-1; i; --i){
        char* item=va_arg(ap, char*);
        rule->rhs.push_back(item);
    }
    va_end(ap);

    // Do not insert an identical rule
    RuleRange isMatch=rLib->equal_range(ruleHead);
    for(RuleItr ri=isMatch.first; ri!=isMatch.second; ++ri) {
        if(*(*ri).second== *rule){cout<<"REUSING RULE: "<< rule->asString()<<"\n";return (*ri).second;}
    }

    bRulePtr tmpRule=bRulePtr(rule);
    rLib->insert(pair<string,bRulePtr>(ruleHead, tmpRule));
 //   cout<<"INSERTING RULE: "<< rule->asString()<<"\n";
    return tmpRule;
    }

enum rules {rNone, rStart_sent, start_neg, sent_clause, clause_SP, clause_AC, clause_CA, clause_SAP,
    subj_NP, SC_NP, obj_NP, objC_NP, iObj_NP,
    NP_DetSel, NP_Sel, det_poss, sel_modSel, sel_qntSel, sel_selSel, sel_noun, sel_selPost,
    pred_VP, pred_VSc, pred_VO, pred_VIO, pred_VOO, VP_verb,
    auto_N, auto_V, auto_Adj, auto_Adv, auto_Mod, auto_Det, auto_Prep
};

void Grammar::loadRules(){
    ///////////// Here is the grammar for English

    rules.clear();
    addRule(1, rStart_sent, 1, "$START",        "$sentence");
    addRule(1, start_neg,   1, "$START",        "$negotiator");
    addRule(1, sent_clause, 1, "$sentence",     "$clause");    // Clause must have finite verb

    addRule(1, clause_SP, 2, "$clause",       "$subject", "$predicate");
    addRule(1, clause_AC, 2, "$clause",       "$adjunct", "$clause");
    addRule(1, clause_CA, 2, "$clause",       "$clause", "$adjunct");
    addRule(1, clause_SAP,3, "$clause",       "$subject", "$adjunct", "$predicate");

    addRule(1, subj_NP, 1, "$subject",      "$nounPhrase");
    addRule(1, SC_NP,   1, "$subjCompl",    "$nounPhrase");
    addRule(1, obj_NP,  1, "$object",       "$nounPhrase");
    addRule(1, objC_NP, 1, "$objCompl",     "$nounPhrase");
    addRule(1, iObj_NP, 1, "$indirectObj",  "$nounPhrase");

    addRule(1, NP_DetSel, 2, "$nounPhrase",   "%determiner", "$selector%");
    addRule(1, NP_Sel,    1, "$nounPhrase",   "$selector%");

    addRule(1, det_poss,  1, "$determiner",   "$possessive"); // TODO: handle 'all of the', 'many of the', etc. Also, "the X of Y('s)"

    addRule(1, sel_modSel, 2, "$selector%",     "%modifier",   "$selector%");
    addRule(1, sel_qntSel, 2, "$selector%",     "$quantifier", "$selector%");
    addRule(1, sel_noun,   1, "$selector%",     "%noun");
    addRule(1, sel_selSel, 2, "$selector%",     "$selector%",   "$selector%");
    addRule(1, sel_selPost,2, "$selector%",     "$selector%", "$selectingPostMod");

    addRule(1, pred_VP,  1, "$predicate",   "$verbPhrase");
    addRule(1, pred_VSc, 2, "$predicate",   "$verbPhrase", "$subjCompl");
    addRule(1, pred_VO,  2, "$predicate",   "$verbPhrase", "$object");
    addRule(1, pred_VIO, 3, "$predicate",   "$verbPhrase", "$indirectObj", "$object");
    addRule(1, pred_VOO, 3, "$predicate",   "$verbPhrase", "$object", "$objCompl");

    addRule(1, VP_verb,  1, "$verbPhrase",  "%verb");

}

///////////////////////////////////// Burser Code
// Thanks to the author of https://github.com/tomerfiliba/tau   (Chart parser in Python)
// and Joseph Irwin at https://github.com/cordarei/chart-parser
// and JOHN AYCOCK AND R. NIGEL HORSPOOL for their paper "Practical Earley Parsing"
// Parts of the code below and in the header file are borrowed or inspired by their work.

struct bArc;
typedef shared_ptr<bArc> ArcPtr;
typedef vector<ArcPtr> ArcPtrs;

struct bArc {  // Arc for the burser. AKA 'Earley state'
    uint start, end, dotPos;
    bRulePtr rule;
    WordSPtr wordS;  // This will hold alternatives the arc can take.

    bArc(uint s, bRulePtr r, uint pos=0, uint e=0):start(s),end(e),dotPos(pos),rule(r){};
    bool complete() const { return (dotPos == rule->rhs.size()); }
    string nextTerm() const { return (complete() ? "" : rule->rhs[dotPos]); }
    bool matches(ArcPtr other) {
        return start == other->start && end == other->end && rule == other->rule && dotPos == other->dotPos;
    }
};

struct BNode {
    typedef deque<BNode> BNodes;
    ArcPtr arc;
    BNodes* children;
    BNode(ArcPtr& Arc, BNodes* Children=0):arc(Arc), children(Children){};

};
typedef deque<BNode> BNodes;

struct Column:vector<ArcPtr> {
    uint index;
    string word;
    WordSPtr wordS;
    Column(uint Index, string Word, WordSPtr WordS):index(Index), word(Word), wordS(WordS){};
};
struct Chart:vector<Column> {
    void GatherTreeRoots(ArcPtrs &parsesOut, string headSymbol);
    void BuildTrees(BNodes* results, ArcPtr usedArc, string indent = "");
    void BuildTreesHelper(BNodes* results,BNodes* children, ArcPtr usedArc, int ruleIndex, uint endColumn, string indent = "");
    void printDiagram(BNode &tree, string indent="");
    void printForest(BNodes& forest);
    string asString();
};

void Chart::GatherTreeRoots(ArcPtrs &parsesOut, string headSymbol){
    for(uint c = 0; c < back().size(); ++c) {
        if (back()[c]->complete() && headSymbol == back()[c]->rule->head) {
            cout<<"PARSE HEAD: "<<back()[c]->rule<<"\n";
            parsesOut.push_back(back()[c]);
        }
    }
}

void Chart::BuildTrees(BNodes* results, ArcPtr usedArc, string indent){
    cout<<indent<<"BUILD TREE FOR: "<<usedArc->rule->head<<"\n";
    BNodes* children=new BNodes;
    BuildTreesHelper(results, children, usedArc, usedArc->rule->rhs.size()-1, usedArc->end, indent);
}

void Chart::BuildTreesHelper(BNodes* results,BNodes* children, ArcPtr usedArc, int ruleIndex, uint endColumn, string indent){
    int startColumn;
    BNodes HResults, VResults;
    results->clear();
    string ruleHead ="<null>"; if (ruleIndex>=0) ruleHead= usedArc->rule->rhs[ruleIndex];
//    cout<<indent<<"ruleIndex:"<<ruleIndex<<"   arc.start:"<<usedArc->start<<"   endColumn:"<<endColumn<<"   RULEHEAD:"<<ruleHead<<"\n";
    if(ruleIndex<0 || (ruleHead[0]!='$' && ruleHead[0]!='%')) {results->push_back(BNode(usedArc, children)); return;}
    else if(ruleIndex==0) startColumn = usedArc->start;
    else startColumn=-1;

    for(uint st = 0; st < (*this)[endColumn].size(); ++st){
        ArcPtr ST=(*this)[endColumn][st];
 //cout<<indent<<"   ColumnArc:"<<ST->rule->head<<"     \t";
        if (ST.get() == usedArc.get()) {break;}
        if (!ST->complete() || ST->rule->head != ruleHead) {continue;}
        if (startColumn != -1 && (int)ST->start != startColumn) {continue;}
        BuildTrees(&VResults, ST, indent+"     ");
        for(BNodes::iterator subTree=VResults.begin(); subTree != VResults.end(); ++subTree){
 //           cout<<indent<<"   subtree:"<<(*subTree).arc->rule->head<<"\n";
            BNodes* childList=new BNodes(*children); childList->push_front((*subTree));
            BuildTreesHelper(&HResults, childList, usedArc, ruleIndex - 1, ST->start, indent+"    .");
            for(BNodes::iterator node=HResults.begin(); node != HResults.end(); ++node){
                results->push_back((*node));
            }
        }
    }
}

void Chart::printDiagram(BNode &tree, string indent){
    if(tree.children && tree.children->empty()) cout << indent <<
       tree.arc->rule->head << ": '" << (*this)[tree.arc->start+1].word<<"'\n";
    else {
        cout << indent << tree.arc->rule->head << " =\n";
        for(BNodes::iterator T=tree.children->begin(); T != tree.children->end(); ++T){
            printDiagram(*T, (indent+"   ."));
        }
  //  cout << indent << tree.arc->rule->head << " =\n";
    }
}

void Chart::printForest(BNodes& forest){
    cout<<"######################################### Sentence Diagrams ("<<forest.size()<<"):\n";
    for(BNodes::iterator tree=forest.begin(); tree != forest.end(); ++tree){
        printDiagram((*tree));
        cout<<"-----------------------\n";
    }
}

infon* infonate(Chart &chart, BNode &tree){
    string head=tree.arc->rule->head;
    Column &col=chart[tree.arc->start+1];
    WordSPtr WordSystem=col.wordS;
    if(tree.children==0 || tree.children->empty()){
        if(head=="%noun"){  // try each noun sense...
        }else if(head=="%verb"){
        }else if(head=="%adj"){
        }
//       cout << head << ": '" << (col.word<<"'\n";
    }else {
        int numChilds=tree.children->size();
        for(BNodes::iterator T=tree.children->begin(); T != tree.children->end(); ++T){
            infonate(chart, *T);
        }
        if(head=="$selector%" && numChilds==2){}
    cout << head << " =\n";
    }
    return 0;
}

string Chart::asString() {
    char buf[50];
    string ret= "#####################################\nChart:\n";
    for(uint i = 0; i < size(); ++i) {
        ret += "===================\nCell: ";
        for(uint j = 0; j < size(); ++j) {
            ret += "[ " + (*this)[i][j]->rule->asString() + " (" + (*this)[i][j]->nextTerm()
                      + ") (" + itoa((*this)[i][j]->start,buf) + " " + itoa((*this)[i][j]->end,buf) + ") ";
            if((*this)[i][j]->complete()) ret += "C]\n"; else ret += "i]\n";
        }
        ret += "\n";
    }
    return ret;
}

void filterCpy(WordSPtr in, WordSPtr out, uint filter){ // Copy alts matching filter from in to out.
    for(WordListItr i=in->altDefs.begin(); i!=in->altDefs.end(); ++i){
        uint wordClass=(*i)->flags2&maskWordClass;
        if(filter){
            if(wordClass == filter) {out->altDefs.push_back((*i)); cout<<"WORD_IN:"<<(*i)->norm<<" ("<<(*i)->attributes["gloss"]<<")\n";}
        }else{
            if(wordClass == wfAdj || wordClass == wfNoun || ((*i)->flags1&wfHasIntesifierSense)) {
                cout<<"WORD_IN:"<<(*i)->norm<<" ("<<(*i)->attributes["gloss"]<<")\n";
                out->altDefs.push_back((*i));
            }
        }
    }
}

int preInfonate(Chart chart, ArcPtr arc){
    string head=arc->rule->head;
    BNode *item1=0, *item2=0, *item3=0, *item4=0;
    WordSPtr word1=0, word2=0, word3=0, word4=0;
    BNodes forest;
    chart.BuildTrees(&forest, arc);
    chart.printForest(forest);
    BNode node=forest.front();
    uint numKids=node.children->size();
    cout<<"EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE "<<head<<"  SIZE:"<<numKids<<"  "<<arc.get()<<"\n";
    switch(numKids){
        case 4: item4 = &node.children->at(3);  word4 = item4->arc->wordS;
        case 3: item3 = &node.children->at(2);  word3 = item3->arc->wordS;
        case 2: item2 = &node.children->at(1);  word2 = item2->arc->wordS; cout<<"item2:"<<item2->arc->rule->head<<"  "<<item2->arc.get()<<"  "<<word2->norm<<".\n";
        case 1: item1 = &node.children->at(0);  word1 = item1->arc->wordS; cout<<"item1:"<<item1->arc->rule->head<<"  "<<item1->arc.get()<<"  "<<word1->norm<<".\n";
    }
if(node.arc->wordS!=0) cout<<"Arc's WordS filled too many times!!!!!\n\n";
    WordSPtr wrdOut=node.arc->wordS=new(WordS);
    WordSPtr wrdIn=chart[arc->start+1].wordS;
    switch(node.arc->rule->ID){
        case auto_N:   filterCpy(wrdIn, wrdOut, wfNoun); break;
        case auto_V:   filterCpy(wrdIn, wrdOut, wfVerb); break;
        case auto_Adj: filterCpy(wrdIn, wrdOut, wfAdj); break;
        case auto_Mod: filterCpy(wrdIn, wrdOut, 0); break;
        case sel_noun: wrdOut->altDefs=word1->altDefs; break;
        case sel_modSel: // Test whether an item1 can modify item2?
        cout<<"SEL-MODifier, Selector: "<<word1->norm<<":  ";
            for(WordListItr i=word1->altDefs.begin(); i!=word1->altDefs.end(); ++i){ cout<<"X" <<(*i)->norm << " ---> [";
                for(WordListItr j=word2->altDefs.begin(); j!=word2->altDefs.end(); ++j){cout <<(*j)->norm << ", ";

                } cout<<"]\n";
            } cout<<"\n";
             break;
        default: item4=item4; break; // Stops compiler warning.
    }
    return 0;
}

struct Burser{ // Gathers items owed from various sources. e.g., subjects, objects, verbs, phrases, etc.
    const string start_symbol="$START";
    uint crntPos;  //  The number of the chart-slot and word we are on.
    Grammar *grammar;
    Chart chart;
    ArcPtrs parses;

    Burser(Grammar *grmr, infon* context):crntPos(0), grammar(grmr){
        //init chart
        chart.push_back(Column(0, start_symbol, 0));
        RuleRange strtRules=grammar->rules.equal_range(start_symbol);
        for(RuleItr ri=strtRules.first; ri!=strtRules.second; ++ri)
            chart[0].push_back(ArcPtr(new bArc(0,(*ri).second)));
    };

    void submitWord(WordSPtr word);
};

void pushArcToChart(Column &column, ArcPtr arc) {
    for(uint k = 0; k < column.size(); ++k)  //TODO: Use a map to store/search these.
        if (arc->matches(column[k]))
            return;
    column.push_back(arc);
}

/*  Practical Earley Parsing:
    SCANNER: If ([A → ... • a ... , j ] is in S[i] && a == input[i+1]) , // i.e., if the rule is terminal and matches.
        add [A → ... a • ... , j ] to S[i+1].
    PREDICTOR: If [A → ... • B ... , j ] is in S[i] ,  // I.e., if the rule is non-terminal.
        add [B → •α, i] to S[i] for all rules B → α.
        If B is nullable, also add [A → ... B • ... , j ] to S[i].   // if we want null productions use this line.
    COMPLETER: If [A → ... •, j ] is in S[i] ,
        add [B → ... A • ... , k] to S[i] for all items [B → ... • A ... , k] in S[j] .
*/

void Burser::submitWord(WordSPtr word){
    uint i=crntPos++; string wordNorm="<EOT>";
    if(word){
        wordNorm=word->norm;
        chart.push_back(Column(i, wordNorm, word));
    }
    cout<<"SUBMITTED: "<<wordNorm<<"  col:"<<i<<"  "<<chart[i].size()<<" \n";
 //   print_chart(chart);
    for(uint j = 0; j < chart[i].size(); ++j) {
        ArcPtr stateItm = chart[i][j];
        if (stateItm->complete()) {  // (Complete)
            cout << "RULE COMPLETE:"<< stateItm->rule->asString()<<"\n";
            stateItm->end=i;
//commented to stop var-not-used warning            int numAltsAdded = preInfonate(chart, stateItm);
            //  TODO: if no alts were added, close this arc.
            for(Column::iterator k = chart[stateItm->start].begin(); k != chart[stateItm->start].end(); ++k ) {
                if (!(*k)->complete() && (*k)->nextTerm() == stateItm->rule->head) {
                   pushArcToChart(chart[i],ArcPtr(new bArc((*k)->start,(*k)->rule, (*k)->dotPos+1, (*k)->start)) );
                }
            }
        } else {
            string term=stateItm->nextTerm();
            if (term[0]!='$' && term[0]!='%') { // For terminals (Scan)
                if (word && word->norm == term) {// cout<<"MATCH:"<<term<<"\n";
                    pushArcToChart(chart[i+1], ArcPtr(
                        new bArc(i, stateItm->rule, stateItm->dotPos+1, i+1)
                        ));
                }
            } else { // Explode non-terminal symbol   (Predict)
               // cout << "EVALUATING: "<<term<<"  "<< word.get() <<" for '"<<wordNorm<<"'...\n";
                if(term[0]=='%'){
                    if(!word) continue;
                    bool wordIsOK=false; int newHeadID=0;
                    if(term=="%determiner"){
                        if(word->flags1&wfHasDetSense){ wordIsOK=true; newHeadID=auto_Det;}
                    } else if(term=="%modifier"){
                        if(word->flags1&(wfHasNounSense+wfHasAdjSense+wfHasIntesifierSense)){ wordIsOK=true; newHeadID=auto_Mod;}
                    } else if(term=="%noun"){ cout<<"NOUN "<<word->norm<<"  "<<word->flags1<<"\n";
                        if(word->flags1&wfHasNounSense){ wordIsOK=true; newHeadID=auto_N;}
                    } else if(term=="%verb"){ cout<<"VERB "<<word->flags1<<"\n";
                        if(word->flags1&wfHasVerbSense){ wordIsOK=true; newHeadID=auto_V;}
                    } else if(term=="%adjective"){ cout<<"ADJ "<<"\n";
                        if(word->flags1&wfHasAdjSense){ wordIsOK=true; newHeadID=auto_Adj;}
                    } else if(term=="%adverb"){ cout<<"ADV "<<"\n";
                        if(word->flags1&wfHasAdvSense){ wordIsOK=true; newHeadID=auto_Adv;}
                    } else if(term=="%preposition"){
                        if(word->flags1&wfHasPrepositionSense){ wordIsOK=true; newHeadID=auto_Prep;}
                    }
                    if(wordIsOK){
                        cout<<"     "<< word->altDefs.size()<<"\n";
                        pushArcToChart(chart[i], ArcPtr(new bArc(i, grammar->addRule(0,newHeadID,1,term.c_str(), word->norm.c_str()))));
                    }
                } else {// cout<<"nxtRule\n";
                    RuleRange nextRules=grammar->rules.equal_range(term);
                    for(RuleItr ri=nextRules.first; ri!=nextRules.second; ++ri) {
                      //  cout << "     "<< (*(*ri).second).asString()<<"\n";
                        pushArcToChart(chart[i], ArcPtr(new bArc(i, (*ri).second)));
                    }
                }
            }
        }
    } // cout<<"at-done1\n";
    if(!word){ // Gather successful parse trees
        cout<<"GATHERING PARSE TREES "<<chart.back().size()<<"\n";
        chart.GatherTreeRoots(parses, start_symbol);
        if(parses.size()==0) cout<<"Parsing Failed: I don't understand what you entered\n";
    }
}

///////////////////////////////////////
// Word Identification Section

#define DEB_find(msg) {} //{cout<< msg;}

uint getPOS_Sense(WordSPtr wordWithPOS){
    uint wordClass = (wordWithPOS->flags2 & maskWordClass);
    if(wordClass == wfNoun) return wfHasNounSense;
    if(wordClass == wfVerb) return wfHasVerbSense;
    if(wordClass == wfAdj)  return wfHasAdjSense;
    if(wordClass == wfAdv)  return wfHasAdvSense;
    return 0;
}

int FunctionWordFlags(WordSPtr tag){
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
DEB_find("  TRY: "<<wrdToParse<<"   \n")
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
            DEB_find("     PUSHING:"<<crntChoice->norm<<"\n")
            (*resultList)[pos].push_back(crntChoice);
            searchLen=choiceLen+1;
        } else searchLen=matchLen+1;
        if(matchLen <= 0) {break;}

    }
}

typedef vector<WordList> wordListVec;
typedef wordListVec::iterator wordListVecItr;

int parseWord(WordSMap *wordLib, const string &wrdToParse, const string &scopeID, wordListVec *resultList){
    DEB_find("\nFIND: "<<wrdToParse<<"   \n")
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
        DEB_find("\nSPELLING-RULE: "<<msg<<"\n")   \
        modToParse=(str); modLen=modToParse.length();                             \
        for(sPos=0; sPos<=modLen; ++sPos) alts[sPos].clear();                      \
        farPos=parseWord(wordLibrary, modToParse, scopeID, &alts);               \
        if(farPos==modLen) foundPath=true;                                        \
    } }

/* findDefinitions(WordSPtr words).
 * INPUT:
 *   The main data going into this is the words field of the argument. i.e. words->words.
 *   It is a list of the words in the words chain. So if I types into clip "the big eye opening preenactments find."
 *   then words->words would contain 6 items. One for each of those words.
 *
 * OUTPUT:
 *   Afterwards each word will have been examined and classified.
 *   'the' will have been marked as the correct function word.
 *   'big' will have had its definition field pointing to the infon for &big.
 *   'eye' and 'opening' will have been moved under a new WordS whose defintion will point to the infon for "eye-opening".
 *   'preenactments' (if not found in the library) will have been decomposed into pre, enact, ment and s.
 *      The ending 's' will mean that the word's wfForm_S_ES flag is set (i.e., this is either plural or a -s verb)
 *   Other plural forms will cause the word to be marked as plural.
 *   Words with 's or s' will have wfIsMarkedPossessive set. Similar for -es, -ed, -ing.
 *   A string of words like "seven hundred and twenty five" will have become a number (725)
 *   If a word does not have a definition, the argument words will have its wfErrorInAChildWord flag set.
 *   If there are multiple definitions they will listed under altDefs. Definition may then be null.
 */

void XlaterENGLISH::findDefinitions(WordS& words){
    UnicodeString ChainText; tagChainToString(&words, &ChainText);
    WordSPtr crntChoice=0; uint wordFlags=0;
    WordListItr WLi;
    string scopeID=words.key.substr(words.key.find('%')+1);
    for(WordListItr crntWrd=words.words.begin(); crntWrd!=words.words.end();){
        WordListItr tmpWrd; wordFlags=0;
        uint numWordsInCompound=0, numWordsInChosen=0, scopeScore=0; crntChoice=0;
        string trial="", wordKey="";
        if((*crntWrd)->sysType>=wstNumOrd) {++crntWrd; continue;} // It's a number. Don't try to find it.
        for(tmpWrd=crntWrd; tmpWrd!=words.words.end() && numWordsInCompound++ < maxWordsInCompound; tmpWrd++){
            if((*tmpWrd)->sysType>=wstNumOrd) break; // Numbers cannot be in the compound words (yet).
            if(numWordsInCompound>1) wordKey.append("-");
            wordKey.append((*tmpWrd)->norm);
      //tmp      cout<<"#######>"<<wordKey<<"\t\t"<<scopeID<<"\n";
            WordSMap::iterator trialItr=wordLibrary->wrappedLowerBound(wordKey, this);
            if(trialItr==wordLibrary->end()) break;
            trial=trialItr->first;
            bool stopSearch=true;
            int keyLen=wordKey.length();
        //tmp    cout<<"TRIAL: '"<<trial<<"'\n";
            while(trial.substr(0,keyLen) == wordKey){
                stopSearch=false;
                if(trial[keyLen]=='%'){//cout<<"\tMATCH: "<<trial<<"  "<<trialItr->second->flags2<<"\n";
                    uint newScopeScore=calcScopeScore(scopeID, trial.substr(keyLen+1));
                    if(newScopeScore > scopeScore) {
                        (*crntWrd)->altDefs.clear();  //tmp cout<<"CLEAR-X! "<<*crntWrd<<"\n";
                        scopeScore=newScopeScore; //tmp cout<<"PUSHING-X:"<<trialItr->second<<"\n";
                        (*crntWrd)->altDefs.push_back(trialItr->second);
                        (*crntWrd)->flags1 |= getPOS_Sense(trialItr->second);
                        numWordsInChosen=numWordsInCompound;
                    }else if(newScopeScore == scopeScore) {
                        if(numWordsInCompound>numWordsInChosen){
                            (*crntWrd)->altDefs.clear(); //tmp cout<<"CLEAR!\n";
                            numWordsInChosen=numWordsInCompound;
                        }//tmp cout<<"PUSHING "<<*crntWrd<<"\n";
                        (*crntWrd)->altDefs.push_back(trialItr->second);
                        (*crntWrd)->flags1 |= getPOS_Sense(trialItr->second);
                    }
                } else cout<<"\tNO-MATCH: "<<trial<<"\n";
                if(++trialItr != wordLibrary->end()) trial=trialItr->first; else break;
            }
            if(stopSearch) break;
        }

        if((*crntWrd)->altDefs.empty()){
            (*crntWrd)->baseForm=(*crntWrd)->norm;
            if(tagIsMarkedPossessive(*crntWrd)){}
            wordFlags=FunctionWordFlags(*crntWrd);
            if(wordFlags) {
                (*crntWrd)->flags1=wordFlags;
                if(wordFlags&wfCanStartNum){
                    UErrorCode err=U_ZERO_ERROR;
cout<<"NUMBER: "<<(*crntWrd)->norm<<"  ("<<(*crntWrd)->offsetInSource<<")\n";

                    RuleBasedNumberFormat* formatter= new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale::getUS(), err); // <-- It crashes here.
cout<<"AT AAA\n";
                    Formattable parseResult; ParsePosition parsePos=(*crntWrd)->offsetInSource; // offset likely NOT CORRECT.
                    formatter->parse(ChainText, parseResult, parsePos);
                    delete formatter;
cout<<"NUMBER WAS:"<<parseResult.getLong()<<".\n";
                    // (*crntWrd)->altDefs.push_back(XXX); // crntChoice=XXX;
                    // (*crntWrd)->flags1 |= hasNumberSense; (It SHOULD distinguish between ord and card.)
                    // numWordsInChosen=YYY;
                }
            }else { // Try parsing inside the word for prefixes, suffixes, etc.
            // Here we look for possessives, plurals, verb-forms, affixes, NVAJF, Contracted verbs.
            // We should handle alternate spellings such as UK/US, before a suffix, defined "runs" but uses "running".
                string wrdToParse=(*crntWrd)->baseForm;
                uint wrdLen=wrdToParse.length();
                string modToParse; uint foundPath=0, sPos, modLen, farPos=0; // These are used in tryParse macro.
                wordListVec alts(wrdLen+5,WordList()); // The 5 is to have room for modified spellings.
                uint furthestPos=parseWord(wordLibrary, wrdToParse, scopeID, &alts);
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
                        DEB_find(pre2Char<<", "<<pre1Char<<", "<<suf1Char<<", "<<suf2Char<<" ["<<prePart<<"-"<<postPart<<"]  ")
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
                        if(foundPath) DEB_find("FOUND-PATH\n") else DEB_find("No path\n")
                    }
                } else {modToParse=wrdToParse; modLen=wrdLen; foundPath=true;}
                if(foundPath) {// Extract a path
                    DEB_find("Extracting A Path: \n")
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
                    for(WLi=crntWord.words.begin(); WLi != crntWord.words.end(); ++WLi){DEB_find((*WLi)->norm << "-")}
                    string lastAffix=(*crntWord.words.back()).norm;
                    if(lastAffix=="s" || lastAffix=="es"){crntWord.flags1|=wfForm_S_ES;}
                    else if(lastAffix=="ing"){crntWord.flags1|=wfForm_ING;}
                    else if(lastAffix=="ed"){crntWord.flags1|=wfForm_ED;}

                    else if(lastAffix=="est") {crntWord.wordDegree=dSuperlative;}
                    else if(lastAffix=="er")  {crntWord.wordDegree=dComparative;}

                    else if(lastAffix=="'ll"){} // -will
                    else if(lastAffix=="'d"){}  // -had -would
                    else if(lastAffix=="'ve"){}  // -have
                    else if(lastAffix=="'re"){}  // -are
                    else if(lastAffix=="'m"){}  // I'm
                    else if(lastAffix=="'ll've"){} // -will have
                    else if(lastAffix=="'d've"){}  // -had have -would have
                    // NOW Construct definition  and push it to altDefs.
                }
            }
        }
        if(!wordFlags){ // If this isn't a function word...
            if((*crntWrd)->altDefs.empty()) {
                words.flags1|=wfErrorInAChildWord;
                cout<< ((string)"MESG: What does "+(*crntWrd)->norm+" mean?\n");
                crntWrd++;
            } else {
                if(numWordsInChosen>1){ // If more than one word was consumed, move them into a new parent.
                    WordSPtr parent=WordSPtr(new WordS);
                    parent->altDefs=(*crntWrd)->altDefs;
                    parent->flags1=(*crntWrd)->flags1;
                    words.words.insert(crntWrd, parent);
                    WordListItr wrd=crntWrd;
                    for(uint i=0; i<numWordsInChosen; ++i){
                        if(i>0) parent->norm+=" ";
                        parent->norm+=(*wrd)->norm;
                        parent->words.push_back(*wrd);
                        wrd=words.words.erase(wrd);
                    }
                    crntWrd=wrd;
                } else {++crntWrd;}
            }
        } else ++crntWrd;
    }
}

void XlaterENGLISH::stitchAndDereference(WordS& text){
    infon* context=0;
    Burser burser(&EnglishGrammarRules, context);
    for(WordListItr WLi=text.words.begin(); WLi!=text.words.end(); ++WLi){
        burser.submitWord(*WLi);
    }
    burser.submitWord(0); // End the Parsing
    BNodes forest;
 //   infon* result=0;
    for(ArcPtrs::iterator arc=burser.parses.begin(); arc != burser.parses.end(); ++arc){
        burser.chart.BuildTrees(&forest, (*arc));
        burser.chart.printForest(forest);
 //       result=infonate(chart, forest);
        cout<<". . . . . . . . . . . . . . . . . . . . . . . . . . .\n";
    }


  //  WordListItr WLI=text.words.begin(); //++WLI;
  //  text.definition=(*(*WLI)->altDefs.begin())->definition;
}

infon* XlaterENGLISH::infonate(WordS& text){
 //tmp   cout<<"NORM:"<<(*text.words.begin())<<(*text.words.begin())->norm<<"   def:"<<(*(*text.words.begin())->altDefs.begin())->norm<<"\n";
    if (text.definition) return text.definition;
    WordListItr WLI=text.words.begin(); //++WLI;
    text.definition=(*(*WLI)->altDefs.begin())->definition;
 //tmp   cout<<"DEF:"<<text.definition<<"\n";
    return text.definition;
}
