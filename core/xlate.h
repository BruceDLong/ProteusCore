///////////////////////////////////////////////////
//  xlate.h 1.0  Copyright (c) 2012 Bruce Long
//  xlate is a base class for language extensions to Proteus
/*  This file is part of the "Proteus Engine"
    The Proteus Engine is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
    The Proteus Engine is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with the Proteus Engine.  If not, see <http://www.gnu.org/licenses/>.
*/

using namespace std;

class xlate{
public:
  //  tags2Proteus(tagList tags);
 //   tagList proteus2Tags(infon* proteus);
    bool loadLanguageData(string dataFilename);
    bool unloadLanguageData();
    ~xlate(){unloadLanguageData();};
private:
    bool deWordPrefix();
    bool deSuffix();
};

