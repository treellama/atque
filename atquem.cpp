/* atquem.cpp

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

#include <iostream>
#include <string>
#include <vector>

#include "merge.h"

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: atquem <source> <dest>" << std::endl;
		return 1;
	}

	try {
		std::vector<std::string> merge_log;
		atque::merge(argv[1], argv[2], merge_log);
	}
	catch (const atque::merge_error& e)
	{
		std::cerr << "atquem: " << e.what() << std::endl;
	}

	return 0;
}

