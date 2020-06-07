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

#include <stdio.h>
#include <signal.h>
#include "keyspace.h"
#include "mysql323thread.h"
#include "atomic.h"

const char *speedPrefixes;
const char SI_PREFIXES[]    = " kMGTPEZY"; // kilo, mega, giga, tera, peta, exa, zetta, yotta
const char OTHER_PREFIXES[] = " kMBTQ";    // thousand, million, billion, trillion, quadrillion

void catchCtrlC(int sign);
int crackMySQL323Length1_2(uint32 hash1, uint32 hash2, char *foundPw);
void formatSpeed(double speed);
int readHash(const char *hash, uint32 &hash1, uint32 &hash2);
char *str2Hex(char *hex, const char *str);

bool stopCracking = false;

int main(int argc, char *argv[])
{
	uint64 totalPasswords, skipPasswords = 0;
	double speed, totalTime;
	MySQL323Thread *threads;
	mySQL323WorkQueueNode *nodes, *node;
	mySQL323WorkQueueData *data;
	mySQL323ThreadArgs *args;
	KeySpace *keySpace;
	double *times, *passwords, *speeds;
	char *hexStr, *hash;
	Atomic keySpacePos(0);
	WorkQueue toMySQL323, fromMySQL323;
	TIMER_TYPE t1, t2, startTime, endTime;
	uint32 a, hash1, hash2, hash3, numThreads;
	char foundPw[3];

	// Catch Ctrl+C
	signal(SIGINT, &catchCtrlC);

	if (argc != 4 && argc != 5)
	{
		printf("Wrong number of arguments\n"
			"This takes three or four arguments:\n"
			" * number of threads\n"
			" * hash\n"
			" * key space file\n"
			" * resume code (optional)\n"
#ifdef _WIN32
	#ifdef ARC_64
			"mysql323 ming64.exe 4 7fffffff7fffffff keyspace.txt 1234567890\n");
	#else
			"mysql323 32.exe 4 7fffffff7fffffff keyspace.txt 1234567890\n");
	#endif
#else
	#ifdef ARC_64
			"./mysql323-64 4 7fffffff7fffffff keyspace.txt 1234567890\n");
	#else
			"./mysql323-32 4 7fffffff7fffffff keyspace.txt 1234567890\n");
	#endif
#endif
		return 1;
	}

	// Number of threads
	sscanf(argv[1], "%u", &numThreads);

	// Resume
	if (argc == 5)
	{
		sscanf(argv[4], pfuint64, &skipPasswords);
		keySpacePos.add(skipPasswords);
	}

	// Init key space
	keySpace = new KeySpace(argv[3]);
	hexStr = new char [2 * keySpace->getMaxPwLen() + 5];

	// Read hash
	hash = argv[2];
	if (readHash(hash, hash1, hash2) != 0)
	{
		fprintf(stderr, "ERROR: Not a valid hash\n");
		return 1;
	}

	// Run MySQL323 len 0
	if (hash1 == 0x50305735 && hash2 == 0x12345671)
	{
		printf("%s:<empty pw>:<empty pw>\n", hash);
	}

	// Run MySQL323 len 1 and 2
	if (crackMySQL323Length1_2(hash1, hash2, foundPw))
	{
		printf("%s:%s:%s\n", hash, str2Hex(hexStr, foundPw), foundPw);
		return 0;
	}

	// Process hash
	hash3 = hash2 - hash1;
	hash3 = hash2 - ((hash3 << 8) ^ hash1);
	hash3 = hash2 - ((hash3 << 8) ^ hash1);
	hash2 = hash2 - ((hash3 << 8) ^ hash1);

	// Init threads
	speedPrefixes = SI_PREFIXES;
	totalPasswords = keySpace->getKeySpace();
	times = new double[numThreads];
	passwords = new double[numThreads];
	speeds = new double[numThreads];
	threads = new MySQL323Thread[numThreads];
	nodes = new mySQL323WorkQueueNode[numThreads * 2];
	data = new mySQL323WorkQueueData[numThreads * 2];
	args = new mySQL323ThreadArgs[numThreads];
	for (a = 0; a < numThreads; a++)
	{
		times[a] = 0;
		passwords[a] = 0;
		speeds[a] = 0;
		args[a].inQueue     = &toMySQL323;
		args[a].outQueue    = &fromMySQL323;
		args[a].keySpacePos = &keySpacePos;
		args[a].keySpace    = keySpace;
		args[a].threadId    = a;
		threads[a].start(args + a);
	}

	printf("\rCalculating speed...");
	fflush(stdout);
	TIMER_FUNC(t1);
	TIMER_FUNC(startTime);

	// Initial work
	for (a = 0; a < 2 * numThreads; a++)
	{
		nodes[a].data = data + a;
		data[a].hash1 = hash1;
		data[a].hash2 = hash2;
		data[a].foundPw = new char[keySpace->getMaxPwLen() + 3];
		data[a].foundPw[0] = 0;
		toMySQL323.push((workQueueNode*) (nodes + a));
	}
	for (; a < 2 * numThreads; a++)
	{
		data[a].foundPw = NULL;
	}

	// Process completed work
	node = (mySQL323WorkQueueNode*) fromMySQL323.pop();
	while (node->data->count != 0 && keySpacePos.get() < totalPasswords)
	{
		a = node->data->threadId;
		times[a]     += node->data->time;
		passwords[a] += node->data->count;

		// Update speed
		TIMER_FUNC(t2);
		if (TIMER_DIFF(t1, t2) >= 5)
		{
			speed = 0;
			for (a = 0; a < numThreads; a++)
			{
				if (times[a] != 0)
				{
					speeds[a] = 94 * 94 * passwords[a] / times[a];
					speed += speeds[a];
				}
				times[a] = 0;
				passwords[a] = 0;
			}
			printf("\r");
			formatSpeed(speed);
			printf(" [");
			for (a = 0; a < numThreads; a++)
			{
				if (a != 0)
				{
					printf(" ");
				}
				printf("%0.1f%%", speeds[a] / speed * 100);
			}
			printf("] ");
			fflush(stdout);
			t1 = t2;
		}

		if (node->data->foundPw[0] != 0)
		{
			printf("\n%s:%s:%s\n", hash, str2Hex(hexStr, node->data->foundPw), node->data->foundPw);
			break;
		}
		if (stopCracking)
		{
			break;
		}

		// 
//		node->data->hash1 = hash1;
//		node->data->hash2 = hash2;
		toMySQL323.push((workQueueNode*) node);
		node = (mySQL323WorkQueueNode*) fromMySQL323.pop();
	}
	for (a = 0; a < numThreads; a++)
	{
		threads[a].stopRunning();
		threads[a].waitForStopRunning();
	}

	// Stats
	TIMER_FUNC(endTime);
	if (totalPasswords > keySpacePos.get())
	{
		totalPasswords = keySpacePos.get();
	}
	totalPasswords -= skipPasswords;
	totalTime = TIMER_DIFF(startTime, endTime);
	printf("\nTotal time:    %0.3f seconds\n"
	         "Average speed: ", totalTime);
	if (totalTime != 0)
	{
		formatSpeed(94 * 94 * (totalPasswords / totalTime));
	}
	else
	{
		printf("n/a");
	}
	printf("\n");

	// Resume code
	if (stopCracking)
	{
		printf("Resume code:   " pfuint64 "\n", keySpacePos.get());
	}

	// Clean up
	for (a = 0; a < 2 * numThreads; a++)
	{
		if (data[a].foundPw != NULL)
		{
			delete [] data[a].foundPw;
		}
	}
	delete keySpace;
	delete [] hexStr;
	delete [] times;
	delete [] passwords;
	delete [] speeds;
	delete [] threads;
	delete [] nodes;
	delete [] data;
	delete [] args;
	return 0;
}

void catchCtrlC(int sign)
{
	stopCracking = true;
}

int crackMySQL323Length1_2(uint32 hash1, uint32 hash2, char *foundPw)
{
	uint32 ch1, ch2, tmp;
	uint32 h1, h2, add;
	uint32 h21, h22;

	for (ch1 = 33, add = 33 + 7, tmp = 60 * 33 + 0x30573500; ch1 <= 126; ch1++, add++, tmp += 60)
	{
		h1 = 0x50305735 ^ tmp;
		h2 = 0x12345671 + (0x34567100 ^ h1);
		if ((h1 & 0x7fffffff) == hash1 && (h2 & 0x7fffffff) == hash2)
		{
			foundPw[0] = ch1;
			foundPw[1] = 0;
			return 1;
		}
		for (ch2 = 33; ch2 <= 126; ch2++)
		{
			h21 = h1 ^ (((h1 & 63) + add) * ch2 + (h1 << 8));
			h22 = h2 + ((h2 << 8) ^ h21);
			if ((h21 & 0x7fffffff) == hash1 && (h22 & 0x7fffffff) == hash2)
			{
				foundPw[0] = ch1;
				foundPw[1] = ch2;
				foundPw[2] = 0;
				return 1;
			}
		}
	}
	return 0;
}

void formatSpeed(double speed)
{
	uint32 a;
	char prefix;

	for (a = 0; speedPrefixes[a + 1] != 0; a++)
	{
		if (speed < 1000)
		{
			break;
		}
		speed /= 1000;
	}
	prefix = speedPrefixes[a];
	if (speed < 10)
	{
		printf("%0.3f %cp/s", speed, prefix);
	}
	else if (speed < 100)
	{
		printf("%0.2f %cp/s", speed, prefix);
	}
	else if (speed < 1000)
	{
		printf("%0.1f %cp/s", speed, prefix);
	}
	else
	{
		printf(">999 %cp/s", prefix);
	}
}

int readHash(const char *hash, uint32 &hash1, uint32 &hash2)
{
	uint32 a, h1 = 0, h2 = 0;

	hash1 = 0;
	hash2 = 0;
	for (a = 0; a < 8; a++)
	{
		if (hash[a] >= '0' && hash[a] <= '9')
		{
			h1 <<= 4;
			h1 |= hash[a] - '0';
		}
		else if (hash[a] >= 'A' && hash[a] <= 'F')
		{
			h1 <<= 4;
			h1 |= hash[a] - 'A' + 10;
		}
		else if (hash[a] >= 'a' && hash[a] <= 'f')
		{
			h1 <<= 4;
			h1 |= hash[a] - 'a' + 10;
		}
		else
		{
			return 1;
		}
	}
	for (; a < 16; a++)
	{
		if (hash[a] >= '0' && hash[a] <= '9')
		{
			h2 <<= 4;
			h2 |= hash[a] - '0';
		}
		else if (hash[a] >= 'A' && hash[a] <= 'F')
		{
			h2 <<= 4;
			h2 |= hash[a] - 'A' + 10;
		}
		else if (hash[a] >= 'a' && hash[a] <= 'f')
		{
			h2 <<= 4;
			h2 |= hash[a] - 'a' + 10;
		}
		else
		{
			return 1;
		}
	}
	if (hash[16] != 0 || h1 > 0x7fffffff || h2 > 0x7fffffff)
	{
		return 1;
	}
	hash1 = h1;
	hash2 = h2;
	return 0;
}

char *str2Hex(char *hex, const char *str)
{
	uint32 a;
	static const char HEX[] = "0123456789abcdef";

	for (a = 0; *str; a += 2, str++)
	{
		hex[a]     = HEX[((unsigned char) *str) >> 4];
		hex[a + 1] = HEX[((unsigned char) *str) & 0xf];
	}
	hex[a] = 0;
	return hex;
}
