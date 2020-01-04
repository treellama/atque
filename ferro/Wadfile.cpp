/* Wadfile.cpp

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
#include "ferro/Wadfile.h"

#include <fstream>
#include <string.h>

using namespace marathon;

bool Wadfile::Open(const std::string& filename)
{
	if (stream_.is_open()) stream_.close();

	try {
		stream_.open(filename.c_str(), std::ios::in | std::ios::binary);
	} 
	catch (std::ios_base::failure e)
	{
		return false;
	}

	return Load(filename);
}

bool Wadfile::Load(const std::string&)
{
	directory_.clear();
	try {
		// load the header
		Seek(0);
		header_.Load(stream_);

		// load the directory
		Seek(header_.directory_offset);
		for (int i = 0; i < header_.wad_count; ++i)
		{
			DirectoryEntry entry;
			entry.Load(stream_, header_.directory_entry_base_size, i);
			directory_[entry.index] = entry;
			if (header_.application_specific_directory_data_size == DirectoryData::kSize)
			{
				directory_data_[entry.index].Load(stream_);
			}
			else
			{
				stream_.ignore(header_.application_specific_directory_data_size);
			}
		}
	} 
	catch (std::ios_base::failure e)
	{
		return false;
	}
	
	return true;	
}

void Wadfile::Close()
{
	directory_.clear();
	if (stream_.is_open()) stream_.close();
}

bool Wadfile::Save(const std::string& path)
{
	std::ofstream stream;
	stream.exceptions(std::ofstream::eofbit | std::ofstream::failbit | std::ofstream::badbit);
	
	try 
	{
		stream.open(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		crc_ostream crc_stream(stream);

		std::streampos start = crc_stream.tellp();

		// write the header
		Header newHeader;
		newHeader.entry_header_size = Wad::kEntryHeaderSize;
		newHeader.data_version = header_.data_version;
		strncpy(newHeader.file_name, header_.file_name, Header::kFilenameLength);
		newHeader.file_name[Header::kFilenameLength - 1] = '\0';
	
		newHeader.wad_count = 0;
		newHeader.directory_offset = Header::kSize;

		for (std::map<int16, DirectoryEntry>::iterator it = directory_.begin(); it != directory_.end(); ++it)
		{
			if (it->second.size)
			{
				++newHeader.wad_count;
				it->second.offset = newHeader.directory_offset;
				newHeader.directory_offset += it->second.size;
			}
		}

		if (newHeader.wad_count == 0) 
			return false;
		else if (newHeader.wad_count == 1)
		{
			newHeader.version = Header::WADFILE_SUPPORTS_OVERLAYS;
			newHeader.application_specific_directory_data_size = 0;
		}
	
		newHeader.Save(crc_stream);
	
		// write wads
		for (std::map<int16, DirectoryEntry>::const_iterator it = directory_.begin(); it != directory_.end(); ++it)
		{
			const Wad& wad = GetWad(it->first);
			wad.Save(crc_stream);
		}

		// write directory
		for (std::map<int16, DirectoryEntry>::const_iterator it = directory_.begin(); it != directory_.end(); ++it)
		{
			it->second.Save(crc_stream);
			if (newHeader.application_specific_directory_data_size > 0)
			{
				if (!directory_data_.count(it->first))
					UpdateDirectory(it->first);

				directory_data_[it->first].Save(crc_stream);
			}
		}

		std::streampos end = crc_stream.tellp();

		// write in the CRC
		newHeader.checksum = crc_stream.checksum();
		crc_stream.seekp(start);
		{
			newHeader.Save(crc_stream);
		}
		crc_stream.seekp(end);
	}
	catch (std::ios_base::failure e)
	{
		return false;
	}

	return true;
}

const Wad& Wadfile::GetWad(int16 index)
{
	if (!wads_.count(index))
	{
		std::vector<char> buffer(directory_[index].size);
		Seek(directory_[index].offset);
		wads_[index].Load(stream_, header_.entry_header_size);
	}
	return wads_[index];
}

void Wadfile::SetWad(int16 index, const Wad& wad)
{
	wads_[index] = wad;
	directory_[index].index = index;
	directory_[index].size = wad.GetSize();
	UpdateDirectory(index);
}

uint32 Wadfile::GetEntryPointFlags(int16 index)
{
	if (directory_.count(index))
	{
		if (!directory_data_.count(index))
			UpdateDirectory(index);

		return directory_data_[index].entry_point_flags;
	}
	else
	{
		return 0;
	}
}

std::string Wadfile::GetLevelName(int16 index)
{
	if (directory_.count(index))
	{
		if (!directory_data_.count(index))
			UpdateDirectory(index);

		return directory_data_[index].level_name;
	}
	else
	{
		return std::string();
	}
}

void Wadfile::SetLevelName(int16 index, const std::string& name)
{
	if (!directory_data_.count(index))
		UpdateDirectory(index);

	strncpy(directory_data_[index].level_name, name.c_str(), MapInfo::kLevelNameLength);
	directory_data_[index].level_name[MapInfo::kLevelNameLength - 1] = '\0';
}

void Wadfile::file_name(const std::string& s)
{
	strncpy(header_.file_name, s.c_str(), Header::kFilenameLength);
	header_.file_name[Header::kFilenameLength - 1] = '\0';
	
}

std::vector<int16> Wadfile::GetEntryPointIndexes(uint32 entry_point_flags)
{
	std::vector<int16> v;
	for (std::map<int16, DirectoryEntry>::const_iterator it = directory_.begin(); it != directory_.end(); ++it)
	{
		if (GetEntryPointFlags(it->first) & entry_point_flags)
			v.push_back(it->first);
	}

	return v;
}

std::vector<int16> Wadfile::GetWadIndexes()
{
	std::vector<int16> v;
	for (std::map<int16, DirectoryEntry>::const_iterator it = directory_.begin(); it != directory_.end(); ++it)
	{
		v.push_back(it->first);
	}

	return v;
}

void Wadfile::Header::Load(std::istream& stream)
{
	std::vector<uint8> data(kSize);
	stream.read(reinterpret_cast<char*>(&data[0]), data.size());
	AIStreamBE s(&data[0], data.size());
	
	s >> version;
	s >> data_version;
	s.read(file_name, kFilenameLength);
	file_name[kFilenameLength - 1] = '\0';
	s >> checksum;
	s >> directory_offset;
	s >> wad_count;
	s >> application_specific_directory_data_size;

	s >> entry_header_size;
	if (version <= WADFILE_HAS_DIRECTORY_ENTRY)
		entry_header_size = Wad::kEntryHeaderOldSize;

	s >> directory_entry_base_size;
	if (version <= WADFILE_HAS_DIRECTORY_ENTRY)
		directory_entry_base_size = DirectoryEntry::kOldSize;

	s >> parent_checksum;
	s.ignore(2 * 20);
}

void Wadfile::Header::Save(crc_ostream& stream)
{
	std::vector<uint8> data(kSize);
	AOStreamBE s(&data[0], data.size());

	s << version;
	s << data_version;
	s.write(file_name, kFilenameLength);
	s << checksum;
	s << directory_offset;
	s << wad_count;
	s << application_specific_directory_data_size;
	s << entry_header_size;
	s << directory_entry_base_size;
	s << parent_checksum;
	std::vector<uint8> padding(2 * 20);
	s.write(&padding[0], padding.size());

	stream.write(&data[0], data.size());
}

void Wadfile::UpdateDirectory(int16 index)
{
	bool wasLoaded = wads_.count(index);

	const Wad& wad = GetWad(index);
	DirectoryData entry;
	if (wad.HasChunk(MapInfo::kTag))
	{
		MapInfo info(wad.GetChunk(MapInfo::kTag));
		
		entry.mission_flags = info.mission_flags();
		entry.environment_flags = info.environment_flags();
		entry.entry_point_flags = info.entry_point_flags();
		strncpy(entry.level_name, info.level_name().c_str(), MapInfo::kLevelNameLength);
		entry.level_name[MapInfo::kLevelNameLength - 1] = '\0';
	}
	directory_data_[index] = entry;

	if (!wasLoaded) wads_.erase(index);
}

void Wadfile::DirectoryEntry::Load(std::istream& stream, int16 directory_entry_base_size, int16 new_index)
{
	std::vector<uint8> data(directory_entry_base_size);
	stream.read(reinterpret_cast<char*>(&data[0]), data.size());
	AIStreamBE s(&data[0], data.size());
	
	s >> offset;
	s >> size;
	if (directory_entry_base_size >= kSize)
		s >> index;
	else
		index = new_index;

	if (directory_entry_base_size > kSize)
		s.ignore(10 - directory_entry_base_size);
}

void Wadfile::DirectoryEntry::Save(crc_ostream& stream) const
{
	std::vector<uint8> data(kSize);
	AOStreamBE s(&data[0], data.size());

	s << offset;
	s << size;
	s << index;

	stream.write(&data[0], data.size());
}

void Wadfile::DirectoryData::Load(std::istream& stream)
{
	std::vector<uint8> data(kSize);
	try {
		stream.read(reinterpret_cast<char*>(&data[0]), data.size());
	} 
	catch (std::ios_base::failure)
	{
		// broken JUICE merge would write some files 2 bytes short
		if (stream.gcount() == data.size() - 2)
		{
			stream.clear();
		}
		else
		{
			throw;
		}
	}

	AIStreamBE s(&data[0], data.size());

	s >> mission_flags;
	s >> environment_flags;
	s >> entry_point_flags;
	s.read(level_name, MapInfo::kLevelNameLength);
	level_name[MapInfo::kLevelNameLength - 1] = '\0';
}

void Wadfile::DirectoryData::Save(crc_ostream& stream) const
{
	std::vector<uint8> data(kSize);
	AOStreamBE s(&data[0], data.size());

	s << mission_flags;
	s << environment_flags;
	s << entry_point_flags;
	s.write(level_name, MapInfo::kLevelNameLength);

	stream.write(&data[0], data.size());
}

std::ostream& marathon::operator<<(std::ostream& s, const Wadfile& w)
{
	s << "Version: " << w.header_.version << std::endl;
	s << "Data version: " << w.header_.data_version << std::endl;
	s << "File name: " << w.header_.file_name << std::endl;
	s << "Checksum: " << w.header_.checksum << std::endl;
	s << "Directory offset: " << w.header_.directory_offset << std::endl;
	s << "Application: " << w.header_.application_specific_directory_data_size << std::endl;
	s << "Entry header size: " << w.header_.entry_header_size << std::endl;
	s << "Wad count: " << w.header_.wad_count << std::endl;
	return s;
}

std::ostream& marathon::operator<<(std::ostream& s, const Wadfile::DirectoryEntry& entry)
{
	s << "Entry index: " << entry.index <<"\tsize: " << entry.size << "\toffset: " << entry.offset << std::endl;
}
