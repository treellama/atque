/* Wad.h

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

#ifndef WAD_H
#define WAD_H

#include "ferro/cstypes.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

class AIStream;
class AOStream;

namespace marathon
{
	class crc_ostream;
	
	class Wad
	{
		friend class Wadfile;
		friend std::ostream& operator<<(std::ostream&, const Wad&);
	public:
		enum {
			kEntryHeaderSize = 16,
			kEntryHeaderOldSize = 12
		};

		Wad() { }
		
		void Load(std::istream& stream);
		void Load(std::istream& stream, int16 entry_header_length);
		
		void AddChunk(uint32 tag, const std::vector<uint8>& data) { chunks_[tag] = data; }
		bool HasChunk(uint32 tag) const;
		const std::vector<uint8>& GetChunk(uint32 tag) const;
		void RemoveChunk(uint32 tag) { chunks_.erase(tag); }

		std::vector<uint32> GetTags() const;
		
		// size that will be output, not size in memory!
		int32 GetSize() const;
		void Save(crc_ostream& s) const;
		
	private:
		typedef std::map<uint32, std::vector<uint8> > chunk_map;
		chunk_map chunks_;
		
		struct EntryHeader
		{
			uint32 tag;
			int32 next_offset;
			int32 length;
			int32 offset;
			
			void Load(AIStream& stream, int16 entry_header_length);
			void Save(AOStream& stream);
		};
	};
}

#endif
