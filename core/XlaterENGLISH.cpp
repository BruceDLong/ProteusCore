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

void tagChainToString(WordS* tags, UnicodeString &strOut){
    strOut=""; WordS* t=tags;
    do{
        t->sourceStr = &strOut; t->offsetInSource=strOut.length();
        strOut+= strOut.fromUTF8(t->norm+" ");
        t=t->next;
    } while(t!=tags);
}

WordS* XlaterENGLISH::ReadLanguageWord(QParser *parser, icu::Locale &locale){ // Reads a 'word' consisting of a number or an alphabetic+hyphen+appostrophies tag
    char tok=parser->Peek();
    if(isdigit(tok) || tok=='-') {getBufs(isCardinalOrOrdinal(parser->peek()),parser);}
    else if(isalpha(tok)){getBufs(isEngWordChar(parser->peek()),parser);}
    else return 0;
    if(parser->buf[0]==0) return 0;
    WordS* tag=new WordS;
    tag->tag=parser->buf; tag->locale=locale.getBaseName();
    if(isalpha(tok)){
        tagNormalizer->normalize(UnicodeString::fromUTF8(tag->tag), uErr).toUTF8String(tag->norm);
        if(tagIsBad(tag->norm, tag->locale.c_str())) throw "Illegal tag being defined";
    } else {
        tag->sysType=wstNumCard;
        if(!validNumberSyntax(&tag->tag)) throw "Invalid number syntax";
    }
    return tag;
}

WordS* XlaterENGLISH::ReadTagChain(QParser *parser, icu::Locale &locale){ // Reads a phrase that ends at a non-matching character or a period that isn't in a number.
    WordS *topTag=0, *nxtTag=0;
    while((nxtTag=ReadLanguageWord(parser, locale))){
        if(topTag==0) {topTag = nxtTag->next = nxtTag->prev = nxtTag;}
        else {nxtTag->next=topTag; nxtTag->prev=topTag->prev; topTag->prev->next=nxtTag; topTag->prev=nxtTag;}
    }
    parser->nxtTokN(1,".");
    return topTag;
}

infon* XlaterENGLISH::tags2Proteus(WordS* words){   // Converts a list of words read by ReadTagChain() into an infon and returns a pointer to it.
    findDefinitions(words);
    stitchAndDereference(words);
    infon* proteusCode = infonate(words);
    return proteusCode;
}

WordS* XlaterENGLISH::proteus2Tags(infon* proteus){  // Converts an infon to a tag chain.
    return 0;
}
typedef map<string, int> WordMap;
typedef WordMap::iterator WordMapIter;

enum funcWordFlags {cardinalNum=8};
WordMap functionWords = {   // int is a WordClass
    // Definite determiners
        {"the", 0}, {"this", 0}, {"that", 0}, {"these", 0}, {"those", 0},   // These and Those are plural of this and that
        {"which", 0}, {"whichever", 0}, {"what", 0}, {"whatever", 0},       // These usually start questions
        // Other definite determiners are possessives inclusing possessive pronouns: my, your, his, her, its, our, their, whose.

    // Indefinite determiners
        {"a", 0}, {"an", 0},       // Use with singular, countable nouns.
        {"some", 0}, {"any", 0},   // Use with plural and non-countable nouns.

    // Quantifying determiners // I need to define a "quantifier phrase"
        {"much", 0}, {"many", 0}, {"more", 0}, {"most", 0}, {"little", 0}, {"few", 0}, {"less", 0}, {"fewer", 0}, {"least", 0}, {"fewest", 0},
        {"all", 0}, {"both", 0}, {"enough", 0}, {"no", 0},

        {"zero", 8}, {"one", 8}, {"two", 8}, {"three", 8}, {"four", 8}, {"five", 8}, {"six", 8}, {"seven", 8}, {"eight", 8}, {"nine", 8},
        {"ten", 8}, {"eleven", 8}, {"twelve", 8}, {"thirteen", 8}, {"fourteen", 8}, {"fifteen", 8}, {"sixteen", 8}, {"seventeen", 8}, {"eighteen", 8}, {"nineteen", 8},
        {"twenty", 8}, {"thirty", 8}, {"forty", 8}, {"fifty", 8}, {"sixty", 8}, {"seventy", 8}, {"eighty", 8}, {"ninety", 8},

        {"each", 0}, {"every", 0}, {"either", 0}, {"neither", 0},   // 'every' can be 'almost every' and some other forms.

        // when compound function words are supported:
        {"all the", 0}, {"some of the", 0}, {"some of the many", 0},  // also: "5 liters of"
        {"a pair of", 0}, {"half", 0}, {"half of", 0}, {"double", 0}, {"more than", 0}, {"less than", 0}, // twice as many as... ,  a dozen,
        {"a lot of", 0}, {"lots of", 0}, {"plenty of", 0}, {"a great deal of", 0}, {"tons of", 0}, {"a few", 0}, {"a little", 0},
        {"several", 0}, {"a couple", 0}, {"a number of", 0}, {"a whole boat load of", 0}, //... I need to find a pattern for these

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
    {"aboard", 0}, {"about", 0}, {"above", 0}, {"across", 0}, {"after", 0}, {"against", 0}, {"along", 0}, {"alongside", 0}, {"amid", 0}, {"amidst", 0},
    {"among", 0}, {"amongst", 0}, {"anti", 0}, {"around", 0}, {"as", 0}, {"astride", 0}, {"at", 0}, {"atop", 0}, {"bar", 0}, {"barring", 0},
    {"before", 0}, {"behind", 0}, {"below", 0}, {"beneath", 0}, {"beside", 0}, {"besides", 0}, {"between", 0}, {"beyond", 0}, {"but", 0}, {"by", 0},
    {"circa", 0}, {"concerning", 0}, {"considering", 0}, {"counting", 0}, {"despite", 0}, {"down", 0}, {"during", 0}, {"except", 0}, {"excepting", 0}, {"excluding", 0},
    {"following", 0}, {"for", 0}, {"from", 0}, {"given", 0}, {"gone", 0}, {"in", 0}, {"including", 0}, {"inside", 0}, {"into", 0}, {"like", 0},
    {"near", 0}, {"of", 0}, {"off", 0}, {"on", 0}, {"onto", 0}, {"opposite", 0}, {"outside", 0}, {"over", 0}, {"past", 0}, {"since", 0},
    {"than", 0}, {"through", 0}, {"thru", 0}, {"throughout", 0}, {"to", 0}, {"touching", 0}, {"toward", 0}, {"towards", 0}, {"under", 0}, {"underneath", 0},
    {"unlike", 0}, {"until", 0}, {"up", 0}, {"upon", 0}, {"versus", 0}, {"via", 0}, {"with", 0}, {"within", 0}, {"without", 0}, {"worth", 0},

    {"according to", 0}, {"ahead of", 0}, {"along with", 0}, {"apart from", 0}, {"as for", 0}, {"aside from", 0}, {"as per", 0}, {"as to", 0}, {"as well as", 0}, {"away from", 0},
    {"because of", 0}, {"but for", 0}, {"by means of", 0}, {"close to", 0}, {"contrary to", 0}, {"depending on", 0}, {"due to", 0}, {"except for", 0}, {"forward of", 0}, {"further to", 0},
    {"in addition to", 0}, {"in between", 0}, {"in case of", 0}, {"in face of", 0}, {"in favor of", 0}, {"in front of", 0}, {"in lieu of", 0}, {"in spite of", 0}, {"instead of", 0}, {"in view of", 0},
    {"irrespective of", 0}, {"near to", 0}, {"next to", 0}, {"on account of", 0}, {"on behalf of", 0}, {"on board", 0}, {"on to", 0}, {"on top of", 0}, {"opposite to", 0}, {"opposite of", 0},
    {"other than", 0}, {"out of", 0}, {"outside of", 0}, {"owing to", 0}, {"prior to", 0}, {"regardless of", 0}, {"thanks to", 0}, {"together with", 0}, {"up against", 0}, {"up to", 0},
    {"up until", 0}, {"one", 0}, {"two", 0}, {"three", 0}, {"four", 0}, {"five", 0}, {"six", 0}, {"seven", 0}, {"eight", 0}, {"nine", 0},
    {"zero", 0}, {"with reference to", 0}, {"with regard to", 0},

    // Conjunctions
    {"and", 0}, {"but", 0}, {"or", 0},  // consider either/or, neither/nor
    {"although", 0}, {"after", 0}, {"as", 0}, {"before", 0}, {"if", 0}, {"since", 0}, {"that", 0},
    {"unless", 0}, {"until", 0}, {"when", 0}, {"whenever", 0}, {"whereas", 0}, {"while", 0},

    {"as long as", 0}, {"as soon as", 0}, {"as though", 0}, {"except that", 0},
    {"in order that", 0}, {"provided that", 0}, {"so long as", 0}, {"such that", 0}, {"four", 0}, {"five", 0}, {"six", 0}, {"seven", 0}, {"eight", 0}, {"nine", 0},

    // Negotiators
    {"hi", 0}, {"hey", 0}, {"hey there", 0}, {"greetings", 0}, {"good-morning", 0},
    {"bye", 0}, {"bye-bye", 0}, {"goodbye", 0}, {"goodnight", 0}, {"see-ya", 0},
    {"excuse me", 0}, {"sorry", 0}, {"thanks", 0}, {"no thank you", 0}, {"oops", 0}, {"wow", 0}, {"yeah", 0}, {"oh", 0}, {"pardon me", 0}, {"cheers", 0},
    {"um", 0}, {"er", 0}, {"umm-hmm", 0}, {"ouch", 0}

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
map<string, infon*> suffixesFront = {  // non-plural-related suffixes
    {"a", 0}, {"ability", 0}, {"able", 0}, {"ably", 0}, {"ac", 0}, {"acea", 0}, {"aceae", 0}, {"acean", 0}, {"aceous", 0}, {"ad", 0},
    {"ade", 0}, {"aemia", 0}, {"age", 0}, {"agog", 0}, {"agogue", 0}, {"aholic", 0}, {"al", 0}, {"ales", 0}, {"algia", 0}, {"amine", 0},
    {"an", 0}, {"ana", 0}, {"anae", 0}, {"ance", 0}, {"ancy", 0}, {"androus", 0}, {"andry", 0}, {"ane", 0}, {"ant", 0}, {"ar", 0}, {"arch", 0},
    {"archy", 0}, {"ard", 0}, {"aria", 0}, {"arian", 0}, {"arium", 0}, {"art", 0}, {"ary", 0}, {"ase", 0}, {"ate", 0}, {"athon", 0},
    {"ation", 0}, {"ative", 0}, {"ator", 0}, {"atory", 0}, {"biont", 0}, {"biosis", 0}, {"blast", 0}, {"cade", 0}, {"caine", 0}, {"carp", 0},
    {"carpic", 0}, {"carpous", 0}, {"cele", 0}, {"cene", 0}, {"centric", 0}, {"cephalic", 0}, {"cephalous", 0}, {"cephaly", 0}, {"chore", 0},
    {"chory", 0}, {"chrome", 0}, {"cide", 0}, {"clast", 0}, {"clinal", 0}, {"cline", 0}, {"clinic", 0}, {"coccus", 0}, {"coel", 0}, {"coele", 0},
    {"colous", 0}, {"cracy", 0}, {"crat", 0}, {"cratic", 0}, {"cratical", 0}, {"cy", 0}, {"cyte", 0}, {"dale", 0}, {"derm", 0}, {"derma", 0},
    {"dermatous", 0}, {"dom", 0}, {"drome", 0}, {"dromous", 0}, {"eae", 0}, {"eaux", 0}, {"ectomy", 0}, {"ed", 0}, {"ee", 0}, {"eer", 0},
    {"ein", 0}, {"eme", 0}, {"emia", 0}, {"en", 0}, {"ence", 0}, {"enchyma", 0}, {"ency", 0}, {"ene", 0}, {"ent", 0}, {"eous", 0}, {"er", 0},
    {"ergic", 0}, {"ergy", 0}, {"es", 0}, {"escence", 0}, {"escent", 0}, {"ese", 0}, {"esque", 0}, {"ess", 0}, {"est", 0}, {"et", 0},
    {"eth", 0}, {"etic", 0}, {"ette", 0}, {"ey", 0}, {"facient", 0}, {"faction", 0}, {"fer", 0}, {"ferous", 0}, {"fic", 0}, {"fication", 0},
    {"fid", 0}, {"florous", 0}, {"fold", 0}, {"foliate", 0}, {"foliolate", 0}, {"form", 0}, {"fuge", 0}, {"ful", 0}, {"fy", 0}, {"gamous", 0},
    {"gamy", 0}, {"gate", 0}, {"gen", 0}, {"gene", 0}, {"genesis", 0}, {"genetic", 0}, {"genic", 0}, {"genous", 0}, {"geny", 0}, {"gnathous", 0},
    {"gon", 0}, {"gony", 0}, {"gram", 0}, {"graph", 0}, {"grapher", 0}, {"graphy", 0}, {"gyne", 0}, {"gynous", 0}, {"gyny", 0}, {"hood", 0},
    {"ia", 0}, {"ial", 0}, {"ian", 0}, {"iana", 0}, {"iasis", 0}, {"iatric", 0}, {"iatrics", 0}, {"iatry", 0}, {"ibility", 0}, {"ible", 0},
    {"ic", 0}, {"icide", 0}, {"ician", 0}, {"ics", 0}, {"id", 0}, {"idae", 0}, {"ide", 0}, {"ie", 0}, {"ify", 0}, {"ile", 0}, {"in", 0},
    {"ina", 0}, {"inae", 0}, {"ine", 0}, {"ineae", 0}, {"ing", 0}, {"ini", 0}, {"ion", 0}, {"ious", 0}, {"isation", 0}, {"ise", 0}, {"ish", 0},
    {"ism", 0}, {"ist", 0}, {"istic", 0}, {"istical", 0}, {"istically", 0}, {"ite", 0}, {"itious", 0}, {"itis", 0}, {"ity", 0}, {"ium", 0},
    {"ive", 0}, {"ix", 0}, {"ization", 0}, {"ize", 0}, {"kin", 0}, {"kinesis", 0}, {"kins", 0}, {"latry", 0}, {"le", 0}, {"lepry", 0}, {"less", 0},
    {"let", 0}, {"like", 0}, {"ling", 0}, {"lite", 0}, {"lith", 0}, {"lithic", 0}, {"log", 0}, {"logue", 0}, {"logic", 0}, {"logical", 0},
    {"logist", 0}, {"logy", 0}, {"ly", 0}, {"lyse", 0}, {"lysis", 0}, {"lyte", 0}, {"lytic", 0}, {"lyze", 0}, {"mancy", 0}, {"mania", 0},
    {"meister", 0}, {"ment", 0}, {"mer", 0}, {"mere", 0}, {"merous", 0}, {"meter", 0}, {"metric", 0}, {"metrics", 0}, {"metry", 0}, {"mire", 0},
    {"mo", 0}, {"morph", 0}, {"morphic", 0}, {"morphism", 0}, {"morphous", 0}, {"most", 0}, {"mycete", 0}, {"mycetes", 0}, {"mycetidae", 0},
    {"mycin", 0}, {"mycota", 0}, {"mycotina", 0}, {"n't", 0}, {"nasty", 0}, {"ness", 0}, {"nik", 0}, {"nomy", 0}, {"nomics", 0}, {"o", 0},
    {"ode", 0}, {"odon", 0}, {"odont", 0}, {"odontia", 0}, {"oholic", 0}, {"oic", 0}, {"oid", 0}, {"oidea", 0}, {"oideae", 0}, {"ol", 0}, {"ole", 0},
    {"oma", 0}, {"ome", 0}, {"omics", 0}, {"on", 0}, {"one", 0}, {"ont", 0}, {"onym", 0}, {"onymy", 0}, {"opia", 0}, {"opsida", 0}, {"opsis", 0},
    {"opsy", 0}, {"or", 0}, {"orama", 0}, {"ory", 0}, {"ose", 0}, {"osis", 0}, {"otic", 0}, {"otomy", 0}, {"ous", 0}, {"para", 0}, {"parous", 0},
    {"path", 0}, {"pathy", 0}, {"ped", 0}, {"pede", 0}, {"penia", 0}, {"petal", 0}, {"phage", 0}, {"phagia", 0}, {"phagous", 0}, {"phagy", 0},
    {"phane", 0}, {"phasia", 0}, {"phil", 0}, {"phile", 0}, {"philia", 0}, {"philiac", 0}, {"philic", 0}, {"philous", 0}, {"phobe", 0}, {"phobia", 0},
    {"phobic", 0}, {"phone", 0}, {"phony", 0}, {"phore", 0}, {"phoresis", 0}, {"phorous", 0}, {"phrenia", 0}, {"phyll", 0}, {"phyllous", 0},
    {"phyceae", 0}, {"phycidae", 0}, {"phyta", 0}, {"phyte", 0}, {"phytina", 0}, {"plasia", 0}, {"plasm", 0}, {"plast", 0}, {"plastic", 0},
    {"plasty", 0}, {"plegia", 0}, {"plex", 0}, {"ploid", 0}, {"pod", 0}, {"pode", 0}, {"podous", 0}, {"poieses", 0}, {"poietic", 0}, {"pter", 0},
    {"punk", 0}, {"rrhea", 0}, {"ric", 0}, {"ry", 0}, {"s", 0}, {"scape", 0}, {"scope", 0}, {"scopy", 0}, {"scribe", 0}, {"script", 0}, {"sect", 0},
    {"sepalous", 0}, {"ship", 0}, {"some", 0}, {"speak", 0}, {"sperm", 0}, {"sphere", 0}, {"sporous", 0}, {"st", 0}, {"stasis", 0}, {"stat", 0},
    {"ster", 0}, {"stome", 0}, {"stomy", 0}, {"taxis", 0}, {"taxy", 0}, {"tend", 0}, {"th", 0}, {"therm", 0}, {"thermal", 0}, {"thermic", 0},
    {"thermy", 0}, {"thon", 0}, {"thymia", 0}, {"tion", 0}, {"tome", 0}, {"tomy", 0}, {"tonia", 0}, {"trichous", 0}, {"trix", 0}, {"tron", 0},
    {"trophic", 0}, {"trophy", 0}, {"tropic", 0}, {"tropism", 0}, {"tropous", 0}, {"tropy", 0}, {"tude", 0}, {"ture", 0}, {"ty", 0}, {"ular", 0},
    {"ule", 0}, {"ure", 0}, {"urgy", 0}, {"uria", 0}, {"uronic", 0}, {"urous", 0}, {"valent", 0}, {"virile", 0}, {"vorous", 0}, {"ward", 0},
    {"wards", 0}, {"ware", 0}, {"ways", 0}, {"wear", 0}, {"wide", 0}, {"wise", 0}, {"worthy", 0}, {"xor", 0}, {"y", 0}, {"yl", 0}, {"yne", 0},
    {"zilla", 0}, {"zoic", 0}, {"zoon", 0}, {"zygous", 0}, {"zyme", 0}
};

map<string, infon*> EnglishSuffixes;

bool XlaterENGLISH::loadLanguageData(string dataFilename){
    string reversedKey;
    // Here we copy suffixes into a new map where they are spelled backwards.
    // This makes it faster to search for things at the end of the word because when backwards they are at the beginning.
    EnglishSuffixes.clear();
    for(map<string, infon*>::iterator S = suffixesFront.begin(); S!=suffixesFront.end(); ++S){
        EnglishSuffixes.insert(pair<string, infon*>(string(S->first.rbegin(), S->first.rend()), S->second));
    }
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

bool tagIsMarkedPossessive(WordS *tag){
    int tagLen=tag->baseForm.length();
    if(tagLen>2){
        string ending=tag->baseForm.substr(tagLen-2, 2);
        if(ending=="'s"){
            WordMapIter w = apostrophics.find(tag->baseForm);
            if(w->second==amEndingInApostrophieS){return false;}
            else if(w->second==amEndsInS_MayBePossessive){return false;} // These could be possessives. TODO: try using grammer/models to decide.
            tag->baseForm=tag->baseForm.substr(0,tagLen-2);
            tag->isMarkedPossessive=true;
        }else if(ending=="s'"){
            tag->baseForm=tag->baseForm.substr(0,tagLen-1);
            tag->isMarkedPossessive=true;
        }
    }
    return true;
}

int FunctionWordClass(WordS *tag){
    WordMapIter w = functionWords.find(tag->baseForm);
    if(w!=functionWords.end())  return w->second;
    return 0;
}

void XlaterENGLISH::findDefinitions(WordS *tags){
    UnicodeString txt=""; // UErrorCode err=U_ZERO_ERROR; int32_t Result=0;
    UnicodeString ChainText; tagChainToString(tags, ChainText);
//    RuleBasedNumberFormat parser(URBNF_SPELLOUT, Locale::getEnglish(), err);
//    Formattable result(Result);
 //   ParsePosition cursor=0, last=txt.length();
    WordS* crntTag=tags;
    do{
        crntTag->baseForm = crntTag->norm;
        if(tagIsMarkedPossessive(crntTag)){}
cout<< crntTag->baseForm << "-";

        string strToParse=crntTag->baseForm;
        int tagLen=strToParse.length();
        int endPos=1;

        while(endPos<=tagLen){
            string strPart=strToParse.substr(0,endPos);
        }

        int funcWordClass=FunctionWordClass(crntTag);
        if(funcWordClass>0){
            if(funcWordClass==cardinalNum){
             /*   ParsePosition prevCursor=cursor;
                parser.parse(txt, result, cursor);
                if(cursor != prevCursor){
                    // package result into tag,
                    // delete other tags.
                    // update crntTag;
                    continue;
                }*/
            } else { // Handle the function word
            }
        } else { // Check if it is a content word?
            ////// Try compound word meanings
            // if whole tag-string is found
            // while(){tag-string+=tag->next->norm; try that.}

            //if not found, do this:
            ////// Try sub-word meanings
            // list prefixes + null
            // list null + suffixes
            // set rootWord // may be null
            // for each prefix+null
            //    for each suffix+null
            //       if(prefix-root-suffix is found) save this if it is larger than previous item saved.
        }
        crntTag=crntTag->next;
// In this function: mark plurals, possessive markers, POS, isName
    } while(crntTag!=tags);
} // TODO: validate apostrophie usage. Handle hyphens. Unspaced compounds like superman.

void XlaterENGLISH::stitchAndDereference(WordS *text){
}

infon* XlaterENGLISH::infonate(WordS *text){
    return 0;
}
