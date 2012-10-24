/////////////////////////////////////////////////
// wordUtils.cpp 1.0  Copyright (c) 2012 Bruce Long
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "wordUtils.h"
#include <map>
using namespace std;

typedef map<string, string> pluralMap;

pluralMap pluralToSingularMap;

//  C:   Classical usage

pluralMap singularToPluralMap = {
    // Irregulars ending with s
    {"corpus"          , "corpuses"},    {"corpus", "C:corpora"},
    {"opus"            , "opuses"},      {"opus", "C:opera"},
    //{"magnum opus"     , "magnum opuses|magna opera"},
    {"genus"           , "genera"},
    {"mythos"          , "mythoi"},
    {"penis"           , "penises"},     {"penis", "C:penes"},
    {"testis"          , "testes"},
    {"atlas"           , "atlases"},     {"atlas", "C:atlantes"},
    {"yes"             , "yeses"},
    //{"editio princeps" , "editiones principes"},
    {"starets"         , "startsy"},
    {"staretz"         , "startzy"},

    // Irregulars not ending in s
    {"child"       , "children"},
    {"brother"     , "brothers"},       {"brother", "C:brethren"},
    {"loaf"        , "loaves"},
    {"hoof"        , "hoofs"},          {"hoof", "C:hooves"},
    {"beef"        , "beefs"},          {"beef", "C:beeves"},
    {"thief"       , "thiefs"},         {"thief", "C:thieves"},
    {"money"       , "monies"},
    {"mongoose"    , "mongooses"},
    {"ox"          , "oxen"},
    {"cow"         , "cows"},           {"cow", "C:kine"},
    {"graffito"    , "graffiti"},
    //{"prima donna" , "prima donnas"},     {"prima donna", "C:prime donne"},
    {"octopus"     , "octopuses"},      {"octopus", "C:octopodes"},
    {"genie"       , "genies"},         {"genie", "C:genii"},
    {"ganglion"    , "ganglions"},      {"ganglion", "C:ganglia"},
    {"trilby"      , "trilbys"},
    {"turf"        , "turfs"},          {"turf", "C:turves"},
    {"numen"       , "numina"},
    {"atman"       , "atmas"},
    {"occiput"     , "occiputs"},       {"occiput", "C:occipita"},
    {"sabretooth"  , "sabretooths"},
    {"sabertooth"  , "sabertooths"},
    {"lowlife"     , "lowlifes"},
    {"flatfoot"    , "flatfoots"},
    {"tenderfoot"  , "tenderfoots"},
    {"Romany"      , "Romanies"},
    {"romany"      , "romanies"},
    {"Tornese"     , "Tornesi"},
    {"Jerry"       , "Jerrys"},
    {"jerry"       , "jerries"},
    {"Mary"        , "Marys"},
    {"mary"        , "maries"},
    {"talouse"     , "talouses"},
    {"blouse"      , "blouses"},
    {"Rom"         , "Roma"},
    {"rom"         , "roma"},
    {"carmen"      , "carmina"},
    {"cheval"      , "chevaux"},
    {"chervonetz"  , "chervontzi"},
    {"kuvasz"      , "kuvaszok"},
    {"felo"        , "felones"},
    //{"put-off"     , "put-offs"},
    //{"set-off"     , "set-offs"},
    //{"set-out"     , "set-outs"},
    //{"set-to"      , "set-tos"},
    //{"brother-german" , "brothers-german"}, {"brother-german", "C:brethren-german"},
    //{"studium generale" , "studia generali"}

};

void initWordUtils(){
    pluralToSingularMap.clear();
    for(pluralMap::iterator S = singularToPluralMap.begin(); S!=singularToPluralMap.end(); ++S){
        string singular=S->first;
        string plural=S->second;
        string mode="";
        if(plural[1]==':') {mode=plural[0]; plural=plural.substr(2); singular=mode+":"+singular;}
        pluralToSingularMap.insert(pair<string, string>(plural, singular));
    }
}

wordUtilResult checkWhetherIrregularPlural(string *pluralIn, string *singularOut){


}

bool fetchPluralForm(string *singularIn, string *pluralOut, bool useClassical){

}

int main(int argc, char **argv){

    return 0;
}
