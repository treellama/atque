/* Wadfile.h

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

#ifndef WADFILE_H
#define WADFILE_H

#include "ferro/MapInfoChunk.h"
#include "ferro/Wad.h"

#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/crc.hpp>

namespace marathon
{
	class crc_ostream;

        class Wadfile 
	{
            friend std::ostream& operator<<(std::ostream&, const Wadfile&);
	public:
		Wadfile() { stream_.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);  }

		virtual bool Open(const std::string& path);
		virtual void Close();

		// will load the whole wadfile into memory!
		virtual bool Save(const std::string& path);

		bool HasWad(int16 index) { return directory_.count(index); }
		const Wad& GetWad(int16 index);
		void SetWad(int16 index, const Wad& wad);

		std::vector<int16> GetWadIndexes();
		std::vector<int16> GetEntryPointIndexes(uint32 entry_point_flags = ~0);

		uint32 GetEntryPointFlags(int16 index);
		std::string GetLevelName(int16 index);
		void SetLevelName(int16 index, const std::string& name);

		// accessors
		void data_version(int16 version) { header_.data_version = version; }
		int16 data_version() { return header_.data_version; }

		std::string file_name() { return header_.file_name; }
		void file_name(const std::string& s);

		int16 version() { return header_.version; }
		uint32 checksum() { return header_.checksum; }
		uint32 parent_checksum() { return header_.parent_checksum; }

	protected:
		virtual bool Load(const std::string& path);
		std::ifstream stream_;
		virtual void Seek(std::streampos pos) { stream_.seekg(pos); }

	private:
		std::map<int16, Wad> wads_;

		struct Header
		{
			static const int kFilenameLength = 64;
			enum { kSize = 128 };
			enum { 
				PRE_ENTRY_POINT_WADFILE_VERSION = 0,
				WADFILE_HAS_DIRECTORY_ENTRY = 1,
				WADFILE_SUPPORTS_OVERLAYS = 2,
				WADFILE_HAS_INFINITY_STUFF = 4,
				CURRENT_WADFILE_VERSION = WADFILE_HAS_INFINITY_STUFF
			};

			enum {
				MARATHON_DATA_VERSION = 0,
				MARATHON_TWO_DATA_VERSION = 1,
				CURRENT_DATA_VERSION = MARATHON_TWO_DATA_VERSION
			};

			Header() : version(CURRENT_WADFILE_VERSION), data_version(CURRENT_DATA_VERSION), checksum(0), directory_offset(0), wad_count(0), application_specific_directory_data_size(DirectoryData::kSize), entry_header_size(0), directory_entry_base_size(DirectoryEntry::kSize), parent_checksum(0) { std::fill_n(file_name, kFilenameLength, '\0'); }

			int16 version;
			int16 data_version;
			char file_name[kFilenameLength];
			uint32 checksum;
			int32 directory_offset;
			int16 wad_count;
			int16 application_specific_directory_data_size;
			int16 entry_header_size;
			int16 directory_entry_base_size;
			uint32 parent_checksum;

			void Load(std::istream&);
			void Save(crc_ostream&);

			// int16 unused[20];
		} header_;

		void UpdateDirectory(int16 index);

		struct DirectoryEntry
		{
			enum { kSize = 10, kOldSize = 8 };
			
			int32 offset;
			int32 size;
			int16 index;

			void Load(std::istream&, int16 directory_entry_base_size, int16 index);
			void Save(crc_ostream&) const;
		};
		friend std::ostream& operator<<(std::ostream&, const DirectoryEntry&);

		std::map<int16, DirectoryEntry> directory_;

		struct DirectoryData
		{
			enum {
				kSize = 74,
			};

			int16 mission_flags;
			int16 environment_flags;
			int32 entry_point_flags;
			char level_name[MapInfo::kLevelNameLength];

			DirectoryData() : mission_flags(0), environment_flags(0), entry_point_flags(0) { std::fill_n(level_name, MapInfo::kLevelNameLength, '\0'); }

			void Load(std::istream&);
			void Save(crc_ostream&) const;
		};
		std::map<int16, DirectoryData> directory_data_;
	};

	class crc_ostream
	{
	public:
		crc_ostream(std::ostream& stream) : stream_(stream) { }

		std::streampos tellp() const { return stream_.tellp(); }
		crc_ostream& seekp(std::streampos pos) { stream_.seekp(pos); return *this; }
		crc_ostream& write(const char *s, std::streamsize n) { stream_.write(s, n); crc_.process_bytes(s, n); return *this; }
		crc_ostream& write(uint8* s, std::streamsize n) { stream_.write(reinterpret_cast<char*>(s), n); crc_.process_bytes(s, n); return *this; }

		std::ostream& stream() { return stream_; }
		uint32 checksum() const { return crc_.checksum(); }

	private:
		std::ostream& stream_;
		boost::crc_32_type crc_;
	};

std::ostream& operator<<(std::ostream& s, const Wadfile& w);
std::ostream& operator<<(std::ostream& s, const Wadfile::DirectoryEntry& entry);

};

#endif
