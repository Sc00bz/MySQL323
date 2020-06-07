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

#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include "common.h"
#include "thread.h"
#include <stdio.h>

struct workQueueNode
{
	workQueueNode *next;
	void          *data;
};

class WorkQueue
{
public:
	WorkQueue();
	~WorkQueue();

	workQueueNode *pop();
	workQueueNode *popAll();
	void push(workQueueNode *n);
	void pushQueue(workQueueNode *h, workQueueNode *t);
	void flush();
	void stopFlush();
	void waitForStopFlush();

private:
	WorkQueue(WorkQueue &q) {}
	WorkQueue &operator=(WorkQueue &q) {return *this;}

	workQueueNode *m_head, *m_tail;
	MUTEX m_mutex;
	COND  m_cond, m_cond2;
	uint32 m_cleanUp, m_flushing, m_numWaiting;
};

#endif
