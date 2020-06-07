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

#include "thread.h"
#include <stdio.h>

Thread::Thread()
{
	m_started = 0;
}

void Thread::waitForStopRunning()
{
	THREAD_WAIT(m_thread);
}

int Thread::start(void *arg)
{
	int ret = -1;

	if (m_started == 0)
	{
		m_started = 1;
		m_arg = arg;
		ret = THREAD_CREATE(m_thread, Thread::callThreadFunc, this);
		if (ret)
		{
			fprintf(stderr, "Thread creation failed: %d\n", ret);
		}
	}
	return ret;
}

void *Thread::callThreadFunc(void *arg)
{
	return ((Thread*)arg)->threadFunc(((Thread*)arg)->m_arg);
}
