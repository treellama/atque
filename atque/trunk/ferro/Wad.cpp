/* Wad.cpp

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

#include "AStream.h"
#include "ferro/Wad.h"
#include "ferro/Wadfile.h"

#include "ferro/MapInfoChunk.h"
#include "ferro/TerminalChunk.h"

#include <boost/assign/list_of.hpp>

using namespace marathon;

bool Wad::HasChunk(uint32 tag) const
{
	return chunks_.count(tag);
}

const std::vector<uint8>& Wad::GetChunk(uint32 tag) const
{
	if (chunks_.count(tag))
	{
		return chunks_.find(tag)->second;
	}
	else
	{
		static std::vector<uint8> empty_vector;
		return empty_vector;
	}
}

std::vector<uint32> Wad::GetTags() const
{
	std::vector<uint32> v;
	for (chunk_map::const_iterator it = chunks_.begin(); it != chunks_.end(); ++it)
	{
		v.push_back(it->first);
	}
	
	return v;
}

void Wad::Load(std::istream& s)
{
	Load(s, kEntryHeaderSize);
}

void Wad::Load(std::istream& s, int16 entry_header_length)
{
	std::streampos start = s.tellg();

	EntryHeader header;
	do {
		// read the entry header
		std::vector<uint8> header_data(entry_header_length);
		s.read(reinterpret_cast<char*>(&header_data[0]), header_data.size());
		AIStreamBE header_stream(&header_data[0], header_data.size());
		header.Load(header_stream, entry_header_length);

		// load the tag data
		std::vector<uint8>& tag_data = chunks_[header.tag];
		tag_data.resize(header.length);
		s.read(reinterpret_cast<char *>(&tag_data.front()), tag_data.size());
		if (header.next_offset) 
			s.seekg(start + static_cast<std::streamoff>(header.next_offset));

	} while (header.next_offset);
}

int32 Wad::GetSize() const
{
	int32 size = 0;
	for (chunk_map::const_iterator it = chunks_.begin(); it != chunks_.end(); ++it)
	{
		if (it->second.size())
			size += it->second.size() + kEntryHeaderSize;
	}

	return size;
}

const std::vector<uint32> tag_save_list = boost::assign::list_of
			     (FOUR_CHARS_TO_INT('P','N','T','S'))
			     (FOUR_CHARS_TO_INT('L','I','N','S'))
			     (FOUR_CHARS_TO_INT('P','O','L','Y'))
			     (FOUR_CHARS_TO_INT('S','I','D','S'))
			     (FOUR_CHARS_TO_INT('L','I','T','E'))
			     (FOUR_CHARS_TO_INT('N','O','T','E'))
			     (FOUR_CHARS_TO_INT('O','B','J','S'))
			     (MapInfo::kTag)
			     (FOUR_CHARS_TO_INT('p','l','a','c'))
			     (FOUR_CHARS_TO_INT('m','e','d','i'))
			     (FOUR_CHARS_TO_INT('a','m','b','i'))
			     (FOUR_CHARS_TO_INT('b','o','n','k'))
			     (FOUR_CHARS_TO_INT('i','i','d','x'))
			     (FOUR_CHARS_TO_INT('E','P','N','T'))
			     (FOUR_CHARS_TO_INT('P','L','A','T'))
			     (TerminalChunk::kTag)
			     (FOUR_CHARS_TO_INT('M','N','p','x'))
			     (FOUR_CHARS_TO_INT('F','X','p','x'))
			     (FOUR_CHARS_TO_INT('P','R','p','x'))
			     (FOUR_CHARS_TO_INT('P','X','p','x'))
			     (FOUR_CHARS_TO_INT('W','P','p','x'))
			     (FOUR_CHARS_TO_INT('S','h','P','a'));
	
void Wad::Save(crc_ostream& s) const
{
	// build an array of tags
	std::map<uint32, bool> used;
	std::vector<uint32> tags;

	// use standard Forge order for the tags we know about
	for (std::vector<uint32>::const_iterator it = tag_save_list.begin(); it != tag_save_list.end(); ++it)
	{
		if (chunks_.count(*it) && chunks_.find(*it)->second.size())
		{
			tags.push_back(*it);
			used[*it] = true;
		}
	}
	
	// catch any remaining tags
	for (chunk_map::const_iterator it = chunks_.begin(); it != chunks_.end(); ++it)
	{
		if (it->second.size() && !used.count(it->first))
		{
			tags.push_back(it->first);
			used[it->first] = true;
		}
	}

	int32 offset = 0;

	for (std::vector<uint32>::const_iterator it = tags.begin(); it != tags.end(); ++it)
	{
		const std::vector<uint8>& chunk = chunks_.find(*it)->second;

		EntryHeader header;
		header.tag = *it;
		header.length = chunk.size();
		if (it == tags.end() - 1)
			header.next_offset = 0;
		else
			header.next_offset = offset + kEntryHeaderSize + header.length;
		header.offset = 0;

		std::vector<uint8> header_buffer(kEntryHeaderSize);
		AOStreamBE header_stream(&header_buffer[0], header_buffer.size());
		header.Save(header_stream);
		s.write(reinterpret_cast<const char *>(&header_buffer[0]), header_buffer.size());
		s.write(reinterpret_cast<const char *>(&chunk[0]), chunk.size());
		offset += header.length + kEntryHeaderSize;
	}
}

void Wad::EntryHeader::Load(AIStream& s, int16 entry_header_length)
{
	s >> tag;
	s >> next_offset;
	s >> length;
	if (entry_header_length >= Wad::kEntryHeaderSize)
		s >> offset;
	
	if (entry_header_length > Wad::kEntryHeaderSize)
		s.ignore(Wad::kEntryHeaderSize - entry_header_length);

}

void Wad::EntryHeader::Save(AOStream& s)
{
	s << tag;
	s << next_offset;
	s << length;
	s << offset;
}

std::ostream& marathon::operator<<(std::ostream& s, const Wad& w)
{
	for (std::map<uint32, std::vector<uint8> >::const_iterator it = w.chunks_.begin(); it != w.chunks_.end(); ++it)
	{
		s << std::string(reinterpret_cast<const char*>(&it->first), 4) << std::endl;
	}
}

