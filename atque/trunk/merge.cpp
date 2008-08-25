/* merge.cpp

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
  Merges files to create a scenario wadfile
*/


#include "merge.h"
#include "ferro/cstypes.h"
#include "ferro/Wad.h"
#include "ferro/TerminalChunk.h"
#include "ferro/Unimap.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/assign/list_of.hpp>

namespace fs = boost::filesystem;
using namespace atque;

const std::vector<uint32> physics_chunks = boost::assign::list_of
	(FOUR_CHARS_TO_INT('M','N','p','x'))
	(FOUR_CHARS_TO_INT('F','X','p','x'))
	(FOUR_CHARS_TO_INT('P','R','p','x'))
	(FOUR_CHARS_TO_INT('P','X','p','x'))
	(FOUR_CHARS_TO_INT('W','P','p','x'))
	;

void MergePhysics(const fs::path& path, marathon::Wad& wad)
{
	marathon::Unimap wadfile;
	if (wadfile.Open(path.string()))
	{
		// check to make sure all physics are present
		marathon::Wad physics = wadfile.GetWad(0);
		for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
		{
			if (!wad.HasChunk(*it))
				return;
		}

		for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
		{
			wad.AddChunk(*it, physics.GetChunk(*it));
		}
	}
}

void MergeShapes(const fs::path& path, marathon::Wad& wad)
{
	const uint32 shapes_tag = FOUR_CHARS_TO_INT('S','h','P','a');
	std::ifstream shapes(path.string().c_str(), std::ios::in | std::ios::binary);
	shapes.seekg(0, std::ios::end);
	uint32 length = shapes.tellg();
	std::vector<uint8> shapes_buffer(length);

	shapes.seekg(0);
	shapes.read(reinterpret_cast<char*>(&shapes_buffer[0]), shapes_buffer.size());
	wad.AddChunk(shapes_tag, shapes_buffer);
}

void MergeTerminal(const fs::path& path, marathon::Wad& wad)
{
	marathon::TerminalChunk chunk;
	chunk.Compile(path.string());
	wad.AddChunk(marathon::TerminalChunk::kTag, chunk.Save());
}

marathon::Wad CreateWad(const std::string& level_name, const fs::path& path)
{
	marathon::Wad wad;
	std::map<std::string, fs::path> extension_map;

	fs::directory_iterator end;
	for (fs::directory_iterator dir(path); dir != end; ++dir)
	{
		extension_map[fs::extension(*dir)] = *dir;
	}

	if (extension_map.count(".sceA"))
	{
		marathon::Unimap wadfile;
		if (wadfile.Open(extension_map[".sceA"].string()) && wadfile.data_version() == 1)
		{
			wad = wadfile.GetWad(0);

			
			if (extension_map.count(".phyA"))
			{
				MergePhysics(extension_map[".sceA"], wad);
			}
			if (extension_map.count(".ShPa"))
			{
				MergeShapes(extension_map[".ShPa"], wad);
			}
			if (extension_map.count(".txt"))
			{
				MergeTerminal(extension_map[".txt"], wad);
			}
		}
	}
	
	return wad;
}

static std::vector<uint8> ReadFile(const std::string& path)
{
	std::ifstream infile;
	infile.open(path.c_str(), std::ios::binary);
	
	infile.seekg(0, std::ios::end);
	int length = infile.tellg();
	infile.seekg(0, std::ios::beg);

	std::vector<uint8> data(length);
	infile.read(reinterpret_cast<char *>(&data[0]), data.size());

	return data;
}

void MergeTEXTs(marathon::Unimap& wadfile, const fs::path& path)
{
	fs::directory_iterator end;
	for (fs::directory_iterator dir(path); dir != end; ++dir)
	{
		if (!fs::is_directory(dir->status()))
		{
			std::istringstream s(dir->leaf());
			int16 index;
			s >> index;
			if (!s.fail() && index >= 128)
			{
				wadfile.SetResource(FOUR_CHARS_TO_INT('t','e','x','t'), index, ReadFile(dir->string()));
			}
		}
	}
	
}

void MergeResources(marathon::Unimap& wadfile, const fs::path& path)
{
	fs::directory_iterator end;
	for (fs::directory_iterator dir(path); dir != end; ++dir)
	{
		if (fs::is_directory(dir->status()))
		{
			if (dir->leaf() == "TEXT")
			{
				MergeTEXTs(wadfile, *dir);
			}
		}
	}
}

void atque::merge(const std::string& src, const std::string& dest, std::vector<std::string>& log)
{
	if (!fs::exists(src))
	{
		throw merge_error(src + " does not exist");
	}
	else if (!fs::is_directory(src))
	{
		throw merge_error("source must be a directory");
	}

	marathon::Unimap wadfile;

	fs::directory_iterator end;
	for (fs::directory_iterator dir(src); dir != end; ++dir)
	{
		if (fs::is_directory(dir->status()))
		{
			if (dir->leaf() == "Resources")
			{
				MergeResources(wadfile, *dir);
			}
			else
			{
				std::istringstream s(dir->leaf());
				int16 index;
				s >> index;
				if (!s.fail())
				{
					char c;
					do {
						c = s.get();
					}
					while (!s.fail() && c == ' ');
					
					if (!s.fail())
					{
						std::string level_name;
						std::getline(s, level_name);
						wadfile.SetWad(index, CreateWad(level_name, *dir));
					} 
				}
			}
		}
	}

	wadfile.file_name(fs::basename(dest));
	wadfile.Save(dest);
	
}
