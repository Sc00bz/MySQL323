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

#ifndef THREAD_H
#define THREAD_H

#ifdef _WIN32
	#include <windows.h>
	#define THREAD                          HANDLE
	#define THREAD_WAIT(thread)             WaitForSingleObject(thread, INFINITE)
	#define THREAD_CREATE(thread,func,arg)  ((thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)func, arg, 0, NULL)) == NULL ? -1 : 0)

	typedef CRITICAL_SECTION    MUTEX;
	typedef CRITICAL_SECTION  *PMUTEX;

	#define MUTEX_CREATE(mutex)             InitializeCriticalSection(&mutex)
	#define MUTEX_DELETE(mutex)             DeleteCriticalSection(&mutex)
	#define MUTEX_LOCK(mutex)               EnterCriticalSection(&mutex)
	#define MUTEX_UNLOCK(mutex)             LeaveCriticalSection(&mutex)

	#define PMUTEX_CREATE(pmutex)           InitializeCriticalSection(pmutex = new CRITICAL_SECTION)
	#define PMUTEX_DELETE(pmutex)           DeleteCriticalSection(pmutex); delete pmutex
	#define PMUTEX_LOCK(pmutex)             EnterCriticalSection(pmutex)
	#define PMUTEX_UNLOCK(pmutex)           LeaveCriticalSection(pmutex)
	#if 0 // (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		typedef CONDITION_VARIABLE    COND;
		typedef CONDITION_VARIABLE  *PCOND;

		#define COND_CREATE(cond)               InitializeConditionVariable(&cond)
		#define COND_DELETE(cond)               /* Memory leak? */
		#define COND_SIGNAL(cond)               WakeConditionVariable(&cond)
		#define COND_SIGNAL_ALL(cond)           WakeAllConditionVariable(&cond)
		#define COND_WAIT(cond,mutex)           SleepConditionVariableCS(&cond, &mutex, INFINITE)

		#define PCOND_CREATE(pcond)             InitializeConditionVariable(pcond = new CONDITION_VARIABLE)
		#define PCOND_DELETE(pcond)             /* Memory leak? */delete pcond
		#define PCOND_SIGNAL(pcond)             WakeConditionVariable(pcond)
		#define PCOND_SIGNAL_ALL(pcond)         WakeAllConditionVariable(pcond)
		#define PCOND_WAIT(pcond,pmutex)        SleepConditionVariableCS(pcond, pmutex, INFINITE)
	#else
		typedef HANDLE   COND;
		typedef HANDLE  PCOND;

		#define COND_CREATE(cond)               (cond = CreateEvent(NULL, TRUE, FALSE, NULL))
		#define COND_DELETE(cond)               CloseHandle(cond)
		#define COND_SIGNAL(cond)               SetEvent(cond)
		#define COND_SIGNAL_ALL(cond)           SetEvent(cond)
		#define COND_WAIT(cond,mutex)           ResetEvent(cond); \
		                                        MUTEX_UNLOCK(mutex); \
		                                        WaitForSingleObject(cond, 1000 /* 1 second because of race condition */); \
		                                        MUTEX_LOCK(mutex)

		#define PCOND_CREATE(pcond)             (pcond = CreateEvent(NULL, TRUE, FALSE, NULL))
		#define PCOND_DELETE(pcond)             CloseHandle(pcond)
		#define PCOND_SIGNAL(pcond)             SetEvent(pcond)
		#define PCOND_SIGNAL_ALL(pcond)         SetEvent(pcond)
		#define PCOND_WAIT(pcond,pmutex)        ResetEvent(pcond); \
		                                        PMUTEX_UNLOCK(pmutex); \
		                                        WaitForSingleObject(pcond, 1000 /* 1 second because of race condition */); \
		                                        PMUTEX_LOCK(pmutex)
	#endif
#else
	#include <pthread.h>
	#define THREAD                          pthread_t
	#define THREAD_WAIT(thread)             pthread_join(thread, NULL);
	#define THREAD_CREATE(thread,func,arg)  pthread_create(&thread, NULL, func, arg)

	typedef pthread_mutex_t    MUTEX;
	typedef pthread_mutex_t  *PMUTEX;
	typedef pthread_cond_t     COND;
	typedef pthread_cond_t   *PCOND;

	#define MUTEX_CREATE(mutex)             pthread_mutex_init(&mutex, NULL)
	#define MUTEX_DELETE(mutex)             pthread_mutex_destroy(&mutex)
	#define MUTEX_LOCK(mutex)               pthread_mutex_lock(&mutex)
	#define MUTEX_UNLOCK(mutex)             pthread_mutex_unlock(&mutex)

	#define PMUTEX_CREATE(pmutex)           pthread_mutex_init(pmutex = new pthread_mutex_t, NULL)
	#define PMUTEX_DELETE(pmutex)           pthread_mutex_destroy(pmutex); delete pmutex
	#define PMUTEX_LOCK(pmutex)             pthread_mutex_lock(pmutex)
	#define PMUTEX_UNLOCK(pmutex)           pthread_mutex_unlock(pmutex)

	#define COND_CREATE(cond)               pthread_cond_init(&cond, NULL)
	#define COND_DELETE(cond)               pthread_cond_destroy(&cond)
	#define COND_SIGNAL(cond)               pthread_cond_signal(&cond)
	#define COND_SIGNAL_ALL(cond)           pthread_cond_broadcast(&cond)
	#define COND_WAIT(cond,mutex)           pthread_cond_wait(&cond, &mutex)

	#define PCOND_CREATE(pcond)             pthread_cond_init(pcond = new pthread_cond_t, NULL)
	#define PCOND_DELETE(pcond)             pthread_cond_destroy(pcond); delete pcond
	#define PCOND_SIGNAL(pcond)             pthread_cond_signal(pcond)
	#define PCOND_SIGNAL_ALL(pcond)         pthread_cond_broadcast(pcond)
	#define PCOND_WAIT(pcond,pmutex)        pthread_cond_wait(pcond, pmutex)
#endif

class Thread
{
public:
	Thread();
	virtual ~Thread() {}
	virtual int stopRunning() = 0;
	void waitForStopRunning();
	int start(void *arg);

protected:
	virtual void *threadFunc(void *arg) = 0;

private:
	Thread(Thread &t) {}
	Thread &operator=(Thread &t) {return *this;}

	static void *callThreadFunc(void *arg);

	void *m_arg;
	THREAD m_thread;
	int m_started;
};

#endif
