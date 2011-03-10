/* The MIT License:

Copyright (c) 2008-2011 Ivan Gagis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// ting 0.6.0
// Homepage: http://code.google.com/p/ting

/**
 * @file Timer.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Timer library.
 */

#pragma once

//#define M_ENABLE_TIMER_TRACE
#ifdef M_ENABLE_TIMER_TRACE
#define M_TIMER_TRACE(x) TRACE(<<"[Timer]" x)
#else
#define M_TIMER_TRACE(x)
#endif

#ifdef _MSC_VER //If Microsoft C++ compiler
#pragma warning(disable:4290) //WARNING: C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#endif

//  ==System dependent headers inclusion==
#if defined(__WIN32__) || defined(WIN32)
#ifndef __WIN32__
#define __WIN32__
#endif

#include <windows.h>

#else //assume linux

#include <ctime>

#endif
//~ ==System dependen headers inclusion==

#include <list>
#include <algorithm>

#include "debug.hpp" //debugging facilities
#include "types.hpp"
#include "Singleton.hpp"
#include "Thread.hpp"
#include "math.hpp"

namespace ting{



//forward declarations
class Timer;



//function prototypes
inline ting::u32 GetTicks();



class TimerLib : public Singleton<TimerLib>{
	friend class ting::Timer;

	class TimerThread : public ting::Thread{
	public:
		bool quitFlag;
		
		ting::Mutex mutex;
		ting::Semaphore sema;

		typedef std::list<Timer*> T_TimerList;
		typedef T_TimerList::iterator T_TimerIter;
		T_TimerList timers;

		bool warpFlag;//false if last call to GetTicks() returned value in first half

		TimerThread() :
				quitFlag(false)
		{}

		~TimerThread(){
			//at the time of DimerLib destroying there should be no active timers
			ASSERT(this->timers.size() == 0)
		}

		inline void AddTimer(Timer* timer, u32 timeout);

		inline bool RemoveTimer(Timer* timer);

		inline void SetQuitFlagAndSignalSema(){
			this->quitFlag = true;
			this->sema.Signal();
		}

		//override (inline is just to make possible method definition in hpp)
		inline void Run();

	private:
		inline void UpdateTimer(Timer* timer, u32 newTimeout);

	} thread;

	inline void AddTimer(Timer* timer, u32 timeout){
		this->thread.AddTimer(timer, timeout);
	}
	
	inline bool RemoveTimer(Timer* timer){
		return this->thread.RemoveTimer(timer);
	}

public:
	TimerLib(){
		this->thread.Start();
	}

	~TimerLib(){
		this->thread.SetQuitFlagAndSignalSema();
		this->thread.Join();
	}
};



class Timer{
	friend class TimerLib::TimerThread;

	ting::u32 endTime;
	bool warp;

	bool isStarted;
public:
	inline Timer() :
			warp(false),
			isStarted(false)
	{}

	virtual ~Timer(){
		ASSERT(TimerLib::IsCreated())
		this->Stop();
	}

	inline void Start(ting::u32 millisec){
		ASSERT_INFO(TimerLib::IsCreated(), "Timer library is not initialized, need to create TimerLib singletone object")
		
		this->Stop();//make sure the timer is not running already
		TimerLib::Inst().AddTimer(this, millisec);
	}

	/**
	 * @brief Stop the timer.
	 * Stops the timer if it was started before. In case it was not started
	 * or it is already expired this method does nothing.
	 * @return true - if the timer was successfuly interrupted. I.e. it was
	 *                in running state and has not expired at the time this
	 *                method was called.
	 * @return false - if the timer was not in running state (either not
	 *                 started or already expired) at the moment this method
	 *                 was called.
	 */
	inline bool Stop(){
		ASSERT(TimerLib::IsCreated())
		return TimerLib::Inst().RemoveTimer(this);
	}

	//return number fo milliseconds to reschedule this timer for,
	//return 0 for no timer rescheduling.
	virtual u32 OnExpire() = 0;
};



//methods

inline void TimerLib::TimerThread::UpdateTimer(Timer* timer, u32 newTimeout){
	ting::u32 curTicks = ting::GetTicks();

	timer->endTime = curTicks + newTimeout;

	if(timer->endTime < curTicks){
		timer->warp = true;
	}else{
		timer->warp = false;
	}
}



inline bool TimerLib::TimerThread::RemoveTimer(Timer* timer){
	ASSERT(timer)
	ting::Mutex::Guard mutexGuard(this->mutex);

	if(!timer->isStarted)
		return false;

	//if isStarted flag is set then the timer will be stopped now, so
	//change the flag
	timer->isStarted = false;

	for(T_TimerIter i = this->timers.begin(); i != this->timers.end(); ++i){
		if(*i == timer){
			this->timers.erase(i);
			this->sema.Signal();
			return true;
		}
	}

	//shall never get there because if timer->isStarted flag is set
	//then the timer have to be in the list
	ASSERT(false)

	return false;
}



inline void TimerLib::TimerThread::AddTimer(Timer* timer, u32 timeout){
	ASSERT(timer)
	ting::Mutex::Guard mutexGuard(this->mutex);

	ASSERT(!timer->isStarted)

	timer->isStarted = true;

	this->UpdateTimer(timer, timeout);

	this->timers.push_back(timer);
	this->sema.Signal();
}



//override
inline void TimerLib::TimerThread::Run(){
	M_TIMER_TRACE(<< "TimerLib::TimerThread::Run(): enter" << std::endl)
	//init warp flag
	if(ting::GetTicks() < ting::u32(-1) / 2){
		this->warpFlag = false;
	}else{
		this->warpFlag = true;
	}

	M_TIMER_TRACE(<< "TimerLib::TimerThread::Run(): creating mutex guard" << std::endl)
	ting::Mutex::Guard mutexGuard(this->mutex);
	M_TIMER_TRACE(<< "TimerLib::TimerThread::Run(): entering while()" << std::endl)
	while(!this->quitFlag){
		//check warp
		ting::u32 ticks = ting::GetTicks();
		M_TIMER_TRACE(<<"TimerThread: ticks = " << ticks << std::endl)

		if(ticks < ting::u32(-1) / 2){
			if(this->warpFlag){
				//Warp detected.
				//clear all warp flags, remove timers which was not warped
				for(T_TimerIter i = this->timers.begin(); i != this->timers.end();){
					//if the timer was not warped and we are warping
					//then this timer is surely expired, need to remove it and
					//notify client calling OnExpire().
					if(!(*i)->warp){
						u32 newTimeout = (*i)->OnExpire();
						if(newTimeout == 0){
							(*i)->isStarted = false;
							i = this->timers.erase(i);
							continue;
						}else{
							this->UpdateTimer(*i, newTimeout);
						}
					}else{
						(*i)->warp = false;
					}
					++i;
				}
			}
			this->warpFlag = false;
		}else{
			this->warpFlag = true;
		}

		//notify expired timers
		M_TIMER_TRACE(<<"TimerThread: search for expired timers, size = " << this->timers.size() << std::endl)
		for(T_TimerIter i = this->timers.begin(); i != this->timers.end();){
			if(!(*i)->warp){
				M_TIMER_TRACE(<<"TimerThread: warp is not set, endTime = "<< (*i)->endTime << std::endl)
				if((*i)->endTime <= ticks){
					u32 newTimeout = (*i)->OnExpire();
					if(newTimeout == 0){
						(*i)->isStarted = false;
						i = this->timers.erase(i);
						continue;
					}else{//set timer again
						this->UpdateTimer(*i, newTimeout);
					}
				}
			}else{
				M_TIMER_TRACE(<<"TimerThread: warp is set" << std::endl)
			}
			++i;
		}

		if(this->timers.size() == 0){
			this->mutex.Unlock();
			M_TIMER_TRACE(<<"TimerThread: waiting forever" << std::endl)
			this->sema.Wait();
			M_TIMER_TRACE(<<"TimerThread: signalled" << std::endl)
			this->mutex.Lock();
			continue;
		}

		//calculate number of milliseconds to wait
		ting::u32 minEndTime = ting::u32(-1);
		for(T_TimerIter i = this->timers.begin(); i != this->timers.end(); ++i){
			if(!(*i)->warp){
				if((*i)->endTime < minEndTime){
					minEndTime = (*i)->endTime;
				}
			}
		}

		unsigned millis = minEndTime - ticks;

		//make sure we will update warpFlag at least 4 times
		//per ticks cycle (ticks cycle = 0xffffffffff ticks)
		millis = (std::min)(millis, ting::u32(-1) / 4);//NOTE: enclose (std::min) into parentheses in order to avoid compilation errors when using MSVC compiler, because windows.h defines min and max macros.
		ASSERT(millis != 0)

		this->mutex.Unlock();

		M_TIMER_TRACE(<<"TimerThread: waiting for "<<millis<< " ms" << std::endl)
		this->sema.Wait(millis);
		M_TIMER_TRACE(<<"TimerThread: signalled" << std::endl)
		//It does not matter signalled or timed out

		this->mutex.Lock();
	}//~while

	M_TIMER_TRACE(<< "TimerLib::TimerThread::Run(): exit" << std::endl)
}//~Run()



/**
 * @brief Returns number of milliseconds since system start.
 * @return number of milliseconds passed since system start.
 */
inline ting::u32 GetTicks(){
#ifdef __WIN32__
	static LARGE_INTEGER perfCounterFreq = {{0, 0}};
	if(perfCounterFreq.QuadPart == 0){
		if(QueryPerformanceFrequency(&perfCounterFreq) == FALSE){
		//looks like the system does not support high resolution tick counter
			return GetTickCount();
		}
	}
	LARGE_INTEGER ticks;
	if(QueryPerformanceCounter(&ticks) == FALSE){
		return GetTickCount();
	}

	return ting::u32((ticks.QuadPart * 1000) / perfCounterFreq.QuadPart);
#else
	timespec ts;
	if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1)
		throw ting::Exc("GetTicks(): clock_gettime() returned error");

	return u32(u32(ts.tv_sec) * 1000 + u32(ts.tv_nsec / 1000000));
#endif
}



}//~namespace
