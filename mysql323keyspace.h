/*
	Copyright (C) 2011 Steve Thomas <steve AT tobtu DOT com>

	This file is part of MySQL323 Cracker.

	MySQL323 Cracker is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MySQL323 Cracker is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MySQL323 Cracker.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MYSQL323_KEY_SPACE_H
#define MYSQL323_KEY_SPACE_H

#include "keyspace.h"

class MySQL323KeySpace : public KeySpace
{
public:
	MySQL323KeySpace(const KeySpace &ks);
	MySQL323KeySpace(const char *fileName);
	MySQL323KeySpace(const MySQL323KeySpace &ks);
	MySQL323KeySpace(perCharCharSet *subKeySpaces, uint32 numSubKeySpaces);
	~MySQL323KeySpace();

	MySQL323KeySpace& operator=(const KeySpace &ks);
	MySQL323KeySpace& operator=(const MySQL323KeySpace &ks);

	int reset(uint64 pwNum);
	uint32 next(char *pwPrefix, uint32 &pwPrefixLen, uint32 &newCharsStart, char **charSet, uint32 &charSetLen);

private:
	void cleanUpMySQL323KeySpace();
	void copyObject(const MySQL323KeySpace &ks);
	void createMySQL323KeySpace();

	uint32 *m_charSetPoses;
	uint32 m_curSubKeySpace;
};

#endif
