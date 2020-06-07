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

#ifndef ATOMIC_H
#define ATOMIC_H

#include "common.h"
#include "thread.h"

class Atomic
{
public:
	Atomic(uint64 value);
	~Atomic();

	uint64 add(uint32 inc);
	uint64 add(uint64 inc);
	const uint64 get();

private:
	Atomic(Atomic &a) {}
	Atomic &operator=(Atomic &a) {return *this;}

	uint64 m_value;
	MUTEX m_mutex;
};

#endif
