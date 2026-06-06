/* ResourceManager.cpp

   Copyright (C) 2026 by Gregory Smith
   
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

#include "ResourceManager.h"

#include <fstream>
#include <iostream>
#include <set>

#include "MacBinaryII.h"

#include <boost/crc.hpp>
#include <boost/endian.hpp>

#include "ferro/cstypes.h"
#include "ferro/macroman.h"
#include "ferro/Wadfile.h"

using namespace marathon;
using namespace boost::endian;

struct ResourceHeader
{
	big_uint32_t data_offset;
	big_uint32_t map_offset;
	big_uint32_t data_length;
	big_uint32_t map_length;
};

struct ResourceMap
{
	ResourceHeader resource_header;
	big_int32_t next_resource_map;
	big_int16_t file_reference_number;
	big_int16_t attributes;
	
	big_int16_t type_list_offset;
	big_int16_t name_list_offset;
	big_int16_t num_types;
};
	
struct TypeListEntry
{
	big_uint32_t type;
	big_int16_t num_refs;
	big_int16_t ref_list_offset;
};

struct RefListEntry
{
	big_int16_t id;
	big_int16_t name_list_offset;
	big_uint32_t data_offset;
	big_uint32_t reserved;
};

bool ResourceManager::Load(const std::filesystem::path& path,
						   read_data_cb read_data_fork)
{
	try
	{
		std::ifstream stream{path, std::ios::in | std::ios::binary};
		stream.exceptions(std::ifstream::eofbit |
						  std::ifstream::failbit |
						  std::ifstream::badbit);

		MacBinaryII header;
		stream.read(reinterpret_cast<char*>(&header), sizeof(header));

		if (header.IsValid())
		{
			stream.seekg(header.GetResourceForkOffset());

			LoadResourceFork(stream, header.resource_fork_length);

			stream.seekg(header.GetDataOffset());
			read_data_fork(stream, header.data_fork_length);
		}
		else
		{
			// TODO: check for Mac resource fork with getxattr

			stream.seekg(0, std::ios_base::end);
			auto length = stream.tellg();
			stream.seekg(0);
			read_data_fork(stream, length);
		}
	}
	catch (const std::ios_base::failure& e)
	{
		return false;
	}

	return true;
}

void ResourceManager::Save(const std::filesystem::path& path,
						   write_data_cb write_data_fork)
{
	MacBinaryII header{};

	header.macbinary_version = 129;
	header.min_macbinary_version = 129;
	// TODO: creation date? file creator? modification date?
	auto filename = utf8_to_mac_roman(path.stem().u8string());
	header.filename_length = std::max(static_cast<size_t>(header.max_filename_length),
									  filename.size());
	std::copy_n(filename.begin(), header.filename_length, header.filename);

	auto extension = utf8_to_mac_roman(path.extension().u8string());
	if (extension.size() == 4)
	{
		header.file_type = FOUR_CHARS_TO_INT(extension[0],
											 extension[1],
											 extension[2],
											 extension[3]);
	}
	else
	{
		// TODO?
	}
	
	std::ofstream stream{path, std::ios::out | std::ios::binary | std::ios::trunc};
	stream.seekp(sizeof(MacBinaryII));

	write_data_fork(stream);
	header.data_fork_length = stream.tellp() - std::streamoff{sizeof(MacBinaryII)};
	auto padding = ((header.data_fork_length + 0x7f) & ~0x7f) - header.data_fork_length;
	while (padding--)
	{
		stream.put('\0');
	}

	SaveResourceFork(stream);
	header.resource_fork_length = stream.tellp() - std::streamoff{header.GetResourceForkOffset()};
	padding = ((header.resource_fork_length + 0x7f) & ~0x7f) - header.resource_fork_length;
	while (padding--)
	{
		stream.put('\0');
	}

	header.crc = header.CalculateCRC();

	stream.seekp(0);
	stream.write(reinterpret_cast<char*>(&header), sizeof(MacBinaryII));
}

bool ResourceManager::CanSaveToWadfile(Wadfile& wadfile)
{
	for (const auto& [key, data] : resource_map_)
	{
		const auto& [res_type, res_id] = key;
		if (wadfile.HasWad(res_id) &&
			wadfile.GetWad(res_id).HasChunk(MapInfo::kTag))
		{
			return false;
		}
	}

	return true;
}

bool ResourceManager::CanSaveToResourceFork()
{
	int size = 0;
	for (const auto& [key, value] : resource_map_)
	{
		size += value.size();
		if (size >= max_resource_fork_data_size)
		{
			return false;
		}
	}

	return true;
}

void ResourceManager::LoadFromWadfile(Wadfile& wadfile)
{
	// assume any chunks without MapInfo tags are resources
	for (const auto& index : wadfile.GetWadIndexes())
	{
		auto wad = wadfile.GetWad(index);
		auto name = wadfile.GetLevelName(index);
		if (!wad.HasChunk(MapInfo::kTag))
		{
			for (const auto& tag : wad.GetTags())
			{
				auto data = wad.GetChunk(tag);
				auto id = std::make_pair(tag, index);
				resource_map_.insert(std::make_pair(id, std::move(data)));

				if (!name.empty())
				{
					name_map_.insert(std::make_pair(id, name));
				}
			}
		}
	}
}

void ResourceManager::SaveToWadfile(Wadfile& wadfile)
{
	for (const auto& [key, data] : resource_map_)
	{
		auto [res_type, res_id] = key;

		// Win95 resource conversions
		if (res_type == FOUR_CHARS_TO_INT('T','E','X','T'))
		{
			res_type = FOUR_CHARS_TO_INT('t','e','x','t');
		}
		
		Wad wad;
		if (wadfile.HasWad(res_id))
		{
			wad = wadfile.GetWad(res_id);
		}

		if (!wad.HasChunk(MapInfo::kTag))
		{
			wad.AddChunk(res_type, data);
			wadfile.SetWad(res_id, wad);
		}
	}
}

void ResourceManager::LoadResourceFork(std::istream& stream, std::streamsize)
{
	std::streampos fork_start = stream.tellg();

	ResourceHeader header;
	stream.read(reinterpret_cast<char*>(&header), sizeof(ResourceHeader));
	std::streampos map_start = fork_start + static_cast<std::streamoff>(header.map_offset);
	stream.seekg(map_start);

	ResourceMap map;
	stream.read(reinterpret_cast<char*>(&map), sizeof(ResourceMap));
	stream.seekg(map_start + static_cast<std::streamoff>(map.type_list_offset + 2));

	std::vector<TypeListEntry> type_list;
	for (auto i = 0; i <= map.num_types; ++i)
	{
		TypeListEntry entry;
		stream.read(reinterpret_cast<char*>(&entry), sizeof(TypeListEntry));
		type_list.push_back(entry);

	}

	std::vector<RefListEntry> ref_list;
	for (const auto& tl_entry : type_list)
	{
		for (auto i = 0; i <= tl_entry.num_refs; ++i)
		{
			RefListEntry rl_entry;
			stream.read(reinterpret_cast<char*>(&rl_entry), sizeof(RefListEntry));
			ref_list.push_back(rl_entry);
		}
	}

	auto it = ref_list.begin();
	for (const auto& tl_entry : type_list)
	{
		for (auto i = 0; i <= tl_entry.num_refs; ++i)
		{
			auto& rl_entry = *it++;

			stream.seekg(fork_start + std::streamoff{header.data_offset} +
						 std::streamoff{rl_entry.data_offset & 0x00FFFFFF});
			big_uint32_t data_length;
			stream.read(reinterpret_cast<char*>(&data_length), sizeof(big_uint32_t));

			std::vector<uint8_t> data(data_length);
			stream.read(reinterpret_cast<char*>(data.data()), data.size());

			auto id = std::make_pair(tl_entry.type, rl_entry.id);
			resource_map_.insert(std::make_pair(id, std::move(data)));
			
			if (rl_entry.name_list_offset != -1)
			{
				stream.seekg(map_start + std::streamoff{map.name_list_offset} + std::streamoff{rl_entry.name_list_offset});
				unsigned char length = stream.get();
				char buf[256] {0};
				stream.read(buf, length);

				name_map_.insert(std::make_pair(id, std::string{buf, length}));
			}
		}
	}
}
 
void ResourceManager::SaveResourceFork(std::ostream& stream)
{
	auto fork_start = stream.tellp();

	// re-order the resource map for output
	std::map<uint32_t, std::set<int16_t>> res_set;
	for (auto& [key, value] : resource_map_)
	{
		auto& [res_type, res_id] = key;
		res_set[res_type].insert(res_id);
	}

	int16_t ref_list_offset = res_set.size() * sizeof(TypeListEntry) + 2;
	int16_t name_list_offset = 0;
	int32_t data_offset = 0;
	
	std::vector<TypeListEntry> type_list;
	std::vector<RefListEntry> ref_list;
	for (auto& [res_type, set] : res_set)
	{
		TypeListEntry type_list_entry{};
		type_list_entry.type = res_type;
		type_list_entry.num_refs = static_cast<int16_t>(set.size() - 1);
		type_list_entry.ref_list_offset = ref_list_offset;

		type_list.push_back(type_list_entry);
		
		for (auto& res_id : set)
		{
			RefListEntry ref_list_entry{};
			ref_list_entry.id = res_id;
			auto name_it = name_map_.find(std::make_pair(res_type, res_id));
			if (name_it != name_map_.end())
			{
				ref_list_entry.name_list_offset = name_list_offset;
				name_list_offset += 1 + std::min(name_it->second.size(), 255UL);
			}
			else
			{
				ref_list_entry.name_list_offset = -1;
			}

			ref_list_entry.data_offset = data_offset;
			data_offset += sizeof(uint32_t);
			data_offset += resource_map_[std::make_pair(res_type, res_id)].size();

			ref_list.push_back(ref_list_entry);
			ref_list_offset += sizeof(RefListEntry);
		}
	}

	ResourceHeader header{};
	header.map_offset = sizeof(ResourceHeader);
	header.map_length = sizeof(ResourceMap) +
						type_list.size() * sizeof(TypeListEntry) +
						ref_list.size() * sizeof(RefListEntry) +
						name_list_offset;
	
	header.data_offset = header.map_offset + header.map_length;
	header.data_length = data_offset;

	stream.write(reinterpret_cast<char*>(&header), sizeof(ResourceHeader));

	auto map_start = stream.tellp();

	ResourceMap map{};
	map.resource_header = header;
	map.type_list_offset = sizeof(ResourceMap) - 2;
	map.name_list_offset = sizeof(ResourceMap) +
						   type_list.size() * sizeof(TypeListEntry) +
						   ref_list.size() * sizeof(RefListEntry);
	map.num_types = type_list.size() - 1;

	stream.write(reinterpret_cast<char*>(&map), sizeof(ResourceMap));

	for (const auto& type_list_entry : type_list)
	{
		stream.write(reinterpret_cast<const char*>(&type_list_entry),
					 sizeof(TypeListEntry));
	}

	for (const auto& ref_list_entry : ref_list)
	{
		stream.write(reinterpret_cast<const char*>(&ref_list_entry),
					 sizeof(RefListEntry));
	}

	assert(stream.tellp() - map_start == map.name_list_offset);

	for (auto& [res_type, set] : res_set)
	{
		for (auto& res_id : set)
		{
			auto name_it = name_map_.find(std::make_pair(res_type, res_id));
			if (name_it != name_map_.end())
			{
				auto& name = name_it->second;
				uint8_t length = std::min(name.size(), 255UL);
				stream.put(length);
				stream.write(name.data(), length);
			}
		}
	}

	assert(stream.tellp() - fork_start == header.data_offset);

	for (auto& [res_type, set] : res_set)
	{
		for (auto& res_id : set)
		{
			auto& data = resource_map_[std::make_pair(res_type, res_id)];
			big_uint32_t data_length = data.size();

			stream.write(reinterpret_cast<char*>(&data_length),
						 sizeof(data_length));
			stream.write(reinterpret_cast<char*>(data.data()), data.size());
		}
	}

	assert(stream.tellp() - fork_start == header.data_offset + header.data_length);
}
