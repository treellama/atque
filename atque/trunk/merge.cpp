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
#include "ferro/macroman.h"
#include "ferro/Wad.h"
#include "ferro/TerminalChunk.h"
#include "ferro/Unimap.h"

#include "filesystem.h"
#include "CLUTResource.h"
#include "PICTResource.h"
#include "SndResource.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <boost/assign/list_of.hpp>

using namespace atque;

const std::vector<uint32> physics_chunks = boost::assign::list_of
	(FOUR_CHARS_TO_INT('M','N','p','x'))
	(FOUR_CHARS_TO_INT('F','X','p','x'))
	(FOUR_CHARS_TO_INT('P','R','p','x'))
	(FOUR_CHARS_TO_INT('P','X','p','x'))
	(FOUR_CHARS_TO_INT('W','P','p','x'))
	;

void MergePhysics(const fs::path& path, marathon::Wad& wad, std::ostream& log)
{
	marathon::Unimap wadfile;
	if (wadfile.Open(path.string()))
	{
		// check to make sure all physics are present
		marathon::Wad physics = wadfile.GetWad(0);
		for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
		{
			if (!physics.HasChunk(*it))
			{
				fs::path file = path.parent() / path.filename();
				log << file.string() << " is not a valid physics model; skipping" << std::endl;
				return;
			}
		}

		for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
		{
			wad.AddChunk(*it, physics.GetChunk(*it));
		}
	}
}

void MergeShapes(const fs::path& path, marathon::Wad& wad, std::ostream& log)
{
	const uint32 shapes_tag = FOUR_CHARS_TO_INT('S','h','P','a');
	std::ifstream shapes(path.string().c_str(), std::ios::in | std::ios::binary);
	shapes.seekg(0, std::ios::end);
	uint32 length = shapes.tellg();
	if (length <= 384 * 1024)
	{
		std::vector<uint8> shapes_buffer(length);
		
		shapes.seekg(0);
		shapes.read(reinterpret_cast<char*>(&shapes_buffer[0]), shapes_buffer.size());
		wad.AddChunk(shapes_tag, shapes_buffer);
	}
	else
	{
		fs::path file = path.parent() / path.filename();
		log << file.string() << " is larger than 384K; skipping" << std::endl;
	}
}

void MergeTerminal(const fs::path& path, marathon::Wad& wad, std::ostream& log)
{
	try 
	{
		marathon::TerminalChunk chunk;
		chunk.Compile(path.string());
		wad.AddChunk(marathon::TerminalChunk::kTag, chunk.Save());
	}
	catch (const marathon::TerminalChunk::ParseError& e)
	{
		fs::path file = path.parent() / path.filename();
		log << file.string() << ": " << e.what() << "; skipping" << std::endl;
	}
}

marathon::Wad CreateWad(const fs::path& path, std::ostream& log)
{
	marathon::Wad wad;
	std::map<std::string, fs::path> extension_map;

	std::vector<fs::path> dir = path.ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		extension_map[fs::extension(it->filename())] = *it;
	}

	if (extension_map.count(".sceA"))
	{
		marathon::Unimap wadfile;
		if (wadfile.Open(extension_map[".sceA"].string()) && wadfile.data_version() == 1)
		{
			wad = wadfile.GetWad(0);

			
			if (extension_map.count(".phyA"))
			{
				MergePhysics(extension_map[".phyA"], wad, log);
			}
			if (extension_map.count(".ShPa"))
			{
				MergeShapes(extension_map[".ShPa"], wad, log);
			}
			if (extension_map.count(".txt"))
			{
				MergeTerminal(extension_map[".txt"], wad, log);
			}
		}
	}
	
	return wad;
}

void MergeCLUTs(marathon::Unimap& wadfile, const fs::path& path)
{
	std::vector<fs::path> dir = path.ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (!it->is_directory())
		{
			std::istringstream s(it->filename());
			int16 index;
			s >> index;
			if (!s.fail() && index >= 128)
			{
				CLUTResource clut;
				if (clut.Import(it->string()))
				{
					wadfile.SetResource(FOUR_CHARS_TO_INT('c','l','u','t'), index, clut.Save());
				}
			}
		}
	}
}

void MergePICTs(marathon::Unimap& wadfile, const fs::path& path)
{
	std::vector<fs::path> dir = path.ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (!it->is_directory())
		{
			std::istringstream s(it->filename());
			int16 index;
			s >> index;
			if (!s.fail() && index >= 128)
			{
				PICTResource pict;
				if (pict.Import(it->string()))
				{
					wadfile.SetResource(FOUR_CHARS_TO_INT('P','I','C','T'), index, pict.Save());
				}
			}
		}
	}
}

void MergeSnds(marathon::Unimap& wadfile, const fs::path& path)
{
	std::vector<fs::path> dir = path.ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (!it->is_directory())
		{
			std::istringstream s(it->filename());
			int16 index;
			s >> index;
			if (!s.fail() && index >= 128)
			{
				SndResource snd;
				if (snd.Import(it->string()))
				{
					wadfile.SetResource(FOUR_CHARS_TO_INT('s','n','d',' '), index, snd.Save());
				}
			}
		}
	}
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
	std::vector<fs::path> dir = path.ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (!it->is_directory())
		{
			std::istringstream s(it->filename());
			int16 index;
			s >> index;
			if (!s.fail() && index >= 128)
			{
				wadfile.SetResource(FOUR_CHARS_TO_INT('t','e','x','t'), index, ReadFile(it->string()));
			}
		}
	}
	
}

void MergeResources(marathon::Unimap& wadfile, const fs::path& path)
{
	std::vector<fs::path> dir = path.ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (it->is_directory())
		{
			if (it->filename() == "TEXT")
			{
				MergeTEXTs(wadfile, *it);
			}
			else if (it->filename() == "CLUT")
			{
				MergeCLUTs(wadfile, *it);
			}
			else if (it->filename() == "PICT")
			{
				MergePICTs(wadfile, *it);
			}
			else if (it->filename() == "snd")
			{
				MergeSnds(wadfile, *it);
			}
		}
	}
}

static std::string get_line(std::istream& stream)
{
	std::string line;
	char c;
	stream.get(c);
	while (!stream.eof() && c != '\n' && c != '\r')
	{
		if (c == '\r')
		{
			if (!stream.eof() && stream.peek() == '\n')
			{
				stream.ignore(1);
			}
		}
		else if (c != '\n' && c != '\0')
		{
			line += c;
		}

		stream.get(c);

	};

	return line;
}


void atque::merge(const std::string& src, const std::string& dest, std::ostream& log)
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
	fs::path level_select_path(src);
	level_select_path = level_select_path / "Level Select Names.txt";
	
	std::map<int16, std::string> level_select_names;
	if (level_select_path.exists())
	{
		std::ifstream s(level_select_path.string().c_str());
		while (!s.eof() && !s.fail())
		{
			int16 index;
			s >> index;
			if (!s.fail())
			{
				level_select_names[index] = get_line(s);
			}
		}
	}

	std::vector<fs::path> dir = fs::path(src).ls();
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (it->is_directory())
		{
			if (it->filename() == "Resources")
			{
				MergeResources(wadfile, *it);
			}
			else
			{
				std::istringstream s(it->filename());
				int16 index;
				s >> index;
				if (!s.fail())
				{
					while (!s.fail() && s.peek() == ' ')
						s.get();
					
					if (!s.fail())
					{
						std::string level_name;
						std::getline(s, level_name);
						wadfile.SetWad(index, CreateWad(*it, log));
						if (level_select_names.count(index))
							wadfile.SetLevelName(index, utf8_to_mac_roman(level_select_names[index]));
					} 
				}
			}
		}
	}

	wadfile.file_name(fs::basename(fs::path(dest).filename()));
	wadfile.Save(dest);
	
}
