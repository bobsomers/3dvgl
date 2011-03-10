/* The MIT License:

Copyright (c) 2009-2010 Ivan Gagis

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
 * @file WaitSet.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Wait set.
 */

#pragma once

#include <vector>
#include <sstream>
#include <cerrno>

#include "types.hpp"
#include "debug.hpp"
#include "Exc.hpp"
#include "Array.hpp"



#if defined(__WIN32__) || defined(WIN32)
#ifndef __WIN32__
#define __WIN32__
#endif
#ifndef WIN32
#define WIN32
#endif

//if _WINSOCKAPI_ macro is not defined then it means that the winsock header file
//has not been included. Here we temporarily define the macro in order to prevent
//inclusion of winsock.h from within the windows.h. Because it may later conflict with
//winsock2.h if it is included later.
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#else
#include <windows.h>
#endif

#elif defined(__linux__)
#include <sys/epoll.h>
#else
#error "Unsupported OS"
#endif


//if Microsoft MSVC compiler,
//then disable warning about throw specification is ignored.
#ifdef _MSC_VER
#pragma warning(push) //push warnings state
#pragma warning( disable : 4290)
#endif


namespace ting{



/**
 * @brief Base class for objects which can be waited for.
 * Base class for objects which can be used in wait sets.
 */
class Waitable{
	friend class WaitSet;

	bool isAdded;

	void* userData;

public:
	enum EReadinessFlags{
		NOT_READY = 0,      // bin: 00000000
		READ = 1,           // bin: 00000001
		WRITE = 2,          // bin: 00000010
		READ_AND_WRITE = 3  // bin: 00000011
	};

protected:
	u32 readinessFlags;

	inline Waitable() :
			isAdded(false),
			userData(0),
			readinessFlags(NOT_READY)
	{}



	inline bool IsAdded()const{
		return this->isAdded;
	}



	/**
	 * @brief Copy constructor.
	 * It is not possible to copy a waitable which is currently added to WaitSet.
	 * Works as std::auto_ptr, i.e. the object it copied from becomes invalid.
	 * Use this copy constructor only if you really know what you are doing.
	 * @param w - Waitable object to copy.
	 */
	inline Waitable(const Waitable& w) :
			isAdded(false),
			userData(w.userData),
			readinessFlags(NOT_READY)//Treat copied Waitable as NOT_READY
	{
		//cannot copy from waitable which is added to WaitSet
		if(w.isAdded)
			throw ting::Exc("Waitable::Waitable(copy): cannot copy Waitable which is added to WaitSet");

		const_cast<Waitable&>(w).ClearAllReadinessFlags();
		const_cast<Waitable&>(w).userData = 0;
	}



	/**
	 * @brief Assignment operator.
	 * It is not possible to assign a waitable which is currently added to WaitSet.
	 * Works as std::auto_ptr, i.e. the object it copied from becomes invalid.
	 * Use this copy constructor only if you really know what you are doing.
	 * @param w - Waitable object to assign to this object.
	 */
	inline Waitable& operator=(const Waitable& w){
		//cannot copy because this Waitable is added to WaitSet
		if(this->isAdded)
			throw ting::Exc("Waitable::Waitable(copy): cannot copy while this Waitable is added to WaitSet");

		//cannot copy from waitable which is adde to WaitSet
		if(w.isAdded)
			throw ting::Exc("Waitable::Waitable(copy): cannot copy Waitable which is added to WaitSet");

		ASSERT(!this->isAdded)

		//Clear readiness flags on copying.
		//Will need to wait for readiness again, using the WaitSet.
		this->ClearAllReadinessFlags();
		const_cast<Waitable&>(w).ClearAllReadinessFlags();

		this->userData = w.userData;
		const_cast<Waitable&>(w).userData = 0;
		return *this;
	}



	inline void SetCanReadFlag(){
		this->readinessFlags |= READ;
	}

	inline void ClearCanReadFlag(){
		this->readinessFlags &= (~READ);
	}

	inline void SetCanWriteFlag(){
		this->readinessFlags |= WRITE;
	}

	inline void ClearCanWriteFlag(){
		this->readinessFlags &= (~WRITE);
	}

	inline void ClearAllReadinessFlags(){
		this->readinessFlags = NOT_READY;
	}

public:
	virtual ~Waitable(){
		ASSERT(!this->isAdded)
	}

	inline bool CanRead()const{
		return (this->readinessFlags & READ) != 0;
	}

	inline bool CanWrite()const{
		return (this->readinessFlags & WRITE) != 0;
	}

	inline void* GetUserData(){
		return this->userData;
	}

	inline void SetUserData(void* data){
		this->userData = data;
	}

#ifdef __WIN32__
protected:
	virtual HANDLE GetHandle() = 0;

	virtual void SetWaitingEvents(u32 /*flagsToWaitFor*/){}

	//returns true if signalled
	virtual bool CheckSignalled(){
		return this->readinessFlags != 0;
	}



#elif defined(__linux__)
protected:
	virtual int GetHandle() = 0;
#else
#error "Unsupported OS"
#endif
};//~class Waitable





/**
 * @brief Set of Waitable objects to wait for.
 */
class WaitSet{
	unsigned size;
	unsigned numWaitables;//number of Waitables added

#if defined(__WIN32__)
	Array<Waitable*> waitables;
	Array<HANDLE> handles; //used to pass array of HANDLEs to WaitForMultipleObjectsEx()

#elif defined(__linux__)
	int epollSet;

	Array<epoll_event> revents;//used for getting the result from epoll_wait()
#else
#error "Unsupported OS"
#endif

public:

	/**
	 * @brief Constructor.
	 * @param maxSize - maximum number of Waitable objects can be added to this wait set.
	 */
	WaitSet(unsigned maxSize) :
			size(maxSize),
			numWaitables(0)
#if defined(__WIN32__)
			,waitables(maxSize)
			,handles(maxSize)
	{
		ASSERT_INFO(maxSize <= MAXIMUM_WAIT_OBJECTS, "maxSize should be less than " << MAXIMUM_WAIT_OBJECTS)
		if(maxSize > MAXIMUM_WAIT_OBJECTS)
			throw ting::Exc("WaitSet::WaitSet(): requested WaitSet size is too big");
	}

#elif defined(__linux__)
			,revents(maxSize)
	{
		ASSERT(int(maxSize) > 0)
		this->epollSet = epoll_create(int(maxSize));
		if(this->epollSet < 0){
			throw ting::Exc("WaitSet::WaitSet(): epoll_create() failed");
		}
	}
#else
#error "Unsupported OS"
#endif



	/**
	 * @brief Destructor.
	 * Note, that destructor will check if the wait set is empty. If it is not, then an assert
	 * will be triggered.
	 * It is user's responsibility to remove any waitable objects from the waitset
	 * before the wait set object is destroyed.
	 */
	~WaitSet(){
		//assert the wait set is empty
		ASSERT_INFO(this->numWaitables == 0, "attempt to destroy WaitSet containig Waitables")
#if defined(__WIN32__)
		//do nothing
#elif defined(__linux__)
		close(this->epollSet);
#else
#error "Unsupported OS"
#endif
	}



	/**
	 * @brief Get maximum size of the wait set.
	 * @return maximum number of Waitables this WaitSet can hold.
	 */
	inline unsigned Size()const{
		return this->size;
	}


	/**
	 * @brief Add Waitable object to the wait set.
	 * @param w - pointer to the Waitable object.
	 * @param flagsToWaitFor - determine events waiting for which we are interested.
	 * @throw ting::Exc - in case the wait set is full or other error occurs.
	 */
	inline void Add(Waitable* w, Waitable::EReadinessFlags flagsToWaitFor){
//		TRACE(<< "WaitSet::Add(): enter" << std::endl)
		ASSERT(w)

		ASSERT(!w->isAdded)

#if defined(__WIN32__)
		ASSERT(this->numWaitables <= this->handles.Size())
		if(this->numWaitables == this->handles.Size())
			throw ting::Exc("WaitSet::Add(): wait set is full");

		//NOTE: Setting wait flags may throw an exception, so do that before
		//adding object to the array and incrementing number of added objects.
		w->SetWaitingEvents(flagsToWaitFor);

		this->handles[this->numWaitables] = w->GetHandle();
		this->waitables[this->numWaitables] = w;

#elif defined(__linux__)
		epoll_event e;
		e.data.fd = w->GetHandle();
		e.data.ptr = w;
		e.events =
				(u32(flagsToWaitFor) & Waitable::READ ? (EPOLLIN | EPOLLPRI) : 0)
				| (u32(flagsToWaitFor) & Waitable::WRITE ? EPOLLOUT : 0)
				| (EPOLLERR);
		int res = epoll_ctl(
				this->epollSet,
				EPOLL_CTL_ADD,
				w->GetHandle(),
				&e
			);
		if(res < 0)
			throw ting::Exc("WaitSet::Add(): epoll_ctl() failed");
#else
#error "Unsupported OS"
#endif

		++this->numWaitables;

		w->isAdded = true;
//		TRACE(<< "WaitSet::Add(): exit" << std::endl)
	}



	/**
	 * @brief Change wait flags for a given Waitable.
	 * Changes wait flags for a given waitable, which is in this waitset.
	 * @param w - pointer to Waitable for which the changing of wait flags is needed.
	 * @param flagsToWaitFor - new wait flags to be set for the given Waitable.
	 * @throw ting::Exc - in case the given Waitable object is not added to this wait set or
	 *                    other error occurs.
	 */
	inline void Change(Waitable* w, Waitable::EReadinessFlags flagsToWaitFor){
		ASSERT(w)

		ASSERT(w->isAdded)

#if defined(__WIN32__)
		//check if the Waitable object is added to this wait set
		{
			unsigned i;
			for(i = 0; i < this->numWaitables; ++i){
				if(this->waitables[i] == w)
					break;
			}
			ASSERT(i <= this->numWaitables)
			if(i == this->numWaitables)
				throw ting::Exc("WaitSet::Change(): the Waitable is not added to this wait set");
		}

		//set new wait flags
		w->SetWaitingEvents(flagsToWaitFor);

#elif defined(__linux__)
		epoll_event e;
		e.data.fd = w->GetHandle();
		e.data.ptr = w;
		e.events =
				(u32(flagsToWaitFor) & Waitable::READ ? (EPOLLIN | EPOLLPRI) : 0)
				| (u32(flagsToWaitFor) & Waitable::WRITE ? EPOLLOUT : 0)
				| (EPOLLERR);
		int res = epoll_ctl(
				this->epollSet,
				EPOLL_CTL_MOD,
				w->GetHandle(),
				&e
			);
		if(res < 0)
			throw ting::Exc("WaitSet::Change(): epoll_ctl() failed");
#else
#error "Unsupported OS"
#endif
	}



	/**
	 * @brief Remove Waitable from wait set.
	 * @param w - pointer to Waitable object to be removed from the wait set.
	 * @throw ting::Exc - in case the given Waitable is not added to this wait set or
	 *                    other error occurs.
	 */
	inline void Remove(Waitable* w){
		ASSERT(w)

		ASSERT(w->isAdded)

#if defined(__WIN32__)
		//remove object from array
		{
			unsigned i;
			for(i = 0; i < this->numWaitables; ++i){
				if(this->waitables[i] == w)
					break;
			}
			ASSERT(i <= this->numWaitables)
			if(i == this->numWaitables)
				throw ting::Exc("WaitSet::Change(): the Waitable is not added to this wait set");

			unsigned numObjects = this->numWaitables - 1;//decrease number of objects before shifting the object handles in the array
			//shift object handles in the array
			for(; i < numObjects; ++i){
				this->handles[i] = this->handles[i + 1];
				this->waitables[i] = this->waitables[i + 1];
			}
		}

		//clear wait flags (disassociate socket and Windows event)
		w->SetWaitingEvents(0);

#elif defined(__linux__)
		int res = epoll_ctl(
				this->epollSet,
				EPOLL_CTL_DEL,
				w->GetHandle(),
				0
			);
		if(res < 0)
			throw ting::Exc("WaitSet::Remove(): epoll_ctl() failed");
#else
#error "Unsupported OS"
#endif

		--this->numWaitables;

		w->isAdded = false;
//		TRACE(<< "WaitSet::Remove(): completed successfuly" << std::endl)
	}



	/**
	 * @brief wait for event.
	 * This function blocks calling thread exectution until one of the Waitable objects in the WaitSet
	 * triggers. Upon return from the function, pointers to triggered objects are placed in the
	 * 'out_events' buffer and the return value from the function indicates number of these objects
	 * which have triggered.
	 * @param out_events - pointer to buffer where to put pointers to triggered Waitable objects.
	 *                     The buffer will not be initialized to 0's by this function.
	 *                     The buffer shall be large enough to hold maxmimum number of Waitables
	 *                     this WaitSet can hold.
	 *                     It is valid to pass 0 pointer, in that case this argument will not be used.
	 * @return number of objects triggered.
	 * @throw ting::Exc - in case of errors.
	 */
	inline unsigned Wait(Buffer<Waitable*>* out_events = 0){
		return this->Wait(true, 0, out_events);
	}


	/**
	 * @brief wait for event with timeout.
	 * The same as Wait() function, but takes wait timeout as parameter. Thus,
	 * this function will wait for any event or timeout. Note, that it guarantees that
	 * it will wait AT LEAST for specified number of milliseconds, or more. This is because of
	 * implementation for linux, if wait is interrupted by signal it will start waiting again,
	 * and so on.
	 * @param timeout - maximum time in milliseconds to wait for event.
	 * @param out_events - pointer to buffer where to put pointers to triggered Waitable objects.
	 *                     The buffer size must be equal or greater than the number ow waitables
	 *                     currently added to the wait set.
	 *                     This pointer can be 0, if you are not interested in list of triggered waitables.
	 * @return number of objects triggered. If 0 then timeout was hit.
	 * @throw ting::Exc - in case of errors.
	 */
	inline unsigned WaitWithTimeout(u32 timeout, Buffer<Waitable*>* out_events = 0){
		return this->Wait(false, timeout, out_events);
	}



private:
	unsigned Wait(bool waitInfinitly, u32 timeout, Buffer<Waitable*>* out_events){
		if(out_events){
			if(out_events->Size() < this->numWaitables){
				throw ting::Exc("WaitSet::Wait(): passed out_events buffer is not large enough to hold all possible triggered objects");
			}
		}

#if defined(__WIN32__)
		if(timeout == INFINITE)
			timeout -= 1;

		DWORD waitTimeout = waitInfinitly ? (INFINITE) : DWORD(timeout);
		ASSERT(waitTimeout >= 0)
		DWORD res = WaitForMultipleObjectsEx(
				this->numWaitables,
				this->handles.Begin(),
				FALSE, //do not wait for all objects, wait for at least one
				waitTimeout,
				FALSE
			);

		ASSERT(res != WAIT_IO_COMPLETION)//it is impossible because we supplied FALSE as last parameter to WaitForMultipleObjectsEx()

		if(res == WAIT_FAILED)
			throw ting::Exc("WaitSet::Wait(): WaitForMultipleObjectsEx() failed");

		if(res == WAIT_TIMEOUT)
			return 0;

		//check for activities
		unsigned numEvents = 0;
		for(unsigned i = 0; i < this->numWaitables; ++i){
			if(this->waitables[i]->CheckSignalled()){
				if(out_events){
					ASSERT(numEvents < out_events->Size())
					out_events->operator[](numEvents) = this->waitables[i];
				}
				++numEvents;
			}
		}

		return numEvents;

#elif defined(__linux__)
		ASSERT(int(timeout) >= 0)
		int epollTimeout = waitInfinitly ? (-1) : int(timeout);

//		TRACE(<< "going to epoll_wait() with timeout = " << epollTimeout << std::endl)

		int res;

		while(true){
			res = epoll_wait(
					this->epollSet,
					this->revents.Begin(),
					this->revents.Size(),
					epollTimeout
				);

//			TRACE(<< "epoll_wait() returned " << res << std::endl)

			if(res < 0){
				//if interrupted by signal, try waiting again.
				if(errno == EINTR){
					continue;
				}

				std::stringstream ss;
				ss << "WaitSet::Wait(): epoll_wait() failed, error code = " << errno << ": " << strerror(errno);
				throw ting::Exc(ss.str().c_str());
			}
			break;
		};

		ASSERT(unsigned(res) <= this->revents.Size())

		unsigned numEvents = 0;
		for(
				epoll_event *e = this->revents.Begin();
				e < this->revents.Begin() + res;
				++e
			)
		{
			Waitable* w = static_cast<Waitable*>(e->data.ptr);
			ASSERT(w)
			if((e->events & (EPOLLIN | EPOLLPRI | EPOLLERR)) != 0){
				w->SetCanReadFlag();
			}
			if((e->events & EPOLLOUT) != 0){
				w->SetCanWriteFlag();
			}
			ASSERT(w->CanRead() || w->CanWrite())
			if(out_events){
				ASSERT(numEvents < out_events->Size())
				out_events->operator[](numEvents) = w;
				++numEvents;
			}
		}

		ASSERT(res >= 0)//NOTE: 'res' can be zero, if no events happened in the specified timeout
		return unsigned(res);
#else
#error "Unsupported OS"
#endif
	}
};//~class WaitSet



}//~namespace ting


//if Microsoft MSVC compiler, restore warnings state
#ifdef _MSC_VER
#pragma warning(pop) //pop warnings state
#endif

