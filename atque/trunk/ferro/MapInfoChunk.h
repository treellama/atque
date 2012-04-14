/* MapInfoChunk.h

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

#ifndef MAPINFOCHUNK_H
#define MAPINFOCHUNK_H

#include "cstypes.h"

#include <string>
#include <vector>

namespace marathon
{
	class MapInfo
	{
	public:
		static const int kLevelNameLength = 64 + 2;
		enum { kTag = FOUR_CHARS_TO_INT('M','i','n','f') };
		
		MapInfo() { }
		MapInfo(const std::vector<uint8>& data) { Load(data); }
		
		void Load(const std::vector<uint8>&);

		int16 mission_flags() const { return _mission_flags; }
		int16 environment_flags() const { return _environment_flags; }
		std::string level_name() const { return std::string(_level_name); } 
		uint32 entry_point_flags() const { return _entry_point_flags; }
	private:
		int16 _environment_code;
		int16 _physics_model;
		int16 _song_index;
		int16 _mission_flags;
		int16 _environment_flags;
		// int16 unused[4];
		char _level_name[kLevelNameLength];
		uint32 _entry_point_flags;
	};
}

#endif
