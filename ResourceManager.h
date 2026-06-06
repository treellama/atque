/* ResourceManager.h

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

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <string>

namespace marathon
{
class Wadfile;

class ResourceManager
{
public:
	static constexpr auto max_resource_fork_data_size = 2U << 24;
	
	using res_id_t = std::pair<uint32_t, int16_t>;
	using res_data_t = std::vector<uint8_t>;
	using res_map_t = std::map<res_id_t, res_data_t>;
	using name_map_t = std::map<res_id_t, std::string>;

	using read_data_cb = std::function<void(std::istream& stream,
											std::streamsize length)>;
	using write_data_cb = std::function<void(std::ostream& stream)>;

	const res_map_t& resource_map() const { return resource_map_; }
	res_map_t& resource_map() { return resource_map_; }

	const name_map_t& name_map() const { return name_map_; }
	name_map_t& name_map() { return name_map_; }

	bool Load(const std::filesystem::path& path, read_data_cb read_data_fork);
	void Save(const std::filesystem::path& path, write_data_cb write_data_fork);

	bool CanSaveToWadfile(Wadfile& wadfile);
	bool CanSaveToResourceFork();

	// win95 resources are stored as chunks in levels in the wadfile
	void LoadFromWadfile(Wadfile& wadfile);
	void SaveToWadfile(Wadfile& wadfile);

private:
	bool LoadResourceMap(std::istream& stream, std::streamsize length);
	void LoadResourceFork(std::istream& stream, std::streamsize length);
	void SaveResourceFork(std::ostream& stream);
	
	res_map_t resource_map_;
	name_map_t name_map_;
};

}

#endif
