/* filesystem.h

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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

namespace fs
{
	class path
	{
	public:
#ifdef __WIN32__
		static const char PATH_SEP = '\\';
#else
		static const char PATH_SEP = '/';
#endif
		path() { }
		path(const std::string& initial_path) : path_(initial_path) {
			canonicalize();
		}


		bool create_directory() const {
#ifdef __WIN32__
			return (mkdir(path_.c_str()) == 0);
#else
			return (mkdir(path_.c_str(), 0777) == 0);
#endif
		}

		bool exists() const {
			return (access(path_.c_str(), R_OK) == 0);
		}

		bool is_directory() const {
			struct stat st;
			if (stat(path_.c_str(), &st) < 0)
				return false;
			return (S_ISDIR(st.st_mode));
		}

		const path operator/ (const std::string& addition) const {
			path result = *this;
			if (result.path_.size() && result.path_[result.path_.size() - 1] != '/')
				result.path_ += PATH_SEP;
			result.path_ += addition;
			result.canonicalize();
			return result;
		}

		const std::string string() const {
			return path_;
		}

		std::string filename() const {
			std::string::size_type pos = path_.rfind(PATH_SEP);
			if (pos == std::string::npos)
				return path_;
			else
				return path_.substr(pos + 1);
		}

		path parent() const {
			std::string::size_type pos = path_.rfind(PATH_SEP);
			if (pos == std::string::npos)
				return path("");
			else
				return path(path_.substr(0, pos - 1));
		}

		std::vector<path> ls() const {
			std::vector<path> result;
			DIR *d = opendir(path_.c_str());
			if (!d)
				return result;

			dirent *de = readdir(d);
			while (de)
			{
				std::string name(de->d_name);
				if (name != "." && name != "..")
				{
					path p = *this / std::string(de->d_name);
					struct stat st;
					if (stat(p.string().c_str(), &st) == 0)
						result.push_back(p);
				}
				de = readdir(d);
			}
			closedir(d);
			return result;
		}

	private:
		void canonicalize() {
#ifndef __WIN32__
			std::string::size_type pos;
			while ((pos = path_.find("//")) != std::string::npos)
				path_.erase(pos);
#endif
			// remove trailing '/'
			if (path_.size() > 1 && path_[path_.size() - 1] == PATH_SEP)
				path_.erase(path_.size() - 1);
		}

		std::string path_;
	};

	static bool exists(const std::string& path_)
	{
		return path(path_).exists();
	}

	static bool is_directory(const std::string& path_)
	{
		return path(path_).is_directory();
	}

	static bool create_directory(const std::string& path_)
	{
		return path(path_).create_directory();
	}

	static std::string extension(const std::string& filename)
	{
		std::string::size_type pos = filename.rfind('.');
		if (pos != std::string::npos)
		{
			return filename.substr(pos);
		}
		else
		{
			return "";
		}
	}

	static std::string basename(const std::string& filename)
	{
		std::string::size_type pos = filename.rfind('.');
		if (pos != std::string::npos)
		{
			return filename.substr(0, pos - 1);
		}
		else
			return filename;
	}
}

#endif
