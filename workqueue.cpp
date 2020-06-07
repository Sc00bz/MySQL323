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

#include "workqueue.h"

WorkQueue::WorkQueue()
{
	m_head = NULL;
	m_tail = NULL;
	m_flushing = 0;
	m_numWaiting = 0;
	MUTEX_CREATE(m_mutex);
	COND_CREATE(m_cond);
	COND_CREATE(m_cond2);
}

WorkQueue::~WorkQueue()
{
	MUTEX_DELETE(m_mutex);
	COND_DELETE(m_cond);
	COND_DELETE(m_cond2);
}

workQueueNode *WorkQueue::pop()
{
	workQueueNode *ret = NULL;

	MUTEX_LOCK(m_mutex);
	while (m_head == NULL && m_flushing == 0)
	{
		COND_WAIT(m_cond, m_mutex);
	}
	if (m_head != NULL)
	{
		ret = m_head;
		m_head = m_head->next;
		ret->next = NULL;
	}
	MUTEX_UNLOCK(m_mutex);
	return ret;
}

workQueueNode *WorkQueue::popAll()
{
	workQueueNode *ret;

	MUTEX_LOCK(m_mutex);
	while (m_head == NULL && m_flushing == 0)
	{
		COND_WAIT(m_cond, m_mutex);
	}
	ret = m_head;
	m_head = NULL;
	m_tail = NULL;
	MUTEX_UNLOCK(m_mutex);
	return ret;
}

void WorkQueue::push(workQueueNode *n)
{
	MUTEX_LOCK(m_mutex);
	n->next = NULL;
	if (m_head == NULL)
	{
		COND_SIGNAL_ALL(m_cond);
		m_head = n;
	}
	else
	{
		m_tail->next = n;
	}
	m_tail = n;
	MUTEX_UNLOCK(m_mutex);
}

void WorkQueue::pushQueue(workQueueNode *h, workQueueNode *t)
{
	MUTEX_LOCK(m_mutex);
	if (m_head == NULL)
	{
		COND_SIGNAL_ALL(m_cond);
		m_head = h;
	}
	else
	{
		m_tail->next = h;
	}
	m_tail = t;
	m_tail->next = NULL;
	MUTEX_UNLOCK(m_mutex);
}

void WorkQueue::flush()
{
	MUTEX_LOCK(m_mutex);
	m_flushing = 1;
	COND_SIGNAL_ALL(m_cond);
	MUTEX_UNLOCK(m_mutex);
}

void WorkQueue::stopFlush()
{
	MUTEX_LOCK(m_mutex);
	m_flushing = 0;
	COND_SIGNAL_ALL(m_cond2);
	MUTEX_UNLOCK(m_mutex);
}

void WorkQueue::waitForStopFlush()
{
	MUTEX_LOCK(m_mutex);
	m_numWaiting++;
	while (m_flushing != 0)
	{
		COND_WAIT(m_cond2, m_mutex);
	}
	m_numWaiting--;
	MUTEX_UNLOCK(m_mutex);
}
