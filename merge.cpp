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
#include "ferro/ScriptChunk.h"
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
#include <boost/algorithm/string/case_conv.hpp>

using namespace atque;
namespace algo = boost::algorithm;

const std::vector<uint32> physics_chunks = boost::assign::list_of
	(FOUR_CHARS_TO_INT('M','N','p','x'))
	(FOUR_CHARS_TO_INT('F','X','p','x'))
	(FOUR_CHARS_TO_INT('P','R','p','x'))
	(FOUR_CHARS_TO_INT('P','X','p','x'))
	(FOUR_CHARS_TO_INT('W','P','p','x'))
	;

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

void MergeScripts(const std::vector<fs::path> paths, marathon::Wad& wad, uint32 tag)
{
	marathon::ScriptChunk chunk;
	for (std::vector<fs::path>::const_iterator it = paths.begin(); it != paths.end(); ++it)
	{
		marathon::ScriptChunk::Script script;
		script.name = fs::basename(it->filename());
		script.data = ReadFile(it->string());
		chunk.AddScript(script);
	}

	wad.AddChunk(tag, chunk.Save());
}

struct SortScriptPaths
{
	bool operator() (const fs::path& a, const fs::path& b) {
		std::string as = a.string();
		std::string bs = b.string();
		algo::to_lower(as);
		algo::to_lower(bs);
		return as < bs;
	}
};

marathon::Wad CreateWad(const fs::path& path, std::ostream& log)
{
	marathon::Wad wad;

	std::vector<fs::path> maps;
	std::vector<fs::path> physics;
	std::vector<fs::path> shapes;
	std::vector<fs::path> terminals;
	std::vector<fs::path> luas;
	std::vector<fs::path> mmls;
	
	std::vector<fs::path> dir = path.ls();	
	for (std::vector<fs::path>::iterator it = dir.begin(); it != dir.end(); ++it)
	{
		if (it->filename()[0] == '.')
		{
			continue;
		}

		std::string extension = fs::extension(it->filename());
		if (extension == ".sceA")
			maps.push_back(*it);
		else if (extension == ".phyA")
			physics.push_back(*it);
		else if (extension == ".ShPa")
			shapes.push_back(*it);
		else if (extension == ".txt")
			terminals.push_back(*it);
		else if (extension == ".lua")
			luas.push_back(*it);
		else if (extension == ".mml")
			mmls.push_back(*it);
	}

	if (maps.size())
	{
		if (maps.size() > 1)
			log << path.string() << ": multiple maps found; using " << maps[0].string() << std::endl;

		marathon::Unimap wadfile;
		if (wadfile.Open(maps[0].string()) && wadfile.data_version() == 1)
		{
			wad = wadfile.GetWad(0);

			
			if (physics.size())
			{
				if (physics.size() > 1)
					log << path.string() << ": multiple physics models found; using " << physics[0].string() << std::endl;

				MergePhysics(physics[0], wad, log);
			}
			if (shapes.size())
			{
				if (shapes.size() > 1)
					log << path.string() << ": multiple shapes patches found; using " << shapes[0].string() << std::endl;
				MergeShapes(shapes[0], wad, log);
			}
			if (terminals.size())
			{
				if (terminals.size() > 1)
					log << path.string() << ": multiple terminal texts files found; using " << terminals[0].string() << std::endl;
				MergeTerminal(terminals[0], wad, log);
			}
			if (luas.size())
			{
				std::sort(luas.begin(), luas.end(), SortScriptPaths());
				MergeScripts(luas, wad, marathon::ScriptChunk::kLuaTag);
			}
			if (mmls.size())
			{
				std::sort(mmls.begin(), mmls.end(), SortScriptPaths());
				MergeScripts(mmls, wad, marathon::ScriptChunk::kMMLTag);
			}
		}
	}
	else
	{
		throw merge_error(path.string() + " does not contain a map");
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
				s.ignore();
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
					} 
				}
			}
		}
	}

	for (std::map<int16, std::string>::iterator it = level_select_names.begin(); it != level_select_names.end(); ++it)
	{
		if (wadfile.HasWad(it->first))
		{
			wadfile.SetLevelName(it->first, utf8_to_mac_roman(it->second));
		}
	}

	wadfile.file_name(fs::basename(fs::path(dest).filename()));
	wadfile.Save(dest);
	
}
