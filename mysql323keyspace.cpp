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

#include "mysql323keyspace.h"
#include <string.h>

MySQL323KeySpace::MySQL323KeySpace(const KeySpace &ks)
	: KeySpace(ks)
{
	createMySQL323KeySpace();
}

MySQL323KeySpace::MySQL323KeySpace(const char *fileName)
	: KeySpace(fileName)
{
	createMySQL323KeySpace();
}

MySQL323KeySpace::MySQL323KeySpace(const MySQL323KeySpace &ks)
	: KeySpace(ks)
{
	copyObject(ks);
}

MySQL323KeySpace::MySQL323KeySpace(perCharCharSet *subKeySpaces, uint32 numSubKeySpaces)
	: KeySpace(subKeySpaces, numSubKeySpaces)
{
	createMySQL323KeySpace();
}

MySQL323KeySpace::~MySQL323KeySpace()
{
	cleanUpMySQL323KeySpace();
}

MySQL323KeySpace& MySQL323KeySpace::operator=(const KeySpace &ks)
{
	if (this != &ks)
	{
		cleanUpKeySpace();
		copyObject(ks);
		createMySQL323KeySpace();
	}
	return *this;
}

MySQL323KeySpace& MySQL323KeySpace::operator=(const MySQL323KeySpace &ks)
{
	if (this != &ks)
	{
		cleanUpKeySpace();
		copyObject((KeySpace) ks);
		copyObject(ks);
	}
	return *this;
}

int MySQL323KeySpace::reset(uint64 pwNum)
{
	uint64 quotient64;
	uint32 *charSetPoses, *charSetLengths;
	uint32 curSubKeySpace = 0, passwordLength, charSetLength, divisor, remainder, quotient;
	int a, ret, numShortCutChars, stop;

	if (pwNum >= m_keySpace)
	{
		return -1;
	}
	while (pwNum >= m_keySpaceBoundaries[curSubKeySpace + 1])
	{
		curSubKeySpace++;
	}
	pwNum -= m_keySpaceBoundaries[curSubKeySpace];
	passwordLength = m_passwordLengths[curSubKeySpace];
	numShortCutChars = m_numShortCutChars[curSubKeySpace];
	charSetPoses = m_charSetPoses;
	charSetLengths = m_charSetLengths[curSubKeySpace];
	m_curSubKeySpace = curSubKeySpace;
	a = (int) passwordLength - 1;
	if (numShortCutChars > 0)
	{
		divisor = m_divShortCut[curSubKeySpace];
		DIV_MOD_64_RET_32_32(pwNum, divisor, remainder, quotient);

		// First character of password
		charSetLength = charSetLengths[a];
		ret = (int) (charSetLength - remainder % charSetLength);
		remainder /= charSetLength;
		if (ret == (int) charSetLength)
		{
			ret = 0;
		}
		charSetPoses[a] = 0;

		// First part of password
		stop = (int) passwordLength - numShortCutChars;
		for (a--; a >= stop; a--)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = remainder % charSetLength;
			remainder /= charSetLength;
		}

		// Last part of password
		for (; a >= 0; a--)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = quotient % charSetLength;
			quotient /= charSetLength;
		}
	}
	else if (numShortCutChars < 0)
	{
		// Too big for DIV_MOD_64_RET_32_32
		numShortCutChars = -numShortCutChars;
		divisor = m_divShortCut[curSubKeySpace];
		remainder = pwNum % divisor;
		quotient64 = pwNum / divisor;

		// First character of password
		charSetLength = charSetLengths[a];
		ret = (int) (charSetLength - remainder % charSetLength);
		remainder /= charSetLength;
		if (ret == (int) charSetLength)
		{
			ret = 0;
		}
		charSetPoses[a] = 0;

		// First part of password
		stop = (int) passwordLength - numShortCutChars;
		for (a--; a >= stop; a--)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = remainder % charSetLength;
			remainder /= charSetLength;
		}

		// Second part of password
		for (; quotient64 >= UINT64_C(0x100000000); a--)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = quotient64 % charSetLength;
			quotient64 /= charSetLength;
		}

		// Last part of password
		quotient = (uint32) quotient64;
		for (; a >= 0; a--)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = quotient % charSetLength;
			quotient /= charSetLength;
		}
	}
	else if (passwordLength > 1)
	{
		quotient = (uint32) pwNum;

		// First character of password
		charSetLength = charSetLengths[a];
		ret = (int) (charSetLength - quotient % charSetLength);
		quotient /= charSetLength;
		if (ret == (int) charSetLength)
		{
			ret = 0;
		}
		charSetPoses[a] = 0;

		// The rest of password
		for (a--; a >= 0; a--)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = quotient % charSetLength;
			quotient /= charSetLength;
		}
	}
	else
	{
		charSetPoses[0] = 0;
		charSetLength = charSetLengths[0];
		ret = (int) (charSetLength - (uint32) pwNum);
		if (ret == (int) charSetLength)
		{
			ret = 0;
			charSetPoses[0] = -1;
		}
	}
	if (ret == 0 && passwordLength > 1)
	{
		charSetPoses[passwordLength - 2]--;
	}
	return ret;
}

uint32 MySQL323KeySpace::next(char *pwPrefix, uint32 &pwPrefixLen, uint32 &newCharsStart, char **charSet, uint32 &charSetLen)
{
	uint32 *charSetPoses, *charSetLengths;
	char **subKeySpaces;
	uint32 curSubKeySpace, passwordLength;
	int a;
	bool incCurSubKeySpace = false;

	curSubKeySpace = m_curSubKeySpace;
	passwordLength = m_passwordLengths[curSubKeySpace];
	charSetPoses = m_charSetPoses;
	charSetLengths = m_charSetLengths[curSubKeySpace];
	subKeySpaces = m_subKeySpaces[curSubKeySpace];
	if (passwordLength > 1)
	{
		for (a = (int) passwordLength - 2; a >= 0; a--)
		{
			charSetPoses[a]++;
			pwPrefix[a] = subKeySpaces[a][charSetPoses[a]]; // This can read unallocated memory but is not used
			if (charSetPoses[a] < charSetLengths[a])
			{
				break;
			}
			pwPrefix[a] = subKeySpaces[a][0];
			charSetPoses[a] = 0;
		}
		if (a < 0)
		{
			incCurSubKeySpace = true;
		}
		else if (pwPrefix[0] == subKeySpaces[0][charSetPoses[0]])
		{
			newCharsStart = a;
		}
		else
		{
			// First time running next after running reset
			// Note that caller should set pwPrefix[0] = 0 before calling next if there was a reset
			newCharsStart = 0;
			for (; a >= 0; a--)
			{
				pwPrefix[a] = subKeySpaces[a][charSetPoses[a]];
			}
		}
	}
	else if (charSetPoses[0] == 0)
	{
		incCurSubKeySpace = true;
	}
	else
	{
		charSetPoses[0] = 0;
		newCharsStart = 0;
	}
	if (incCurSubKeySpace)
	{
		curSubKeySpace++;
		if (curSubKeySpace >= m_numSubKeySpaces)
		{
			pwPrefix[0] = 0;
			pwPrefixLen = 0;
			newCharsStart = 0;
			charSet = NULL;
			charSetLen = 0;
			return 1;
		}
		m_curSubKeySpace = curSubKeySpace;
		passwordLength = m_passwordLengths[curSubKeySpace];
		charSetLengths = m_charSetLengths[curSubKeySpace];
		subKeySpaces = m_subKeySpaces[curSubKeySpace];
		for (a = 0; a < (int) passwordLength; a++)
		{
			charSetPoses[a] = 0;
			pwPrefix[a] = subKeySpaces[a][0];
		}
		newCharsStart = 0;
	}
	pwPrefixLen = passwordLength - 1;
	*charSet = subKeySpaces[passwordLength - 1];
	charSetLen = charSetLengths[passwordLength - 1];
	return 0;
}

void MySQL323KeySpace::cleanUpMySQL323KeySpace()
{
	delete [] m_charSetPoses;
}

void MySQL323KeySpace::copyObject(const MySQL323KeySpace &ks)
{
	m_charSetPoses = new uint32[m_maxPwLen];
	memcpy(m_charSetPoses, ks.m_charSetPoses, sizeof(uint32) * m_maxPwLen);
	m_curSubKeySpace = ks.m_curSubKeySpace;
}

void MySQL323KeySpace::createMySQL323KeySpace()
{
	m_charSetPoses = new uint32[m_maxPwLen];
	memset(m_charSetPoses, 0, sizeof(uint32) * m_maxPwLen);
	m_curSubKeySpace = 0;
}
