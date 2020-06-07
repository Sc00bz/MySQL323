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

#include "mysql323thread.h"
#include "mysql323keyspace.h"
#include <stdio.h>

MySQL323Thread::MySQL323Thread()
{
	m_queue = NULL;
	MUTEX_CREATE(m_mutex);
}

MySQL323Thread::~MySQL323Thread()
{
	MUTEX_DELETE(m_mutex);
}

int MySQL323Thread::stopRunning()
{
	MUTEX_LOCK(m_mutex);
	if (m_queue == NULL)
	{
		MUTEX_UNLOCK(m_mutex);
		return 1;
	}
	m_queue->flush();
	MUTEX_UNLOCK(m_mutex);
	return 0;
}

void *MySQL323Thread::debugrun(void *arg)
{
	return threadFunc(arg);
}

void *MySQL323Thread::threadFunc(void *arg)
{
	uint64 start;
	double time;
	WorkQueue *inQueue, *outQueue;
	mySQL323WorkQueueNode *node;
	Atomic *keySpacePos;
	char *charSet;
	TIMER_TYPE t1, t2;
	uint32 startCount = 5000000, count, countDone, hash1, hash2, pwPrefixLen, firstNewChar, charSetLen, threadId, found = 0;
	uint32 state1_ary[MAX_LEN - 2], state2_ary[MAX_LEN - 2];
	uint32 xor_ary[MAX_LEN - 3], step_ary[MAX_LEN - 3];
	int ret;
	char pwPrefix[MAX_LEN + 1];
	
	state1_ary[0] = 0x50305735;
	state2_ary[0] = 0x12345671;
	inQueue = ((mySQL323ThreadArgs*)arg)->inQueue;
	outQueue = ((mySQL323ThreadArgs*)arg)->outQueue;
	keySpacePos = ((mySQL323ThreadArgs*)arg)->keySpacePos;
	MySQL323KeySpace keySpace = *(((mySQL323ThreadArgs*)arg)->keySpace);
	threadId = ((mySQL323ThreadArgs*)arg)->threadId;
	if (keySpace.getMaxPwLen() + 2 > MAX_LEN)
	{
		fprintf(stderr, "ERROR: **** Key space max password length (%u) > MAX_LEN (%u) ****\n", keySpace.getMaxPwLen() + 2, MAX_LEN);
		return NULL;
	}
	MUTEX_LOCK(m_mutex);
	m_queue = inQueue;
	MUTEX_UNLOCK(m_mutex);

	TIMER_FUNC(t1);
	node = (mySQL323WorkQueueNode*) inQueue->pop();
	while (node != NULL)
	{
		count = startCount;
		start = keySpacePos->add(count);
		hash1 = node->data->hash1;
		hash2 = node->data->hash2;
		pwPrefix[0] = 0;
		ret = keySpace.reset(start);
		if (ret == -1)
		{
			node->data->count = 0;
			node->data->time = 0;
			node->data->threadId = threadId;
			outQueue->push((workQueueNode*) node);
			break;
		}
		count -= (uint32) ret;
		countDone = 0;

		uint32 i, ci, c, d, e, sum, step, diff, div, xor1, xor2, state1, state2;
		uint32 newstate1, newstate2, newstate3;
		do
		{
			if (keySpace.next(pwPrefix, pwPrefixLen, firstNewChar, &charSet, charSetLen))
			{
				break;
			}
//			char tmp;
//			pwPrefix[pwPrefixLen] = 0;
//			tmp = charSet[charSetLen];
//			charSet[charSetLen] = 0;
//			printf("'%s', %u, %u, '%s', %u\n", pwPrefix, pwPrefixLen, firstNewChar, charSet, charSetLen);
//			charSet[charSetLen] = tmp;

			// Run MySQL323
			sum = 7;
			for (i = 0; i < firstNewChar; i++)
			{
				sum += pwPrefix[i];
			}
			for (; i < pwPrefixLen; i++)
			{
				step_ary[i] = (state1_ary[i] & 0x3f) + sum;
				xor_ary[i]  = step_ary[i] * pwPrefix[i] + (state1_ary[i] << 8);
				sum += pwPrefix[i];
				state1_ary[i+1] = state1_ary[i] ^ xor_ary[i];
				state2_ary[i+1] = state2_ary[i] + ((state2_ary[i] << 8) ^ state1_ary[i+1]);
			}

			ci = 0;
			c = charSet[0] - 1;
			state1 = state1_ary[i];
			state2 = state2_ary[i];
			step = (state1 & 0x3f) + sum;
			xor1 = step * c + (state1 << 8);
			xor2 = (state2 << 8) ^ state1;

			// This takes advantage of the brain dead branch prediction:
			// * if address > EIP/RIP then assume not taken
			// * if address < EIP/RIP then assume taken
			while (1)
			{
				// Increment password
				d = charSet[ci] - c;
				c += d;
				xor1 += step;
				if (d != 1)
				{
					xor1 += (d - 1) * step;
				}

				newstate2 = state2 + (xor1 ^ xor2);
				newstate1 = state1 ^ xor1;
				newstate3 = (hash2 - newstate2) ^ (newstate2 << 8);

				// Predict second to last character
				div  = (newstate1 & 0x3f) + sum + c;
				diff = ((newstate3 ^ newstate1) - (newstate1 << 8)) & 0x7fffffff;
				if ((diff >> 7) > div) // ~99.997% fail rate
				{
					// gets here MORE than 50% of the time and branch prediction is fall through :)
					ci++;
					if (ci < charSetLen)
						continue;
					break;
				}
				if ((diff >> 5) >= div) // 25% fail rate (32 / 128)
				{
					// gets here MORE than 50% of the time and branch prediction is fall through :)
					if (diff % div != 0) // ~99.7% fail rate
					{
						// gets here MORE than 50% of the time and branch prediction is fall through :)
						ci++;
						if (ci < charSetLen)
							continue;
						break;
					}
					d = diff / div; // Second to last character

					// Predict last character
					div  = (newstate3 & 0x3f) + sum + c + d;
					diff = ((hash1 ^ newstate3) - (newstate3 << 8)) & 0x7fffffff;
					if ((diff >> 7) > div) // ~99.997% fail rate
					{
						// gets here MORE than 50% of the time and branch prediction is fall through :)
						ci++;
						if (ci < charSetLen)
							continue;
						break;
					}
					if ((diff >> 5) >= div) // 25% fail rate (32 / 128)
					{
						// gets here MORE than 50% of the time and branch prediction is fall through :)
						if (diff % div != 0) // ~99.7% fail rate
						{
							// gets here MORE than 50% of the time and branch prediction is fall through :)
							ci++;
							if (ci < charSetLen)
								continue;
							break;
						}
						e = diff / div; // Last character
						if (d >= 33 && d <= 126 && e >= 33 && e <= 126) // 4.123% fail rate (1 - 94 / 96 * 94 / 96)
						{
							// gets here MORE than 50% of the time and branch prediction is fall through :)
							memcpy(node->data->foundPw, pwPrefix, pwPrefixLen);
							node->data->foundPw[pwPrefixLen  ] = c;
							node->data->foundPw[pwPrefixLen+1] = d;
							node->data->foundPw[pwPrefixLen+2] = e;
							node->data->foundPw[pwPrefixLen+3] = 0;
							found = 1;
							countDone++;
							break;
						}
					}
				}
				ci++;
				if (ci >= charSetLen)
				{
					break;
				}
			}
			countDone += ci;
			if (found)
			{
				break;
			}
		} while (countDone < count);
		node->data->threadId = threadId;
		node->data->count = countDone;
		TIMER_FUNC(t2);
		time = TIMER_DIFF(t1, t2);
		node->data->time = time;
		t1 = t2;

		// Adjust startCount
		if (time < 0.080 || time > 0.125)
		{
			if (time <= 0.015)
			{
				startCount += startCount;
			}
			else
			{
				startCount = (uint32) (startCount * 0.1 / time);
			}
			if (startCount < 5000000)
			{
				startCount = 5000000;
			}
		}

		outQueue->push((workQueueNode*) node);
		if (countDone < count)
		{
			break;
		}
		node = (mySQL323WorkQueueNode*) inQueue->pop();
	}
	MUTEX_LOCK(m_mutex);
	m_queue = NULL;
	MUTEX_UNLOCK(m_mutex);
	return NULL;
}
