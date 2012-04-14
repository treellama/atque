/* MapInfoChunk.cpp

   Copyright (C) 2008 by Gregory Smith
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
   
   This license is contained in the file "COPYING", which is included
   with this source code; it is available online at
   http://www.gnu.org/licenses/gpl.html
   
*/

#include "ferro/AStream.h"
#include "ferro/MapInfoChunk.h"

using namespace marathon;

void MapInfo::Load(const std::vector<uint8>& data)
{
	AIStreamBE s(&data[0], data.size());

	s >> _environment_code;
	s >> _physics_model;
	s >> _song_index;
	s >> _mission_flags;
	s >> _environment_flags;
	s.ignore(4 * 2);
	s.read(_level_name, kLevelNameLength);
	s >> _entry_point_flags;
}
