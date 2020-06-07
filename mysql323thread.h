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

#ifndef MYSQL323_THREAD_H
#define MYSQL323_THREAD_H

#include "thread.h"
#include "workqueue.h"
#include "keyspace.h"
#include "atomic.h"

struct mySQL323ThreadArgs
{
	WorkQueue *inQueue, *outQueue;
	const KeySpace *keySpace;
	Atomic *keySpacePos;
	uint32 threadId;
};

struct mySQL323WorkQueueData
{
	double time;
	char *foundPw;
	uint32 hash1, hash2, count, threadId;
};

struct mySQL323WorkQueueNode
{
	mySQL323WorkQueueNode *next;
	mySQL323WorkQueueData *data;
};

class MySQL323Thread : public Thread
{
public:
	MySQL323Thread();
	~MySQL323Thread();

	int stopRunning();
	void *debugrun(void *arg);

protected:
	void *threadFunc(void *arg);

private:
	WorkQueue *m_queue;
	MUTEX m_mutex;
};

#endif
