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

#ifndef KEY_SPACE_H
#define KEY_SPACE_H

#include "common.h"
#include <list>
#include <string>

using namespace std;

struct perCharCharSet
{
	char **charSets;
	uint32 pwLength;
};

class KeySpace
{
public:
	KeySpace(const KeySpace &ks);
	KeySpace(const char *fileName);
	KeySpace(list<list<string> > subKeySpaces);
	KeySpace(perCharCharSet *subKeySpaces, uint32 numSubKeySpaces);
	virtual ~KeySpace();

	KeySpace& operator=(const KeySpace &ks);

//	bool reset(uint64 pwNum);
//	uint32 next(vector4 *pw, uint32 numPws, uint32 vecSize);
//	uint32 next(vector4 *pw, uint32 numPws, uint32 vecSize, uint32 *diffs);
	uint64 getKeySpace();
	uint32 getMaxPwLen();

protected:
//	bool cycleCache();
//	bool cycleCache(uint32 cachePos);
	void init(list<list<string> > subKeySpaces);
	void cleanUpKeySpace();
	void copyObject(const KeySpace &ks);

	uint64 m_keySpace;
	uint64 *m_keySpaceBoundaries;
	uint32 *m_passwordLengths, *m_divShortCut/*, *m_charSetPoses*/, **m_charSetLengths/*, *m_charSetCacheLengths*/;
	int *m_numShortCutChars;
/*	uint16 **m_charSetCache;*/
	char *m_charSet, ***m_subKeySpaces;
	uint32 m_numSubKeySpaces/*, m_cachePos, m_curSubKeySpace, m_pw[4]*/, m_maxPwLen;
};

#endif
