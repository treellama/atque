/* Unimap.h

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

/* Unimap is a Wadfile that can handle resource forks / embedded resources */

#ifndef UNIMAP_H
#define UNIMAP_H

#include "ferro/Wadfile.h"
#include <iostream>

namespace marathon
{
	class Unimap : public Wadfile
	{
	public:
		typedef std::pair<uint32, int16> ResourceIdentifier
;
		bool HasResource(uint32 type, int16 id) { return HasResource(ResourceIdentifier(type, id)); }
		bool HasResource(ResourceIdentifier id);

		const std::vector<uint8>& GetResource(uint32 type, int16 id) { return GetResource(ResourceIdentifier(type, id)); }
		const std::vector<uint8>& GetResource(ResourceIdentifier id);

		std::string GetResourceName(int16 id);

		void SetResource(uint32 type, int16 id, const std::vector<uint8>& data) { SetResource(ResourceIdentifier(type, id), data); }
		void SetResource(ResourceIdentifier id, const std::vector<uint8>& data);

		void SetResourceName(int16 id, const std::string& name) { SetLevelName(id, name); }

		std::vector<ResourceIdentifier> GetResourceIdentifiers();

	private:
		bool LoadMacBinary();
		bool Load(const std::string& path);
		void LoadResourceFork(std::istream& stream, std::streamsize size);

		virtual void Seek(std::streampos pos);

		void LoadResource(ResourceIdentifier id);

		std::streampos data_fork_;
		std::streamsize data_length_;

		// loaded resources
		std::map<ResourceIdentifier, std::vector<uint8> > resources_;
		std::map<int16, std::string> names_;
	};
};

#endif
