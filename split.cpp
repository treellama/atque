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
#include "ferro/ScriptChunk.h"
#include "ferro/TerminalChunk.h"
#include "ferro/Wadfile.h"

#include "split.h"
#include "CLUTResource.h"
#include "PICTResource.h"
#include "ResourceManager.h"
#include "SndResource.h"

#include <filesystem>
#include <iostream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <set>

#include <boost/assign/list_of.hpp>

#if defined(__APPLE__) && defined(__MACH__)
#include <string.h>
#include <sys/attr.h>
#include <unistd.h>
#include <sys/vnode.h>

struct FInfoAttrBuf {
	unsigned long length;
	fsobj_type_t objType;
	char finderInfo[32];
};

static void set_type_code(const std::string& path, const std::string& type)
{
	if (type.size() != 4) 
		return;
	attrlist attrList = { ATTR_BIT_MAP_COUNT, 0, ATTR_CMN_OBJTYPE | ATTR_CMN_FNDRINFO, 0, 0, 0, 0 };
	FInfoAttrBuf attrBuf;
	if (getattrlist(path.c_str(), &attrList, &attrBuf, sizeof(attrBuf), 0) || attrBuf.objType != VREG)
		return;

	type.copy(attrBuf.finderInfo, 4);
	
	attrList.commonattr = ATTR_CMN_FNDRINFO;
	setattrlist(path.c_str(), &attrList, attrBuf.finderInfo, sizeof(attrBuf.finderInfo), 0);
}
#else
static void set_type_code(const std::string&, const std::string&) { }
#endif

using namespace atque;
namespace fs = std::filesystem;

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
		set_type_code(path, "phy\260");
	}
}

void SaveLevel(marathon::Wad& wad, const std::string& name, const fs::path& path)
{
	// export everything remaining in the wad file!
	marathon::Wadfile wadfile;
	wadfile.SetWad(0, wad);
	wadfile.file_name(name);
	wadfile.Save(path);
	set_type_code(path, "sce2");
}

// removes shapes chunk from Wad, and saves file
void SaveShapes(marathon::Wad& wad, const fs::path& path)
{
	const uint32 shapes_tag = FOUR_CHARS_TO_INT('S','h','P','a');
	if (wad.HasChunk(shapes_tag))
	{
		const std::vector<uint8>& data = wad.GetChunk(shapes_tag);
		if (data.size())
		{
			std::ofstream outfile(path, std::ios::trunc | std::ios::binary);
			outfile.write(reinterpret_cast<const char*>(&data[0]), data.size());
			set_type_code(path, "ShPa");
		}
		wad.RemoveChunk(shapes_tag);
	}
}

void SaveSounds(marathon::Wad& wad, const fs::path& path)
{
	const uint32_t sounds_tag = FOUR_CHARS_TO_INT('S','n','P','a');
	if (wad.HasChunk(sounds_tag))
	{
		const std::vector<uint8_t>& data = wad.GetChunk(sounds_tag);
		if (data.size())
		{
			std::ofstream outfile(path, std::ios::trunc | std::ios::binary);
			outfile.write(reinterpret_cast<const char*>(data.data()), data.size());
			set_type_code(path, "SnPa");
		}
		wad.RemoveChunk(sounds_tag);
	}
}

void SaveScripts(marathon::Wad& wad, const fs::path& dir)
{
	using marathon::ScriptChunk;

	if (wad.HasChunk(ScriptChunk::kLuaTag))
	{
		ScriptChunk luas(wad.GetChunk(ScriptChunk::kLuaTag));
		std::list<ScriptChunk::Script> scripts = luas.GetScripts();
		for (std::list<ScriptChunk::Script>::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
		{
			if (it->data.size())
			{
				auto path = dir;
				path /= fs::u8path(it->name);
				path += ".lua";

				std::ofstream outfile(path, std::ios::trunc | std::ios::binary);
				outfile.write(reinterpret_cast<const char*>(&it->data[0]), it->data.size());
			}
		}

		wad.RemoveChunk(ScriptChunk::kLuaTag);
	}

	if (wad.HasChunk(ScriptChunk::kMMLTag))
	{
		ScriptChunk mmls(wad.GetChunk(ScriptChunk::kMMLTag));
		std::list<ScriptChunk::Script> scripts = mmls.GetScripts();
		for (std::list<ScriptChunk::Script>::const_iterator it = scripts.begin(); it != scripts.end(); ++it)
		{
			if (it->data.size())
			{
				auto path = dir;
				path /= fs::u8path(it->name);
				path += ".mml";

				std::ofstream outfile(path, std::ios::trunc | std::ios::binary);
				outfile.write(reinterpret_cast<const char*>(&it->data[0]), it->data.size());
			}
		}

		wad.RemoveChunk(ScriptChunk::kMMLTag);
		
	}

}

void SaveTEXT(const std::vector<uint8_t>& data, const fs::path& path)
{
	if (data.size())
	{
		std::ofstream outfile(path, std::ios::trunc);
		outfile.write(reinterpret_cast<const char*>(&data[0]), data.size());
	}
}

void SaveTerminal(marathon::Wad& wad, const fs::path& path)
{
	if (wad.HasChunk(marathon::TerminalChunk::kTag))
	{
		marathon::TerminalChunk chunk;
		chunk.Load(wad.GetChunk(marathon::TerminalChunk::kTag));
		chunk.Decompile(path.string());
		wad.RemoveChunk(marathon::TerminalChunk::kTag);
	}
}

static std::string sanitize(const std::string& input)
{
	std::string result;
	for (std::string::const_iterator it = input.begin(); it != input.end(); ++it)
	{
		switch (*it)
		{
		case '/':
#if defined(__WIN32__) || (defined(__APPLE__) && defined(__MACH__))
		case ':':
#endif
#ifdef __WIN32__
		case '\\':
		case '"':
		case '*':
		case '<':
		case '>':
		case '?':
		case '|':
#endif
			result.push_back('-');
			break;
		default:
			if (static_cast<unsigned char>(*it) >= ' ')
			{
				result.push_back(*it);
			}
			break;
		}
	}

#ifdef __WIN32__
	// strings can't end with space or dot!?
	result.erase(result.find_last_not_of(" .") + 1);
#endif

	// erase leading dots so files don't appear hidden in Linux
	result.erase(0, result.find_first_not_of("."));
	
	if (result == "")
	{
		result = "Unnamed Level";
	}

	return result;
}

void atque::split(const fs::path& src, const fs::path& dest, std::ostream& log)
{
	if (!fs::exists(src))
	{
		throw split_error(src.string() + " does not exist");
	}
	
	if (fs::exists(dest) && !fs::is_directory(dest))
	{
		throw split_error("destination must be a directory");
	}

	marathon::ResourceManager resource_manager;
	std::optional<marathon::Wadfile> wadfile;
	std::optional<std::vector<uint8_t>> data_fork;
	
	if (!resource_manager.Load(src, [&](std::istream& stream,
										std::streamsize length)
		{
			auto start = stream.tellg();
			
			marathon::Wadfile check_wad;
			if (check_wad.Load(stream) &&
				check_wad.version() >= 1 &&
				check_wad.data_version() == 1 &&
				check_wad.GetWadIndexes().size())
			{
				resource_manager.LoadFromWadfile(check_wad);
				wadfile = std::move(check_wad);
			}
			else
			{
				stream.seekg(start);

				std::vector<uint8_t> data(length);
				stream.read(reinterpret_cast<char*>(data.data()), data.size());
				data_fork = std::move(data);
			}
		}))
	{
		throw split_error("error loading resource or data fork");
	};

	if (!fs::exists(dest))
	{
		if (!fs::create_directory(dest))
		{
			throw split_error("could not create directory");
		}
	}

	std::map<int16, std::string> level_select_names;

	if (wadfile)
	{
		for (const auto& index : wadfile->GetWadIndexes())
		{
			marathon::Wad wad = wadfile->GetWad(index);
			if (wad.HasChunk(marathon::MapInfo::kTag))
			{
				try
				{
					marathon::MapInfo minf(wad.GetChunk(marathon::MapInfo::kTag));
					
					std::string level = wadfile->GetLevelName(index);
					std::string actual_level = minf.level_name();
					if (level != actual_level)
					{
						level_select_names[index] = level;
					}
					
					level = sanitize(level);
					actual_level = sanitize(actual_level);
					
					std::ostringstream level_number;
					level_number << std::setw(2) << std::setfill('0') << index;
					
					auto destfolder = dest;
					destfolder /= level_number.str() + " ";
					destfolder += fs::u8path(mac_roman_to_utf8(level));
					fs::create_directory(destfolder);
					
					auto physics_path = destfolder;
					physics_path /= fs::u8path(mac_roman_to_utf8(actual_level));
					physics_path += ".phyA";
					SavePhysics(wad, actual_level, physics_path);
					
					auto shapes_path = destfolder;
					shapes_path = fs::u8path(mac_roman_to_utf8(actual_level));
					shapes_path += ".ShPa";
					SaveShapes(wad, shapes_path);
					
					auto sounds_path = destfolder;
					sounds_path /= fs::u8path(mac_roman_to_utf8(actual_level));
					sounds_path += ".SnPa";
					SaveSounds(wad, sounds_path);
					
					auto terminal_path = destfolder;
					terminal_path /= fs::u8path(mac_roman_to_utf8(actual_level));
					terminal_path += ".term.txt";
					SaveTerminal(wad, terminal_path.string());
					
					SaveScripts(wad, destfolder);
					
					auto level_path = destfolder;
					level_path /= fs::u8path(mac_roman_to_utf8(actual_level));
					level_path += ".sceA";
					SaveLevel(wad, actual_level, level_path.string());
				}
				catch (const std::exception&)
				{
					std::ostringstream error;
					error << "error writing level " << index << "; aborting";
					throw split_error(error.str());
				}
			}				
		}
	}
	else
	{
		fs::path data_fork_path(dest);
		data_fork_path = data_fork_path / "Data.bin";
		std::ofstream s{data_fork_path.string().c_str(), std::ios::out | std::ios::binary | std::ios::trunc};
		s.write(reinterpret_cast<char*>(data_fork->data()), data_fork->size());
	}

	fs::path resource_path = fs::path(dest) / "Resources";
	if (resource_manager.resource_map().size())
	{
		fs::create_directory(resource_path);
	}

	std::map<int16, std::string> resource_names;

	for (const auto& [res_id, res_data] : resource_manager.resource_map())
	{
		const auto& [res_type, res_index] = res_id;
		
		std::ostringstream id;
		id << std::setw(5) << std::setfill('0') << res_index;

		if (res_type == FOUR_CHARS_TO_INT('P','I','C','T') ||
			res_type == FOUR_CHARS_TO_INT('p','i','c','t'))
		{
			auto pict_dir = resource_path / "PICT";
			fs::create_directory(pict_dir);
			
			auto pict_path = pict_dir / id.str(); 
			PICTResource pict;
			if (res_type == FOUR_CHARS_TO_INT('P','I','C','T'))
			{
				pict.Load(res_data);
			}
			else
			{
				auto clut_data = resource_manager.resource_map()[std::make_pair(FOUR_CHARS_TO_INT('c','l','u','t'), res_index)];
				pict.LoadRaw(res_data, clut_data);
			}

			if (pict.IsUnparsed())
				log << "Exporting PICT " << res_index << " as .pct (" << pict.WhyUnparsed() << ")" << std::endl;

			pict.Export(pict_path.string());
		}
		else if (res_type == FOUR_CHARS_TO_INT('T','E','X','T') ||
				 res_type == FOUR_CHARS_TO_INT('t','e','x','t'))
		{
			auto text_dir = resource_path / "TEXT";
			fs::create_directory(text_dir);

			auto text_path = text_dir / (id.str() + ".txt");
			SaveTEXT(res_data, text_path.string());
		}
		else if (res_type == FOUR_CHARS_TO_INT('c','l','u','t'))
		{
			auto clut_dir = resource_path / "CLUT";
			fs::create_directory(clut_dir);
			
			auto clut_path = clut_dir / (id.str() + ".act");
			CLUTResource clut(res_data);
			clut.Export(clut_path.string());
		}
		else if (res_type == FOUR_CHARS_TO_INT('s','n','d',' '))
		{
			auto snd_dir = resource_path / "snd";
			fs::create_directory(snd_dir);
			
			auto snd_path = snd_dir / (id.str() + ".wav");
			SndResource snd(res_data);
			snd.Export(snd_path.string());
		}
		else
		{
			std::ostringstream oss;
			oss << std::hex << std::setw(8) << std::setfill('0') << res_type;
			auto res_dir = resource_path / oss.str();
			
			fs::create_directory(res_dir);
			
			fs::path res_path = res_dir / (id.str() + ".bin");
			std::ofstream stream{res_path.string(), std::ios_base::binary | std::ios_base::trunc};
			stream.write(reinterpret_cast<const char*>(res_data.data()), res_data.size());
		}
	}

	if (level_select_names.size())
	{
		fs::path level_select_path(dest);
		level_select_path = level_select_path / "Level Select Names.txt";
		std::ofstream s(level_select_path.string().c_str(), std::ios::out);
		for (std::map<int16, std::string>::iterator it = level_select_names.begin(); it != level_select_names.end(); ++it)
		{
			s << it->first << " " << mac_roman_to_utf8(it->second) << std::endl;
		}
	}
}
