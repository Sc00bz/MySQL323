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

#include "keyspace.h"
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <list>

using namespace std;

void readLineTrim(ifstream &fin, string &line)
{
	uint32 lastNonBlank = 0xffffffff;
	char ch;

	line = "";
	ch = fin.get();
	while (fin.good())
	{
		if (ch == '\r')
		{
			if (fin.peek() == '\n')
			{
				fin.get();
			}
			break;
		}
		if (ch == '\n')
		{
			break;
		}
		if (ch == ' ' || ch == '\t')
		{
			if (lastNonBlank != 0xffffffff)
			{
				line += ch;
			}
		}
		else
		{
			line += ch;
			lastNonBlank = line.length();
		}
		ch = fin.get();
	}
	if (lastNonBlank != 0xffffffff && lastNonBlank != line.length())
	{
		line = line.substr(0, lastNonBlank);
	}
}

string hex2bin(const string &hexStr)
{
	string ret;
	uint32 a, len = hexStr.length();
	char ch, ch2;

	if ((len & 1) != 0)
	{
		return "";
	}
	for (a = 0; a < len; a += 2)
	{
		ch = hexStr[a];
		if (ch >= '0' && ch <= '9')
		{
			ch2 = (ch - '0') << 4;
		}
		else if (ch >= 'A' && ch <= 'F')
		{
			ch2 = (ch - 'A' + 10) << 4;
		}
		else if (ch >= 'a' && ch <= 'f')
		{
			ch2 = (ch - 'a' + 10) << 4;
		}
		else
		{
			return "";
		}
		ch = hexStr[a + 1];
		if (ch >= '0' && ch <= '9')
		{
			ch2 |= ch - '0';
		}
		else if (ch >= 'A' && ch <= 'F')
		{
			ch2 |= ch - 'A' + 10;
		}
		else if (ch >= 'a' && ch <= 'f')
		{
			ch2 |= ch - 'a' + 10;
		}
		else
		{
			return "";
		}
		ret += ch2;
	}
	return ret;
}

KeySpace::KeySpace(const KeySpace &ks)
{
	copyObject(ks);
}

KeySpace::KeySpace(const char *fileName)
{
	ifstream fin(fileName, ifstream::in);
	string line, name, charSet;
	map<string,string> charSets;
	map<string,string>::iterator charSetMapIt;
	list<string> subKeySpace;
	list<list<string> > subKeySpaces;
	list<string>::iterator charSetIt;
	list<list<string> >::iterator subKeySpaceIt;
	size_t pos, pos2;
	uint32 len, lineNumber = 0;
	bool multiCharNames = false;
	char ch[2] = {0, 0};

	if (!fin.good())
	{
		fprintf(stderr, "ERROR: Can't open key space file '%s'\n", fileName);
		exit(1);
	}

	// Read character sets
	while (1)
	{
		// Read line
		readLineTrim(fin, line);
		lineNumber++;
		if (!fin.good() && !fin.eof())
		{
			fprintf(stderr, "ERROR: Unable to read line #%u from key space file '%s'\n", lineNumber, fileName);
			exit(1);
		}
		if (line.length() == 0 || line[0] == '#')
		{
			if (fin.eof())
			{
				fprintf(stderr, "ERROR: '%s' is not a valid key space file\n", fileName);
				exit(1);
			}
			continue;
		}
		if (line[line.length() - 1] != ']')
		{
			break;
		}

		// Get character set name
		pos = line.find_first_of(" \t", 0, 2);
		if (pos == string::npos)
		{
			break;
		}
		name = line.substr(0, pos);

		// Get character set
		pos = line.find_first_not_of(" \t", pos, 2);
		if (pos == string::npos)
		{
			break; // this is not even possible... meh
		}
		if (line[pos] == '[')
		{
			charSet = line.substr(pos + 1, line.length() - pos - 2);
		}
		else if (line[pos] == 'h' && line[pos + 1] == '[')
		{
			charSet = hex2bin(line.substr(pos + 2, line.length() - pos - 3));
			if (charSet.length() == 0 && line.length() - pos - 3 != 0)
			{
				fprintf(stderr, "ERROR: '%s' has an invalid character set on line #%u\n", fileName, lineNumber);
				exit(1);
			}
		}
		else
		{
			break;
		}
		if (charSet.length() == 0)
		{
			fprintf(stderr, "ERROR: '%s' has an empty character set on line #%u\n", fileName, lineNumber);
			exit(1);
		}

		// Insert into map
		if (charSets.find(name) == charSets.end())
		{
			if (name.length() != 1)
			{
				multiCharNames = true;
			}
			charSets[name] = charSet;
		}
		else
		{
			fprintf(stderr, "ERROR: '%s' has a duplicate character set name on line #%u\n", fileName, lineNumber);
			exit(1);
		}
	}
	if (line[line.length() - 1] == ']')
	{
		fprintf(stderr, "ERROR line #%u: '%s' is not a valid key space file\n", lineNumber, fileName);
		exit(1);
	}

	// Read sub key spaces
	do
	{
		if (multiCharNames)
		{
			pos = 0;
			while (pos != string::npos)
			{
				// Get character set name
				pos2 = line.find_first_of(" \t", pos, 2);
				if (pos2 == string::npos)
				{
					pos2 = line.length();
				}
				name = line.substr(pos, pos2 - pos);

				// Get character set
				charSetMapIt = charSets.find(name);
				if (charSetMapIt == charSets.end())
				{
					fprintf(stderr, "ERROR: Character set name '%s' not found on line #%u from key space file '%s'\n", name.c_str(), lineNumber, fileName);
					exit(1);
				}
				subKeySpace.push_back(charSetMapIt->second);

				if (pos2 == line.length())
				{
					break;
				}
				pos = line.find_first_not_of(" \t", pos2, 2);
			}
		}
		else
		{
			len = line.length();
			for (pos = 0; pos < len; pos++)
			{
				*ch = line[pos];
				if (*ch != ' ' && *ch != '\t')
				{
					charSetMapIt = charSets.find(ch);
					if (charSetMapIt == charSets.end())
					{
						fprintf(stderr, "ERROR: Character set name '%c' not found on line #%u from key space file '%s'\n", *ch, lineNumber, fileName);
						exit(1);
					}
					subKeySpace.push_back(charSetMapIt->second);
				}
			}
		}
		subKeySpaces.push_back(subKeySpace);
		subKeySpace.clear();

		// Read line
		do
		{
			readLineTrim(fin, line);
			lineNumber++;
			if (!fin.good() && !fin.eof())
			{
				fprintf(stderr, "ERROR: Unable to read line #%u from key space file '%s'\n", lineNumber, fileName);
				exit(1);
			}
		} while ((line.length() == 0 || line[0] == '#') && fin.good());
	} while (line.length() != 0);

	init(subKeySpaces);
}

KeySpace::KeySpace(list<list<string> > subKeySpaces)
{
	init(subKeySpaces);
}

void KeySpace::init(list<list<string> > subKeySpaces)
{
	uint64 keySpace, keySpaceBoundary = 0;
	list<string> uniqueCharSets;
	list<string>::iterator uniqueCharSetsIt;
	list<string>::reverse_iterator charSetItRev;
	list<string>::iterator charSetIt;
	list<list<string> >::iterator subKeySpaceIt;
	uint32 a, b, charSetLen = 0, maxPwLen = 1;
	sint32 charSetDiffLen;

	m_numSubKeySpaces = subKeySpaces.size();
	m_keySpaceBoundaries = new uint64 [m_numSubKeySpaces + 1];
	m_passwordLengths    = new uint32 [m_numSubKeySpaces];
	m_divShortCut        = new uint32 [m_numSubKeySpaces];
	m_numShortCutChars   = new int    [m_numSubKeySpaces];
	m_charSetLengths     = new uint32*[m_numSubKeySpaces];
	m_subKeySpaces       = new char** [m_numSubKeySpaces];

	m_keySpaceBoundaries[0] = 0;
	for (a = 0, subKeySpaceIt = subKeySpaces.begin(); subKeySpaceIt != subKeySpaces.end(); ++subKeySpaceIt, a++)
	{
		if (maxPwLen < subKeySpaceIt->size())
		{
			maxPwLen = subKeySpaceIt->size();
		}
		keySpace = 1;
		m_passwordLengths[a] = subKeySpaceIt->size();
		m_charSetLengths[a] = new uint32[subKeySpaceIt->size()];
		for (b = subKeySpaceIt->size() - 1, charSetItRev = subKeySpaceIt->rbegin(); charSetItRev != subKeySpaceIt->rend(); ++charSetItRev, b--)
		{
			// Calculate key space
			keySpace *= charSetItRev->length();
			if (keySpace < UINT64_C(0x100000000))
			{
				m_divShortCut[a] = (uint32) keySpace;
				m_numShortCutChars[a] = subKeySpaceIt->size() - b;
			}

			// Find unique character sets
			m_charSetLengths[a][b] = charSetItRev->length();
			for (uniqueCharSetsIt = uniqueCharSets.begin(); uniqueCharSetsIt != uniqueCharSets.end(); ++uniqueCharSetsIt)
			{
				charSetDiffLen = m_charSetLengths[a][b] - uniqueCharSetsIt->length();
				if (charSetDiffLen > 0)
				{
					if (charSetItRev->find(*uniqueCharSetsIt) != string::npos)
					{
						*uniqueCharSetsIt = *charSetItRev;
						charSetLen += charSetDiffLen;
						break;
					}
				}
				else if (charSetDiffLen < 0)
				{
					if (uniqueCharSetsIt->find(*charSetItRev) != string::npos)
					{
						break;
					}
				}
				else if (*uniqueCharSetsIt == *charSetItRev)
				{
					break;
				}
			}
			if (uniqueCharSetsIt == uniqueCharSets.end())
			{
				uniqueCharSets.push_back(charSetItRev->c_str());
				charSetLen += charSetItRev->length();
			}
		}
		if (keySpace == m_divShortCut[a])
		{
			m_numShortCutChars[a] = 0;
		}
		else if (keySpace / m_divShortCut[a] >= UINT64_C(0x100000000))
		{
			m_numShortCutChars[a] = -m_numShortCutChars[a];
		}
		keySpaceBoundary += keySpace;
		m_keySpaceBoundaries[a + 1] = keySpaceBoundary;
	}
	m_keySpace = keySpaceBoundary;
	m_maxPwLen = maxPwLen;

	m_charSet = new char[charSetLen+1];
	m_charSet[charSetLen] = 0;
	a = 0;
	for (uniqueCharSetsIt = uniqueCharSets.begin(); uniqueCharSetsIt != uniqueCharSets.end(); ++uniqueCharSetsIt)
	{
		charSetLen = uniqueCharSetsIt->length();
		memcpy(m_charSet + a, uniqueCharSetsIt->c_str(), charSetLen);
		a += charSetLen;
	}
	uniqueCharSets.clear();
	for (a = 0, subKeySpaceIt = subKeySpaces.begin(); subKeySpaceIt != subKeySpaces.end(); ++subKeySpaceIt, a++)
	{
		m_subKeySpaces[a] = new char*[subKeySpaceIt->size()];
		for (b = 0, charSetIt = subKeySpaceIt->begin(); charSetIt != subKeySpaceIt->end(); ++charSetIt, b++)
		{
			m_subKeySpaces[a][b] = strstr(m_charSet, charSetIt->c_str());
		}
	}
}

KeySpace::KeySpace(perCharCharSet *subKeySpaces, uint32 numSubKeySpaces)
{
	uint64 keySpace, keySpaceBoundary = 0;
	list<char*> uniqueCharSets;
	list<char*>::iterator it;
	uint32 a, b, charSetLen = 0, maxPwLen = 1;
	sint32 i, charSetDiffLen;

	m_numSubKeySpaces = numSubKeySpaces;
	m_keySpaceBoundaries = new uint64 [numSubKeySpaces + 1];
	m_passwordLengths    = new uint32 [numSubKeySpaces];
	m_divShortCut        = new uint32 [numSubKeySpaces];
	m_numShortCutChars   = new int    [numSubKeySpaces];
	m_charSetLengths     = new uint32*[numSubKeySpaces];
	m_subKeySpaces       = new char** [numSubKeySpaces];

	m_keySpaceBoundaries[0] = 0;
	for (a = 0; a < numSubKeySpaces; a++)
	{
		if (maxPwLen < subKeySpaces[a].pwLength)
		{
			maxPwLen = subKeySpaces[a].pwLength;
		}
		keySpace = 1;
		for (i = subKeySpaces[a].pwLength - 1; i >= 0; i--)
		{
			keySpace *= strlen(subKeySpaces[a].charSets[i]);
			if (keySpace < UINT64_C(0x100000000))
			{
				m_divShortCut[a] = (uint32) keySpace;
				m_numShortCutChars[a] = subKeySpaces[a].pwLength - i;
			}
		}
		if (keySpace == m_divShortCut[a])
		{
			m_numShortCutChars[a] = 0;
		}
		else if (keySpace / m_divShortCut[a] >= UINT64_C(0x100000000))
		{
			m_numShortCutChars[a] = -m_numShortCutChars[a];
		}
		keySpaceBoundary += keySpace;
		m_keySpaceBoundaries[a + 1] = keySpaceBoundary;

		m_passwordLengths[a] = subKeySpaces[a].pwLength;
		m_charSetLengths[a] = new uint32[subKeySpaces[a].pwLength];
		for (b = 0; b < subKeySpaces[a].pwLength; b++)
		{
			m_charSetLengths[a][b] = strlen(subKeySpaces[a].charSets[b]);
			for (it = uniqueCharSets.begin(); it != uniqueCharSets.end(); ++it)
			{
				charSetDiffLen = m_charSetLengths[a][b] - strlen(*it);
				if (charSetDiffLen > 0)
				{
					if (strstr(subKeySpaces[a].charSets[b], *it) != NULL)
					{
						*it = subKeySpaces[a].charSets[b];
						charSetLen += charSetDiffLen;
						break;
					}
				}
				else if (charSetDiffLen < 0)
				{
					if (strstr(*it, subKeySpaces[a].charSets[b]) != NULL)
					{
						break;
					}
				}
				else if (strcmp(*it, subKeySpaces[a].charSets[b]) == 0)
				{
					break;
				}
			}
			if (it == uniqueCharSets.end())
			{
				uniqueCharSets.push_back(subKeySpaces[a].charSets[b]);
				charSetLen += strlen(subKeySpaces[a].charSets[b]);
			}
		}
	}
	m_keySpace = keySpaceBoundary;
	m_maxPwLen = maxPwLen;

	m_charSet = new char[charSetLen+1];
	m_charSet[charSetLen] = 0;
	a = 0;
	for (it = uniqueCharSets.begin(); it != uniqueCharSets.end(); ++it)
	{
		charSetLen = strlen(*it);
		memcpy(m_charSet + a, *it, charSetLen);
		a += charSetLen;
	}
	uniqueCharSets.clear();
	for (a = 0; a < numSubKeySpaces; a++)
	{
		m_subKeySpaces[a] = new char*[subKeySpaces[a].pwLength];
		for (b = 0; b < subKeySpaces[a].pwLength; b++)
		{
			m_subKeySpaces[a][b] = strstr(m_charSet, subKeySpaces[a].charSets[b]);
		}
	}
}

KeySpace::~KeySpace()
{
	cleanUpKeySpace();
}

KeySpace& KeySpace::operator=(const KeySpace &ks)
{
	if (this != &ks)
	{
		cleanUpKeySpace();
		copyObject(ks);
	}
	return *this;
}

void KeySpace::cleanUpKeySpace()
{
	uint32 a/*, b*/;

	for (a = 0; a < m_numSubKeySpaces; a++)
	{
/*
		for (b = 0; b < a; b++)
		{
			if (m_charSetCache[a] == m_charSetCache[b])
			{
				break;
			}
		}
		if (a == b)
		{
			delete [] m_charSetCache[a];
		}
*/
		delete [] m_subKeySpaces[a];
		delete [] m_charSetLengths[a];
	}
	delete [] m_keySpaceBoundaries;
	delete [] m_passwordLengths;
	delete [] m_divShortCut;
	delete [] m_numShortCutChars;
//	delete [] m_charSetPoses;
	delete [] m_charSetLengths;
//	delete [] m_charSetCacheLengths;
//	delete [] m_charSetCache;
	delete [] m_charSet;
	delete [] m_subKeySpaces;
}

void KeySpace::copyObject(const KeySpace &ks)
{
	uint32 a, b, tmp, charSetLen = 1;

	m_keySpace = ks.m_keySpace;
	m_numSubKeySpaces = ks.m_numSubKeySpaces;
//	m_cachePos = ks.m_cachePos;
//	m_curSubKeySpace = ks.m_curSubKeySpace;
//	m_pw[0] = ks.m_pw[0];
//	m_pw[1] = ks.m_pw[1];
//	m_pw[2] = ks.m_pw[2];
//	m_pw[3] = ks.m_pw[3];
	m_maxPwLen = ks.m_maxPwLen;

	m_keySpaceBoundaries  = new uint64 [m_numSubKeySpaces + 1];
	m_passwordLengths     = new uint32 [m_numSubKeySpaces];
	m_divShortCut         = new uint32 [m_numSubKeySpaces];
	m_numShortCutChars    = new int    [m_numSubKeySpaces];
	m_charSetLengths      = new uint32*[m_numSubKeySpaces];
//	m_charSetCacheLengths = new uint32 [m_numSubKeySpaces];
//	m_charSetCache        = new uint16*[m_numSubKeySpaces];
	m_subKeySpaces        = new char** [m_numSubKeySpaces];
//	m_charSetPoses        = new uint32 [m_maxPwLen];
	memcpy(m_keySpaceBoundaries,  ks.m_keySpaceBoundaries,  sizeof(uint64) * (m_numSubKeySpaces + 1));
	memcpy(m_passwordLengths,     ks.m_passwordLengths,     sizeof(uint32) * m_numSubKeySpaces);
	memcpy(m_divShortCut,         ks.m_divShortCut,         sizeof(uint32) * m_numSubKeySpaces);
	memcpy(m_numShortCutChars,    ks.m_numShortCutChars,    sizeof(uint32) * m_numSubKeySpaces);
//	memcpy(m_charSetCacheLengths, ks.m_charSetCacheLengths, sizeof(uint32) * m_numSubKeySpaces);
//	memcpy(m_charSetPoses,        ks.m_charSetPoses,        sizeof(uint32) * m_maxPwLen);
	for (a = 0; a < m_numSubKeySpaces; a++)
	{
//		m_charSetCache[a]   = new uint16[m_charSetCacheLengths[a]];
		m_charSetLengths[a] = new uint32[m_passwordLengths[a]];
//		memcpy(m_charSetCache[a],   ks.m_charSetCache[a],   sizeof(uint16) * m_charSetCacheLengths[a]);
		memcpy(m_charSetLengths[a], ks.m_charSetLengths[a], sizeof(uint32) * m_passwordLengths[a]);
		for (b = 0; b < m_passwordLengths[a]; b++)
		{
			tmp = m_charSetLengths[a][b] + (ks.m_subKeySpaces[a][b] - ks.m_charSet);
			if (charSetLen < tmp)
			{
				charSetLen = tmp;
			}
		}
	}
	m_charSet = new char[charSetLen];
	memcpy(m_charSet, ks.m_charSet, sizeof(char) * charSetLen);
	for (a = 0; a < m_numSubKeySpaces; a++)
	{
		m_subKeySpaces[a] = new char*[m_passwordLengths[a]];
		for (b = 0; b < m_passwordLengths[a]; b++)
		{
			m_subKeySpaces[a][b] = m_charSet + (ks.m_subKeySpaces[a][b] - ks.m_charSet);
		}
	}
}
/*
bool KeySpace::reset(uint64 pwNum)
{
	uint32 *charSetPoses, *charSetLengths;
	char **subKeySpaces;
	uint32 a, curSubKeySpace = 0, passwordLength, numShortCutChars, charSetLength, divisor, remainder, quotient, pw[3] = {0, 0, 0};

	if (pwNum >= m_keySpace)
	{
		return true;
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
	subKeySpaces = m_subKeySpaces[curSubKeySpace];
	m_curSubKeySpace = curSubKeySpace;
	if (numShortCutChars > 0)
	{
		divisor = m_divShortCut[curSubKeySpace];
		DIV_MOD_64_RET_32_32(pwNum, divisor, remainder, quotient);
		charSetLength = m_charSetCacheLengths[curSubKeySpace];
		m_cachePos = remainder % charSetLength;
		remainder /= charSetLength;
		for (a = 2; a < numShortCutChars; a++)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = remainder % charSetLength;
			((char*)pw)[a] = subKeySpaces[a][remainder % charSetLength];
			remainder /= charSetLength;
		}
		for (; a < passwordLength; a++)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = quotient % charSetLength;
			((char*)pw)[a] = subKeySpaces[a][quotient % charSetLength];
			quotient /= charSetLength;
		}
	}
	else if (passwordLength > 1)
	{
		quotient = (uint32) pwNum;
		charSetLength = m_charSetCacheLengths[curSubKeySpace];
		m_cachePos = quotient % charSetLength;
		quotient /= charSetLength;
		for (a = 2; a < passwordLength; a++)
		{
			charSetLength = charSetLengths[a];
			charSetPoses[a] = quotient % charSetLength;
			((char*)pw)[a] = subKeySpaces[a][quotient % charSetLength];
			quotient /= charSetLength;
		}
	}
	else
	{
		charSetPoses[0] = (uint32) pwNum;
		((char*)pw)[0] = subKeySpaces[0][pwNum];
	}
	m_pw[0] = TO_LITTLE_ENDIAN(8 * passwordLength);
	if (passwordLength < 4)
	{
		m_pw[1] = pw[0] | TO_LITTLE_ENDIAN(0x80 << (passwordLength << 3));
		m_pw[2] = 0;
		m_pw[3] = 0;
	}
	else if (passwordLength < 8)
	{
		m_pw[1] = pw[0];
		m_pw[2] = pw[1] | TO_LITTLE_ENDIAN(0x80 << ((passwordLength - 4) << 3));
		m_pw[3] = 0;
	}
	else
	{
		m_pw[1] = pw[0];
		m_pw[2] = pw[1];
		m_pw[3] = pw[2] | TO_LITTLE_ENDIAN(0x80 << ((passwordLength - 8) << 3));
	}
	return false;
}

uint32 KeySpace::next(vector4 *pw, uint32 numPws, uint32 vecSize)
{
	uint16 *charSetCache;
	uint32 a, curSubKeySpace, passwordLength, charSetCacheLength, cachePos, pw0, pw1, pw2, pw3;

	cachePos = m_cachePos;
	curSubKeySpace = m_curSubKeySpace;
	passwordLength = m_passwordLengths[curSubKeySpace];
	charSetCacheLength = m_charSetCacheLengths[curSubKeySpace];
	charSetCache = m_charSetCache[curSubKeySpace];
	pw0 = m_pw[0];
	pw1 = m_pw[1];
	pw2 = m_pw[2];
	pw3 = m_pw[3];
	for (a = 0; a < numPws; a++)
	{
		pw[vecSize*0].data[a] = pw0;
		pw[vecSize*1].data[a] = pw1 | CACHE_TO_UINT32(charSetCache[cachePos]);
		pw[vecSize*2].data[a] = pw2;
		pw[vecSize*3].data[a] = pw3;
		cachePos++;
		if (cachePos >= charSetCacheLength)
		{
			if (cycleCache())
			{
				return a + 1;
			}
			pw0 = m_pw[0];
			pw1 = m_pw[1];
			pw2 = m_pw[2];
			pw3 = m_pw[3];
			cachePos = 0;
			curSubKeySpace = m_curSubKeySpace;
			passwordLength = m_passwordLengths[curSubKeySpace];
			charSetCacheLength = m_charSetCacheLengths[curSubKeySpace];
			charSetCache = m_charSetCache[curSubKeySpace];
		}
	}
	m_cachePos = cachePos;
	return a;
}

uint32 KeySpace::next(vector4 *pw, uint32 numPws, uint32 vecSize, uint32 *diffs)
{
	uint16 *charSetCache;
	uint32 a, curSubKeySpace, passwordLength, charSetCacheLength, cachePos, pw0, pw1, pw2, pw3;

	cachePos = m_cachePos;
	curSubKeySpace = m_curSubKeySpace;
	passwordLength = m_passwordLengths[curSubKeySpace];
	charSetCacheLength = m_charSetCacheLengths[curSubKeySpace];
	charSetCache = m_charSetCache[curSubKeySpace];
	pw0 = m_pw[0];
	pw1 = m_pw[1];
	pw2 = m_pw[2];
	pw3 = m_pw[3];
	for (a = 0; a < numPws; a++)
	{
		cachePos += diffs[a];
		if (cachePos >= charSetCacheLength)
		{
			if (cycleCache(cachePos))
			{
				return a;
			}
			pw0 = m_pw[0];
			pw1 = m_pw[1];
			pw2 = m_pw[2];
			pw3 = m_pw[3];
			cachePos = m_cachePos;
			curSubKeySpace = m_curSubKeySpace;
			passwordLength = m_passwordLengths[curSubKeySpace];
			charSetCacheLength = m_charSetCacheLengths[curSubKeySpace];
			charSetCache = m_charSetCache[curSubKeySpace];
		}
		pw[vecSize*0].data[a] = pw0;
		pw[vecSize*1].data[a] = pw1 | CACHE_TO_UINT32(charSetCache[cachePos]);
		pw[vecSize*2].data[a] = pw2;
		pw[vecSize*3].data[a] = pw3;
	}
	m_cachePos = cachePos;
	return a;
}

bool KeySpace::cycleCache(uint32 cachePos)
{
	uint32 *charSetPoses;
	uint32 a, tmp, tmp2, passwordLength, curSubKeySpace, cachePosCycles = 0, charSetCacheLength;

	curSubKeySpace = m_curSubKeySpace;
	charSetCacheLength = m_charSetCacheLengths[curSubKeySpace];
	passwordLength = m_passwordLengths[curSubKeySpace];
	if (passwordLength > 2)
	{
		cachePosCycles = cachePos / charSetCacheLength;
		cachePos %= charSetCacheLength;
		charSetPoses = m_charSetPoses;
		for (a = 2; a < passwordLength; a++)
		{
			tmp = (charSetPoses[a] += cachePosCycles);
			if (tmp < m_charSetLengths[curSubKeySpace][a])
			{
				((char*)(m_pw+1))[a] = m_subKeySpaces[curSubKeySpace][a][tmp];
				break;
			}
			tmp2 = m_charSetLengths[curSubKeySpace][a];
			cachePosCycles = tmp / tmp2;
			charSetPoses[a] = (tmp %= tmp2);
			((char*)(m_pw+1))[a] = m_subKeySpaces[curSubKeySpace][a][tmp];
		}
		if (a != passwordLength)
		{
			m_cachePos = cachePos;
			return false;
		}
		for (a = passwordLength - 1; a > 2; a--)
		{
			cachePosCycles = (cachePosCycles - 1) * m_charSetLengths[curSubKeySpace][a] + charSetPoses[a];
		}
		cachePos = cachePosCycles * charSetCacheLength + cachePos;
	}
	else
	{
		cachePos -= charSetCacheLength;
	}
	if (++curSubKeySpace >= m_numSubKeySpaces)
	{
		return true;
	}
	return reset(m_keySpaceBoundaries[curSubKeySpace] + cachePos);
}

bool KeySpace::cycleCache()
{
	uint32 *charSetPoses;
	uint32 a, passwordLength, curSubKeySpace;
	bool nextKeySpace = true;

	//m_cachePos = 0;
	curSubKeySpace = m_curSubKeySpace;
	passwordLength = m_passwordLengths[curSubKeySpace];
	if (passwordLength > 2)
	{
		charSetPoses = m_charSetPoses;
		for (a = 2; a < passwordLength; a++)
		{
			if (++charSetPoses[a] < m_charSetLengths[curSubKeySpace][a])
			{
				((char*)(m_pw+1))[a] = m_subKeySpaces[curSubKeySpace][a][charSetPoses[a]];
				break;
			}
			charSetPoses[a] = 0;
			((char*)(m_pw+1))[a] = m_subKeySpaces[curSubKeySpace][a][0];
		}
		nextKeySpace = a == passwordLength;
	}
	if (nextKeySpace)
	{
		if (++curSubKeySpace >= m_numSubKeySpaces)
		{
			return true;
		}
		m_curSubKeySpace = curSubKeySpace;
		passwordLength = m_passwordLengths[curSubKeySpace];
		charSetPoses = m_charSetPoses;
		m_pw[0] = TO_LITTLE_ENDIAN(8 * passwordLength);
		m_pw[1] = 0;
		m_pw[2] = 0;
		m_pw[3] = 0;
		for (a = 0; a < passwordLength; a++)
		{
			charSetPoses[a] = 0;
			((char*)(m_pw+1))[a] = m_subKeySpaces[curSubKeySpace][a][0];
		}
		((char*)(m_pw+1))[a] = (char) 0x80;
	}
	return false;
}
*/
uint64 KeySpace::getKeySpace()
{
	return m_keySpace;
}

uint32 KeySpace::getMaxPwLen()
{
	return m_maxPwLen;
}
