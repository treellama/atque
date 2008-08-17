/* split.cpp

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

/*
  Splits wadfile
*/

#include "ferro/macroman.h"
#include "ferro/MapInfoChunk.h"
#include "ferro/TerminalChunk.h"
#include "ferro/Wadfile.h"
#include "ferro/Unimap.h"

#include "split.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
using namespace atque;

const std::vector<uint32> physics_chunks = boost::assign::list_of
	(FOUR_CHARS_TO_INT('M','N','p','x'))
	(FOUR_CHARS_TO_INT('F','X','p','x'))
	(FOUR_CHARS_TO_INT('P','R','p','x'))
	(FOUR_CHARS_TO_INT('P','X','p','x'))
	(FOUR_CHARS_TO_INT('W','P','p','x'))
	;

// removes physics chunks from wad, and saves file
void SavePhysics(marathon::Wad& wad, const std::string& name, const std::string& path)
{
	marathon::Wad physicsWad;
	bool has_physics = false;
	
	for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
	{
		if (wad.HasChunk(*it))
		{
			physicsWad.AddChunk(*it, wad.GetChunk(*it));
			wad.RemoveChunk(*it);
			has_physics = true;
		}
	}

	if (has_physics)
	{
		// export it!
		marathon::Wadfile wadfile;
		wadfile.SetWad(0, physicsWad);
		wadfile.data_version(0);
		wadfile.file_name(name);
		wadfile.Save(path);
	}
}

void SaveLevel(marathon::Wad& wad, const std::string& name, const std::string& path)
{
	// export everything remaining in the wad file!
	marathon::Wadfile wadfile;
	wadfile.SetWad(0, wad);
	wadfile.file_name(name);
	wadfile.Save(path);
}

// removes shapes chunk from Wad, and saves file
void SaveShapes(marathon::Wad& wad, const std::string& path)
{
	const uint32 shapes_tag = FOUR_CHARS_TO_INT('S','h','P','a');
	if (wad.HasChunk(shapes_tag))
	{
		const std::vector<uint8>& data = wad.GetChunk(shapes_tag);
		if (data.size())
		{
			std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
			outfile.write(reinterpret_cast<const char*>(&data[0]), data.size());
		}
		wad.RemoveChunk(shapes_tag);
	}
}

void SavePICT(marathon::Unimap& wad, marathon::Unimap::ResourceIdentifier id, const std::string& path)
{
	const std::vector<uint8>& data = wad.GetResource(id);
	if (data.size())
	{
		std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
		for (int i = 0; i < 512; ++i)
		{
			outfile.put('\0');
		}

		outfile.write(reinterpret_cast<const char*>(&data[0]), data.size());
	}
}

void SaveTEXT(marathon::Unimap& wad, marathon::Unimap::ResourceIdentifier id, const std::string& path)
{
	const std::vector<uint8>& data = wad.GetResource(id);
	if (data.size())
	{
		std::ofstream outfile(path.c_str(), std::ios::out | std::ios::trunc);
		outfile.write(reinterpret_cast<const char*>(&data[0]), data.size());
	}
}

void SaveTerminal(marathon::Wad& wad, const std::string& path)
{
	if (wad.HasChunk(marathon::TerminalChunk::kTag))
	{
		marathon::TerminalChunk chunk;
		chunk.Load(wad.GetChunk(marathon::TerminalChunk::kTag));
		chunk.Decompile(path);
		wad.RemoveChunk(marathon::TerminalChunk::kTag);
	}
}

void atque::split(const std::string& src, const std::string& dest)
{
	if (!fs::exists(dest))
	{
		fs::create_directory(dest);
	}
	else if (!fs::is_directory(dest))
	{
		std::cerr << "Destination must be a directory" << std::endl;
		return;
	}

	marathon::Unimap wadfile;
	wadfile.Open(src.c_str());

	std::vector<int16> indexes = wadfile.GetWadIndexes();
	for (std::vector<int16>::iterator it = indexes.begin(); it != indexes.end(); ++it)
	{
		marathon::Wad wad = wadfile.GetWad(*it);
		if (wad.HasChunk(marathon::MapInfo::kTag))
		{
			marathon::MapInfo minf(wad.GetChunk(marathon::MapInfo::kTag));

			std::string level = wadfile.GetLevelName(*it);
			std::string actual_level = minf.level_name();
			std::ostringstream level_number;
			level_number << std::setw(2) << std::setfill('0') << *it;

			fs::path destfolder = fs::path(dest) / (level_number.str() + " " + mac_roman_to_utf8(level));
			fs::create_directory(destfolder);
			
			fs::path physics_path = destfolder / (mac_roman_to_utf8(actual_level) + ".phyA");
			SavePhysics(wad, actual_level, physics_path.string());
			
			fs::path shapes_path = destfolder / (mac_roman_to_utf8(actual_level) + ".ShPa");
			SaveShapes(wad, shapes_path.string());
			
			fs::path terminal_path = destfolder / (mac_roman_to_utf8(actual_level) + ".term.txt");
			SaveTerminal(wad, terminal_path.string());
			
			fs::path level_path = destfolder / (mac_roman_to_utf8(actual_level) + ".sceA");
			SaveLevel(wad, actual_level, level_path.string());

		}
	}

#if 0 // no resources for now
	fs::path resource_path = fs::path(dest) / "Resources";
	fs::create_directory(resource_path);

	std::vector<marathon::Unimap::ResourceIdentifier> resources = wadfile.GetResourceIdentifiers();
	for (std::vector<marathon::Unimap::ResourceIdentifier>::const_iterator it = resources.begin(); it != resources.end(); ++it)
	{
		if (it->first == FOUR_CHARS_TO_INT('P','I','C','T'))
		{
			std::ostringstream resource_id;
			resource_id << it->second;
			
			fs::path pict_path = resource_path / (resource_id.str() + ".pct"); 
			SavePICT(wadfile, *it, pict_path.string());
		}
		else if (it->first == FOUR_CHARS_TO_INT('T','E','X','T'))
		{
			std::ostringstream resource_id;
			resource_id << it->second;

			fs::path text_path = resource_path / (resource_id.str() + ".txt");
			SaveTEXT(wadfile, *it, text_path.string());
		}
	}
#endif
}
