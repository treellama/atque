/* merge.h

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

#ifndef MERGE_H
#define MERGE_H

#include <stdexcept>
#include <string>
#include <vector>

namespace atque
{
	class merge_error : public std::runtime_error
	{
	public:
		merge_error(const std::string& what) : std::runtime_error(what) { }
	};

	void merge(const std::string& source, const std::string& destination, std::vector<std::string>& log);
}

#endif
