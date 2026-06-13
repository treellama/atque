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
#include "ferro/Wadfile.h"

#include "CLUTResource.h"
#include "PICTResource.h"
#include "ResourceManager.h"
#include "SndResource.h"

#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace atque;
namespace algo = boost::algorithm;
namespace fs = std::filesystem;

const std::vector<uint32> physics_chunks = boost::assign::list_of
	(FOUR_CHARS_TO_INT('M','N','p','x'))
	(FOUR_CHARS_TO_INT('F','X','p','x'))
	(FOUR_CHARS_TO_INT('P','R','p','x'))
	(FOUR_CHARS_TO_INT('P','X','p','x'))
	(FOUR_CHARS_TO_INT('W','P','p','x'))
	;

static std::vector<uint8> ReadFile(const fs::path& path)
{
	std::ifstream infile(path, std::ios::binary | std::ios::ate);
	int length = infile.tellg();
	infile.seekg(0, std::ios::beg);

	std::vector<uint8> data(length);
	infile.read(reinterpret_cast<char *>(&data[0]), data.size());

	return data;
}

void MergePhysics(const fs::path& path, marathon::Wad& wad, std::ostream& log)
{
	marathon::Wadfile wadfile;
	if (wadfile.Load(path))
	{
		// check to make sure all physics are present
		marathon::Wad physics = wadfile.GetWad(0);
		for (std::vector<uint32>::const_iterator it = physics_chunks.begin(); it != physics_chunks.end(); ++it)
		{
			if (!physics.HasChunk(*it))
			{
				log << path << " is not a valid physics model; skipping" << std::endl;
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
	std::ifstream shapes(path, std::ios::binary | std::ios::ate);
	uint32 length = shapes.tellg();
	if (length <= 4096 * 1024)
	{
		std::vector<uint8> shapes_buffer(length);
		
		shapes.seekg(0);
		shapes.read(reinterpret_cast<char*>(&shapes_buffer[0]), shapes_buffer.size());
		wad.AddChunk(shapes_tag, shapes_buffer);
	}
	else
	{
		log << path << " is larger than 4 MB; skipping" << std::endl;
	}
}

void MergeSounds(const fs::path& path, marathon::Wad& wad, std::ostream& log)
{
	const uint32_t sounds_tag = FOUR_CHARS_TO_INT('S','n','P','a');
	std::ifstream sounds(path, std::ios::binary | std::ios::ate);
	uint32_t length = sounds.tellg();
	if (length <= 4096 * 1024)
	{
		std::vector<uint8_t> sounds_buffer(length);

		sounds.seekg(0);
		sounds.read(reinterpret_cast<char*>(sounds_buffer.data()), sounds_buffer.size());
		wad.AddChunk(sounds_tag, sounds_buffer);
	}
	else
	{
		log << path << " is longer than 4 MB; skipping" << std::endl;
	}
}

void MergeTerminal(const fs::path& path, marathon::Wad& wad, std::ostream& log)
{
	try 
	{
		marathon::TerminalChunk chunk;
		chunk.Compile(path);
		wad.AddChunk(marathon::TerminalChunk::kTag, chunk.Save());
	}
	catch (const marathon::TerminalChunk::ParseError& e)
	{
		log << path << ": " << e.what() << "; skipping" << std::endl;
	}
}

void MergeScripts(const std::vector<fs::path> paths, marathon::Wad& wad, uint32 tag)
{
	marathon::ScriptChunk chunk;
	for (std::vector<fs::path>::const_iterator it = paths.begin(); it != paths.end(); ++it)
	{
		marathon::ScriptChunk::Script script;
		script.name = it->stem();
		script.data = ReadFile(*it);
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
	std::vector<fs::path> sounds;
	std::vector<fs::path> terminals;
	std::vector<fs::path> luas;
	std::vector<fs::path> mmls;

	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		// skip hidden files
		if (dir_entry.path().filename().string()[0] == '.')
		{
			continue;
		}

		auto extension = dir_entry.path().extension();
		if (extension == ".sceA")
			maps.push_back(dir_entry);
		else if (extension == ".phyA")
			physics.push_back(dir_entry);
		else if (extension == ".ShPa")
			shapes.push_back(dir_entry);
		else if (extension == ".SnPa")
			sounds.push_back(dir_entry);
		else if (extension == ".txt")
			terminals.push_back(dir_entry);
		else if (extension == ".lua")
			luas.push_back(dir_entry);
		else if (extension == ".mml")
			mmls.push_back(dir_entry);
	}

	if (maps.size())
	{
		if (maps.size() > 1)
			log << path.string() << ": multiple maps found; using " << maps[0].string() << std::endl;

		marathon::Wadfile wadfile;
		if (wadfile.Load(maps[0].string()) && wadfile.data_version() == 1)
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
			if (sounds.size())
			{
				if (sounds.size() > 1)
				{
					log << path.string() << ": multiple sounds patches found; using " << sounds[0].string() << std::endl;
				}
				MergeSounds(sounds[0], wad, log);
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

void MergeCLUTs(marathon::ResourceManager& resource_manager,
				const fs::path& path)
{
	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		if (dir_entry.is_regular_file())
		{
			std::istringstream s(dir_entry.path().filename());
			int16 index;
			s >> index;
			if (!s.fail())
			{
				CLUTResource clut;
				if (clut.Import(dir_entry.path()))
				{
					resource_manager.resource_map()[std::make_pair(FOUR_CHARS_TO_INT('c','l','u','t'), index)] = clut.Save();
				}
			}
		}
	}
}

void MergePICTs(marathon::ResourceManager& resource_manager,
				const fs::path& path)
{
	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		if (dir_entry.is_regular_file())
		{
			std::istringstream s(dir_entry.path().filename());
			int16 index;
			s >> index;
			if (!s.fail())
			{
				PICTResource pict;
				if (pict.Import(dir_entry.path()))
				{
					resource_manager.resource_map()[std::make_pair(FOUR_CHARS_TO_INT('P','I','C','T'), index)] = pict.Save();
				}
			}
		}
	}
}

void MergeSnds(marathon::ResourceManager& resource_manager, const fs::path& path)
{
	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		if (dir_entry.is_regular_file())
		{
			std::istringstream s(dir_entry.path().filename());
			int16 index;
			s >> index;
			if (!s.fail())
			{
				SndResource snd;
				if (snd.Import(dir_entry.path()))
				{
					resource_manager.resource_map()[std::make_pair(FOUR_CHARS_TO_INT('s','n','d',' '), index)] = snd.Save();
				}
			}
		}
	}
}

void MergeTEXTs(marathon::ResourceManager& resource_manager,
				const fs::path& path)
{
	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		if (dir_entry.is_regular_file())
		{
			std::istringstream s(dir_entry.path().filename());
			int16 index;
			s >> index;
			if (!s.fail())
			{
				resource_manager.resource_map()[std::make_pair(FOUR_CHARS_TO_INT('T','E','X','T'), index)] = ReadFile(dir_entry);
			}
		}
	}	
}

static std::vector<uint8_t> convert_m1_term(const std::string& m1_term)
{
	auto in = utf8_to_mac_roman(m1_term);
	std::vector<uint8_t> out;

	char prev = 0;
	for (auto c : in)
	{
		if (c == '\n')
		{
			if (prev != '\r')
			{
				out.push_back('\r');
			}
		}
		else
		{
			out.push_back(c);
		}
		prev = c;
	}

	return out;
}

void MergeM1Terms(marathon::ResourceManager& resource_manager,
				  const fs::path& path)
{
	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		if (dir_entry.is_regular_file())
		{
			std::istringstream s(dir_entry.path().filename());
			int16 index;
			s >> index;
			if (!s.fail())
			{
				std::ifstream infile{dir_entry.path(), std::ios::ate};
				auto length = infile.tellg();
				infile.seekg(0);

				std::string m1_term(length, '\0');
				infile.read(m1_term.data(), m1_term.size());

				resource_manager.resource_map()[std::make_pair(FOUR_CHARS_TO_INT('t','e','r','m'), index)] = convert_m1_term(m1_term);
			}
		}
	}
}

void MergeResourceDir(marathon::ResourceManager& resource_manager,
					  const fs::path& path)
{
	try
	{
		uint32_t res_type = std::stoul(path.filename(), nullptr, 16);
		for (const auto& dir_entry : fs::directory_iterator{path})
		{
			std::istringstream s(dir_entry.path().filename());
			int16_t index;
			s >> index;
			if (!s.fail())
			{
				resource_manager.resource_map()[std::make_pair(res_type, index)] = ReadFile(dir_entry);
			}
		}
	}
	catch (const std::exception&)
	{
		
	}
}

void MergeResources(marathon::ResourceManager& resource_manager,
					const fs::path& path)
{
	for (const auto& dir_entry : fs::directory_iterator{path})
	{
		if (dir_entry.is_directory())
		{
			auto filename = dir_entry.path().filename();
			if (filename == "TEXT")
			{
				MergeTEXTs(resource_manager, dir_entry);
			}
			else if (filename == "CLUT")
			{
				MergeCLUTs(resource_manager, dir_entry);
			}
			else if (filename == "PICT")
			{
				MergePICTs(resource_manager, dir_entry);
			}
			else if (filename == "snd")
			{
				MergeSnds(resource_manager, dir_entry);
			}
			else if (filename == "term")
			{
				MergeM1Terms(resource_manager, dir_entry);
			}
			else
			{
				MergeResourceDir(resource_manager, dir_entry);
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

void atque::merge(const fs::path& src, const fs::path& dest, std::ostream& log)
{
	if (!fs::exists(src))
	{
		throw merge_error(src.string() + " does not exist");
	}
	else if (!fs::is_directory(src))
	{
		throw merge_error("source must be a directory");
	}

	marathon::ResourceManager resource_manager;

	if (fs::exists(src / "Data.bin"))
	{
		MergeResources(resource_manager, src / "Resources");
		if (resource_manager.CanSaveToResourceFork())
		{
			resource_manager.Save(dest, [&](std::ostream& stream) {
				std::ifstream data_fork{src / "Data.bin",
										std::ios::binary | std::ios::ate};
				auto length = data_fork.tellg();
				data_fork.seekg(0, std::ios::beg);

				std::vector<uint8_t> data(length);
				data_fork.read(reinterpret_cast<char*>(data.data()), data.size());
				stream.write(reinterpret_cast<char*>(data.data()), data.size());
			});
			return;
		}
		else
		{
			throw merge_error("merge folders with a raw data fork must contain a Resources directory");
		}
	}

	marathon::Wadfile wadfile;
	
	fs::path level_select_path(src);
	level_select_path = level_select_path / "Level Select Names.txt";
	
	std::map<int16, std::string> level_select_names;
	if (fs::exists(level_select_path))
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

	for (const auto& dir_entry : fs::directory_iterator{src})
	{
		if (dir_entry.is_directory())
		{
			if (dir_entry.path().filename() == "Resources")
			{
				MergeResources(resource_manager, dir_entry);
			}
			else
			{
				std::istringstream s(dir_entry.path().filename());
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
						wadfile.SetWad(index, CreateWad(dir_entry, log));
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

	wadfile.file_name(utf8_to_mac_roman(fs::path(dest).stem().u8string()));

	if (resource_manager.CanSaveToResourceFork())
	{
		resource_manager.Save(dest, [&](std::ostream& stream) {
			wadfile.Save(stream);
		});
	}
	else if (resource_manager.CanSaveToWadfile(wadfile))
	{
		resource_manager.SaveToWadfile(wadfile);

		std::ofstream stream(dest, std::ios_base::binary);
		wadfile.Save(stream);
	}
	else
	{
		throw merge_error("resources too big to save to fork, and resource ids overlap map levels");		
	}
}
