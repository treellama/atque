/* ScriptChunk.cpp

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

#include "ferro/AStream.h"
#include "ferro/ScriptChunk.h"

#include <algorithm>

using namespace marathon;

struct ScriptHeader
{
	static const int kSize = ScriptChunk::kScriptNameLength + 8;
	uint32 flags;
	char name[ScriptChunk::kScriptNameLength];
	uint32 length;
	
	ScriptHeader() : flags(0), length(0) { std::fill(name, name + ScriptChunk::kScriptNameLength, '\0'); }
	void Load(AIStreamBE& stream);
	void Save(AOStreamBE& stream) const;
};


void ScriptChunk::Load(const std::vector<uint8>& data)
{
	scripts_.clear();
	AIStreamBE stream(&data[0], data.size());

	uint16 num_scripts;
	stream >> num_scripts;

	for (int i = 0; i < num_scripts; ++i)
	{
		ScriptHeader header;
		header.Load(stream);

		Script script;
		script.name.assign(header.name);
		script.data.resize(header.length);
		stream.read(&script.data[0], script.data.size());
		
		scripts_.push_back(script);
	}
}

std::vector<uint8> ScriptChunk::Save() const
{
	// calculate size
	size_t size = 2; // number of scripts

	for (std::list<Script>::const_iterator it = scripts_.begin(); it != scripts_.end(); ++it)
	{
		size += ScriptHeader::kSize;
		size += it->data.size();
	}

	std::vector<uint8> v(size);
	AOStreamBE stream(&v[0], v.size());
	
	stream << static_cast<uint16>(scripts_.size());
	for (std::list<Script>::const_iterator it = scripts_.begin(); it != scripts_.end(); ++it)
	{
		ScriptHeader header;
		it->name.copy(header.name, kScriptNameLength - 1);
		header.length = it->data.size();
		
		header.Save(stream);
		stream.write(&it->data[0], it->data.size());
	}

	return v;
}

void ScriptHeader::Load(AIStreamBE& stream)
{
	stream >> flags;
	stream.read(name, ScriptChunk::kScriptNameLength);
	name[ScriptChunk::kScriptNameLength - 1] = '\0';
	stream >> length;
}

void ScriptHeader::Save(AOStreamBE& stream) const
{
	stream << flags;
	stream.write(name, ScriptChunk::kScriptNameLength);
	stream << length;
}
