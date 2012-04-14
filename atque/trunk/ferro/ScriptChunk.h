/* ScriptChunk.h

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

#ifndef SCRIPTCHUNK_H
#define SCRIPTCHUNK_H

#include "cstypes.h"

#include <list>
#include <string>
#include <vector>

namespace marathon
{
	class ScriptChunk
	{
	public:
		static const int kScriptNameLength = 64 + 2;
		enum { kMMLTag = FOUR_CHARS_TO_INT('M','M','L','S'),
		       kLuaTag = FOUR_CHARS_TO_INT('L','U','A','S') };

		ScriptChunk() { }
		ScriptChunk(const std::vector<uint8>& data) { Load(data); }

		void Load(const std::vector<uint8>&);
		std::vector<uint8> Save() const;

		struct Script
		{
			std::string name;
			std::vector<uint8> data;
		};

		const std::list<Script>& GetScripts() { return scripts_; };
		void AddScript(const Script& s) { scripts_.push_back(s); }

		void Clear() { scripts_.clear(); }
		
	private:
		std::list<Script> scripts_;

	};
}

#endif
