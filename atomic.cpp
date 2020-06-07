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

#include "atomic.h"

Atomic::Atomic(uint64 value)
{
	m_value = value;
	MUTEX_CREATE(m_mutex);
}

Atomic::~Atomic()
{
	MUTEX_DELETE(m_mutex);
}

uint64 Atomic::add(uint32 inc)
{
	uint64 ret;

	MUTEX_LOCK(m_mutex);
	ret = m_value;
	m_value += inc;
	if (m_value < ret) // overflow
	{
		m_value = MAX_UINT64;
	}
	MUTEX_UNLOCK(m_mutex);
	return ret;
}

uint64 Atomic::add(uint64 inc)
{
	uint64 ret;

	MUTEX_LOCK(m_mutex);
	ret = m_value;
	m_value += inc;
	if (m_value < ret) // overflow
	{
		m_value = MAX_UINT64;
	}
	MUTEX_UNLOCK(m_mutex);
	return ret;
}

const uint64 Atomic::get()
{
	return m_value;
}
