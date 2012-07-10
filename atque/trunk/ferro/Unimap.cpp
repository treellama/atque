/* Unimap.cpp

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
#include "ferro/MapInfoChunk.h"
#include "ferro/Unimap.h"
#include "ferro/macroman.h"

#include <boost/crc.hpp>

using namespace marathon;

const std::vector<uint8>& Unimap::GetResource(ResourceIdentifier id)
{
	if (!resources_[id].size())
	{
		const Wad& wad = GetWad(id.second);
		if (wad.HasChunk(id.first))
		{
			return wad.GetChunk(id.first);
		}
	}

	return resources_[id];
}

std::string Unimap::GetResourceName(int16 id)
{
	if (names_.count(id))
	{
		return names_[id];
	}
	else
	{
		return GetLevelName(id);
	}
}

void Unimap::SetResource(ResourceIdentifier id, const std::vector<uint8>& data)
{
	Wad wad;
	if (HasWad(id.second))
		wad = GetWad(id.second);
	wad.AddChunk(id.first, data);
	SetWad(id.second, wad);
	resources_.erase(id);
}

std::vector<Unimap::ResourceIdentifier> Unimap::GetResourceIdentifiers()
{
	std::vector<ResourceIdentifier> identifiers;
	for (std::map<ResourceIdentifier, std::vector<uint8> >::const_iterator it = resources_.begin(); it != resources_.end(); ++it)
	{
		identifiers.push_back(it->first);
	}

	// wad resources
	std::vector<int16> wad_indexes = GetWadIndexes();
	for (std::vector<int16>::const_iterator it = wad_indexes.begin(); it != wad_indexes.end(); ++it)
	{
		const Wad& wad = GetWad(*it);
		if (!wad.HasChunk(MapInfo::kTag))
		{
			// assume any chunks are resources
			std::vector<uint32> tags = wad.GetTags();
			for (std::vector<uint32>::const_iterator tag_iterator = tags.begin(); tag_iterator != tags.end(); ++tag_iterator)
			{
				if (!resources_.count(ResourceIdentifier(*it, *tag_iterator)))
				{
					identifiers.push_back(ResourceIdentifier(*tag_iterator, *it));
				}
			}
		}
	}

	return identifiers;
}

bool Unimap::LoadMacBinary()
{
	// detect if it's MacBinary
	std::vector<uint8> header(128);
	try {
		stream_.seekg(0);
		stream_.read(reinterpret_cast<char*>(&header[0]), header.size());
	} 
	catch (std::ios_base::failure)
	{
		return false;
	}

	if (header[0] || header[1] > 63 || header[74] || header[123] > 0x81)
		return false;

	boost::crc_optimal<16, 0x1021, 0, 0, false, false> crc;
	crc.process_bytes(&header[0], 124);
	if (crc.checksum() != ((header[124] << 8) | header[125]))
		return false;

	uint32 data_length = (header[83] << 24) | (header[84] << 16) | (header[85] << 8) | header[86];
	uint32 resource_length = (header[87] << 24) | (header[88] << 16) | (header[89] << 8) | header[90];

	data_fork_ = 128;
	data_length_ = data_length;
//	resource_fork_ = 128 + ((data_length + 0x7f) & ~0x7f);
	uint32 resource_offset = 128 + ((data_length + 0x7f) & ~0x7f);
//	resource_length_ = resource_length;

	stream_.seekg(resource_offset);
	LoadResourceFork(stream_, resource_length);
	return true;

}

bool Unimap::Load(const std::string& path)
{
	// detect if the file is MacBinary
	if (!LoadMacBinary())
	{
#if defined(__APPLE__) && defined(__MACH__)
		// try to open the raw resource fork
		try
		{
			std::string rsrc_path = path + "/..namedfork/rsrc";
			std::ifstream rsrc_fork(rsrc_path.c_str(), std::ios::binary);
			rsrc_fork.seekg(0, std::ios::end);
			std::streamsize length = rsrc_fork.tellg();
			rsrc_fork.seekg(0);
			if (length) LoadResourceFork(rsrc_fork, length);
		}
		catch (const std::ios_base::failure& e)
		{
			resources_.clear();
		}
#endif
		
		data_fork_ = 0;
		data_length_ = 0;
		if (stream_.seekg(0, std::ios::end))
		{
			data_length_ = stream_.tellg();
		}

		if (data_length_)
			return Wadfile::Load(path);
		else
		{
			return resources_.size();
		}
	}
	else
	{
		if (data_length_)
			return Wadfile::Load(path);
		else
			return true;
	}
}

struct ResourceMap
{
	bool Load(AIStreamBE& stream);

	struct TypeListEntry
	{
		uint32 type;
		int16 num_refs;
		int16 ref_list_offset;

		void Load(AIStreamBE& stream);
	};

	struct RefListEntry
	{
		int16 id;
		int16 name_list_offset;
		uint32 data_offset;

		void Load(AIStreamBE& stream);
	};

	std::map<Unimap::ResourceIdentifier, uint32> offsets;
	std::map<Unimap::ResourceIdentifier, uint32> name_offsets;
};

void ResourceMap::TypeListEntry::Load(AIStreamBE& stream)
{
	stream >> type;
	stream >> num_refs;
	++num_refs;
	stream >> ref_list_offset;
}

void ResourceMap::RefListEntry::Load(AIStreamBE& stream)
{
	stream >> id;
	stream >> name_list_offset;
	stream >> data_offset;
	data_offset &= 0x00ffffff;
	stream.ignore(4);
}

bool ResourceMap::Load(AIStreamBE& stream)
{
	offsets.clear();
	stream.ignore(24);

	int16 type_list_offset;
	int16 name_list_offset;
	stream >> type_list_offset;
	stream >> name_list_offset;

	int16 num_types;
	stream >> num_types;
	++num_types;

	std::vector<TypeListEntry> type_list(num_types);
	for (std::vector<TypeListEntry>::iterator it = type_list.begin(); it != type_list.end(); ++it)
	{
		it->Load(stream);
	}

	for (std::vector<TypeListEntry>::iterator it = type_list.begin(); it != type_list.end(); ++it)
	{
		for (int i = 0; i < it->num_refs; ++i)
		{
			RefListEntry entry;
			entry.Load(stream);
			offsets[Unimap::ResourceIdentifier(it->type, entry.id)] = entry.data_offset;
			if (entry.name_list_offset >= 0)
			{
				name_offsets[Unimap::ResourceIdentifier(it->type, entry.id)] = entry.name_list_offset + name_list_offset;
			}
		}
	}
	
}

void Unimap::LoadResourceFork(std::istream& stream, std::streamsize size)
{
	std::streampos start = stream.tellg();

	// read in 16 byte header
	std::vector<uint8> header(16);
	stream.read(reinterpret_cast<char*>(&header[0]), header.size());
	AIStreamBE header_stream(&header[0], header.size());

	uint32 data_offset;
	uint32 map_offset;
	uint32 data_length;
	uint32 map_length;

	header_stream >> data_offset;
	header_stream >> map_offset;
	header_stream >> data_length;
	header_stream >> map_length;

	data_offset += start;
	map_offset += start;

	stream.seekg(map_offset);
	std::vector<uint8> map(map_length);
	stream.read(reinterpret_cast<char*>(&map[0]), map.size());
	AIStreamBE map_stream(&map[0], map.size());

	ResourceMap resource_map;
	resource_map.Load(map_stream);

	std::map<int16, std::string> text_names;

	for (std::map<ResourceIdentifier, uint32>::const_iterator it = resource_map.offsets.begin(); it != resource_map.offsets.end(); ++it)
	{
		stream.seekg(data_offset + it->second);

		uint8 length_buffer[4];
		stream.read(reinterpret_cast<char*>(length_buffer), 4);
		AIStreamBE length_stream(length_buffer, 4);
		uint32 length;
		length_stream >> length;
		
		resources_[it->first].resize(length);
		stream.read(reinterpret_cast<char*>(&resources_[it->first][0]), length);

		if (resource_map.name_offsets.count(it->first))
		{
			stream.seekg(map_offset + resource_map.name_offsets[it->first]);
			unsigned char length = stream.get();
			char buf[255];
			stream.read(buf, length);
			if (it->first.first == FOUR_CHARS_TO_INT('T','E','X','T'))
			{
				text_names[it->first.second].assign(buf, length);
			}
			else
			{
				names_[it->first.second].assign(buf, length);
			}
		}
	}

	for (std::map<int16, std::string>::const_iterator it = text_names.begin(); it != text_names.end(); ++it)
	{
		names_[it->first] = it->second;
	}

}

void Unimap::Seek(std::streampos pos)
{
	stream_.seekg(data_fork_ + pos);
}
