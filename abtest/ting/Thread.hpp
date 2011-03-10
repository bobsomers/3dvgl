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
 * @file Thread.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Multithreading library.
 */

#pragma once

#include <cstring>
#include <sstream>

#include "debug.hpp"
#include "Ptr.hpp"
#include "types.hpp"
#include "Exc.hpp"
#include "WaitSet.hpp"

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

#include <process.h>

#elif defined(__SYMBIAN32__)
#include <string.h>
#include <e32std.h>
#include <hal.h>

#else //assume pthread
#define M_PTHREAD
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <ctime>

//if we have Solaris
#if defined(sun) || defined(__sun)
#include <sched.h>	//	for sched_yield();
#endif

#if defined(__linux__)
#include <sys/eventfd.h>
#endif

#endif



//if Microsoft MSVC compiler,
//then disable warning about throw specification is ignored.
#ifdef _MSC_VER
#pragma warning(push) //push warnings state
#pragma warning( disable : 4290)
#endif



//#define M_ENABLE_MUTEX_TRACE
#ifdef M_ENABLE_MUTEX_TRACE
#define M_MUTEX_TRACE(x) TRACE(<<"[MUTEX] ") TRACE(x)
#else
#define M_MUTEX_TRACE(x)
#endif


//#define M_ENABLE_QUEUE_TRACE
#ifdef M_ENABLE_QUEUE_TRACE
#define M_QUEUE_TRACE(x) TRACE(<<"[QUEUE] ") TRACE(x)
#else
#define M_QUEUE_TRACE(x)
#endif


namespace ting{

//forward declarations
class CondVar;
class Queue;
class Thread;
class QuitMessage;

/**
 * @brief Mutex object class
 * Mutex stands for "Mutual execution".
 */
class Mutex{
	friend class CondVar;

	//system dependent handle
#ifdef __WIN32__
	CRITICAL_SECTION m;
#elif defined(__SYMBIAN32__)
	RCriticalSection m;
#elif defined(M_PTHREAD)
	pthread_mutex_t m;
#else
#error "unknown system"
#endif

	//forbid copying
	Mutex(const Mutex& ){
		ASSERT(false)
	}
	Mutex& operator=(const Mutex& ){
		return *this;
	}

public:
	/**
	 * @brief Creates initially unlocked mutex.
	 */
	Mutex(){
		M_MUTEX_TRACE(<< "Mutex::Mutex(): invoked " << this << std::endl)
#ifdef __WIN32__
		InitializeCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
		if(this->m.CreateLocal() != KErrNone){
			throw ting::Exc("Mutex::Mutex(): failed creating mutex (CreateLocal() failed)");
		}
#elif defined(M_PTHREAD) //pthread
		if(pthread_mutex_init(&this->m, NULL) != 0){
			throw ting::Exc("Mutex::Mutex(): failed creating mutex (pthread_mutex_init() failed)");
		}
#else
#error "unknown system"
#endif
	}



	~Mutex(){
		M_MUTEX_TRACE(<< "Mutex::~Mutex(): invoked " << this << std::endl)
#ifdef __WIN32__
		DeleteCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
		this->m.Close();
#elif defined(M_PTHREAD) //pthread
		int ret = pthread_mutex_destroy(&this->m);
		if(ret != 0){
			std::stringstream ss;
			ss << "Mutex::~Mutex(): pthread_mutex_destroy() failed"
					<< " error code = " << ret << ": " << strerror(ret) << ".";
			if(ret == EBUSY){
				ss << " You are trying to destroy locked mutex.";
			}
			ASSERT_INFO_ALWAYS(false, ss.str())
		}
#else
#error "unknown system"
#endif
	}



	/**
	 * @brief Acquire mutex lock.
	 * If one thread acquired the mutex lock then all other threads
	 * attempting to acquire the lock on the same mutex will wait until the
	 * mutex lock will be released with Mutex::Unlock().
	 */
	void Lock(){
		M_MUTEX_TRACE(<< "Mutex::Lock(): invoked " << this << std::endl)
#ifdef __WIN32__
		EnterCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
		this->m.Wait();
#elif defined(M_PTHREAD) //pthread
		pthread_mutex_lock(&this->m);
#else
#error "unknown system"
#endif
	}



	/**
	 * @brief Release mutex lock.
	 */
	void Unlock(){
		M_MUTEX_TRACE(<< "Mutex::Unlock(): invoked " << this << std::endl)
#ifdef __WIN32__
		LeaveCriticalSection(&this->m);
#elif defined(__SYMBIAN32__)
		this->m.Signal();
#elif defined(M_PTHREAD) //pthread
		pthread_mutex_unlock(&this->m);
#else
#error "unknown system"
#endif
	}



	/**
	 * @brief Helper class which automatically Locks the given mutex.
	 * This helper class automatically locks the given mutex in the constructor and
	 * unlocks the mutex in destructor. This class is useful if the code between
	 * mutex lock/unlock may return or throw an exception,
	 * then the mutex be automaticlly unlocked in such case.
	 */
	class Guard{
		Mutex *mut;

		//forbid copying
		Guard(const Guard& ){
			ASSERT(false)
		}
		Guard& operator=(const Guard& ){
			return *this;
		}
	public:
		Guard(Mutex &m):
				mut(&m)
		{
			this->mut->Lock();
		}
		~Guard(){
			this->mut->Unlock();
		}
	};//~class Guard
};//~class Mutex



/**
 * @brief Semaphore class.
 * The semaphore is actually an unsigned integer value which can be incremented
 * (by Semaphore::Signal()) or decremented (by Semaphore::Wait()). If the value
 * is 0 then any try to decrement it will result in execution blocking of the current thread
 * until the value is incremented so the thread will be able to
 * decrement it. If there are several threads waiting for semaphore decrement and
 * some other thread increments it then only one of the hanging threads will be
 * resumed, other threads will remain waiting for next increment.
 */
class Semaphore{
	//system dependent handle
#ifdef __WIN32__
	HANDLE s;
#elif defined(__SYMBIAN32__)
	RSemaphore s;
#elif defined(M_PTHREAD)
	sem_t s;
#else
#error "unknown system"
#endif

	//forbid copying
	Semaphore(const Semaphore& ){
		ASSERT(false)
	}
	Semaphore& operator=(const Semaphore& ){
		return *this;
	}
public:

	/**
	 * @brief Create the semaphore with given initial value.
	 */
	Semaphore(unsigned initialValue = 0){
#ifdef __WIN32__
		if( (this->s = CreateSemaphore(NULL, initialValue, 0xffffff, NULL)) == NULL)
#elif defined(__SYMBIAN32__)
		if(this->s.CreateLocal(initialValue) != KErrNone)
#elif defined(M_PTHREAD)
		if(sem_init(&this->s, 0, initialValue) < 0 )
#else
#error "unknown system"
#endif
		{
			LOG(<<"Semaphore::Semaphore(): failed"<<std::endl)
			throw ting::Exc("Semaphore::Semaphore(): creating semaphore failed");
		}
	}



	~Semaphore(){
#ifdef __WIN32__
		CloseHandle(this->s);
#elif defined(__SYMBIAN32__)
		this->s.Close();
#elif defined(M_PTHREAD)
		sem_destroy(&this->s);
#else
#error "unknown system"
#endif
	}



	/**
	 * @brief Wait on semaphore.
	 * Decrments semaphore value. If current value is 0 then this method will wait
	 * until some other thread signalls the semaphore (i.e. increments the value)
	 * by calling Semaphore::Signal() on that semaphore.
	 * @param timeoutMillis - waiting timeout.
	 *                        If timeoutMillis is 0 (the default value) then this
	 *                        method will wait forever or until the semaphore is
	 *                        signalled.
	 * @return returns true if the semaphore value was decremented.
	 * @return returns false if the timeout was hit.
	 */
	bool Wait(unsigned timeoutMillis = 0){
#ifdef __WIN32__
		switch(WaitForSingleObject(this->s, DWORD(timeoutMillis == 0 ? INFINITE : timeoutMillis))){
			case WAIT_OBJECT_0:
//				LOG(<<"Semaphore::Wait(): exit"<<std::endl)
				return true;
			case WAIT_TIMEOUT:
				return false;
				break;
			default:
				throw ting::Exc("Semaphore::Wait(): wait failed");
				break;
		}
#elif defined(__SYMBIAN32__)
		if(timeoutMillis == 0){
			this->s.Wait();
		}else{
			throw ting::Exc("Semaphore::Wait(): timeouted wait unimplemented on Symbian, TODO: implement");
		}
#elif defined(M_PTHREAD)
		if(timeoutMillis == 0){
			int retVal;
			do{
				retVal = sem_wait(&this->s);
			}while(retVal == -1 && errno == EINTR);
			if(retVal < 0){
				throw ting::Exc("Semaphore::Wait(): wait failed");
			}
		}else{
			timespec ts;

			if(clock_gettime(CLOCK_REALTIME, &ts) == -1)
				throw ting::Exc("Semaphore::Wait(): clock_gettime() returned error");

			ts.tv_sec += timeoutMillis / 1000;
			ts.tv_nsec += (timeoutMillis % 1000) * 1000 * 1000;
			ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
			ts.tv_nsec = ts.tv_nsec % (1000 * 1000 * 1000);

			if(sem_timedwait(&this->s, &ts) == -1){
				if(errno == ETIMEDOUT)
					return false;
				else
					throw ting::Exc("Semaphore::Wait(): error");
			}
		}
#else
#error "unknown system"
#endif
		return true;
	}



	/**
	 * @brief Signal the semaphore.
	 * Increments the semaphore value.
	 */
	inline void Signal(){
//		TRACE(<< "Semaphore::Signal(): invoked" << std::endl)
#ifdef __WIN32__
		if( ReleaseSemaphore(this->s, 1, NULL) == 0 ){
			throw ting::Exc("Semaphore::Post(): releasing semaphore failed");
		}
#elif defined(__SYMBIAN32__)
		this->s.Signal();
#elif defined(M_PTHREAD)
		if(sem_post(&this->s) < 0){
			throw ting::Exc("Semaphore::Post(): releasing semaphore failed");
		}
#else
#error "unknown system"
#endif
	}
};//~class Semaphore



class CondVar{
#if defined(WIN32) || defined(__SYMBIAN32__)
	Mutex cvMutex;
	Semaphore semWait;
	Semaphore semDone;
	unsigned numWaiters;
	unsigned numSignals;
#elif defined(M_PTHREAD)
	//A pointer to store system dependent handle
	pthread_cond_t cond;
#else
#error "unknown system"
#endif

	//forbid copying
	CondVar(const CondVar& ){
		ASSERT(false)
	}
	CondVar& operator=(const CondVar& ){
		return *this;
	}
	
public:

	CondVar(){
#if defined(__WIN32__) || defined(__SYMBIAN32__)
		this->numWaiters = 0;
		this->numSignals = 0;
#elif defined(M_PTHREAD)
		pthread_cond_init(&this->cond, NULL);
#else
#error "unknown system"
#endif
	}

	~CondVar(){
#if defined(__WIN32__) || defined(__SYMBIAN32__)
#elif defined(M_PTHREAD)
	pthread_cond_destroy(&this->cond);
#else
#error "unknown system"
#endif
	}

	void Wait(Mutex& mutex){
#if defined(__WIN32__) || defined(__SYMBIAN32__)
		this->cvMutex.Lock();
		++this->numWaiters;
		this->cvMutex.Unlock();

		mutex.Unlock();

		this->semWait.Wait();

		this->cvMutex.Lock();
		if(this->numSignals > 0){
			this->semDone.Signal();
			--this->numSignals;
		}
		--this->numWaiters;
		this->cvMutex.Unlock();

		mutex.Lock();
#elif defined(M_PTHREAD)
		pthread_cond_wait(&this->cond, &mutex.m);
#else
#error "unknown system"
#endif
	}

	void Notify(){
#if defined(__WIN32__) || defined(__SYMBIAN32__)
		this->cvMutex.Lock();

		if(this->numWaiters > this->numSignals){
			++this->numSignals;
			this->semWait.Signal();
			this->cvMutex.Unlock();
			this->semDone.Wait();
		}else{
			this->cvMutex.Unlock();
		}
#elif defined(M_PTHREAD)
		pthread_cond_signal(&this->cond);
#else
#error "unknown system"
#endif
	}
};



/**
 * @brief Message abstract class.
 * The messages are sent to message queues (see ting::Queue). One message instance cannot be sent to
 * two or more message queues, but only to a single queue. When sent, the message is
 * further owned by the queue (note the usage of ting::Ptr auto-pointers).
 */
class Message{
	friend class Queue;

	Message *next;//pointer to the next message in a single-linked list

protected:
	Message() :
			next(0)
	{}

public:
	virtual ~Message(){}

	/**
	 * @brief message handler function.
	 * This virtual method is called to handle the message. When deriving from ting::Message,
	 * override this method to define the message handler procedure.
	 */
	virtual void Handle() = 0;
};



/**
 * @brief Message queue.
 * Message queue is used for communication of separate threads by
 * means of sending messages to each other. Thus, when one thread sends a message to another one,
 * it asks that another thread to execute some code portion - handler code of the message.
 * Each Thread object already contains its own queue object (Thread::queue), but one is free to
 * create his/her own Queue objects when they are needed.
 * NOTE: Queue implements Waitable interface which means that it can be used in conjunction
 * with ting::WaitSet API. But, note, that the implementation of the Waitable is that it
 * shall only be used to wait for READ. If you are trying to wait for WRITE the behavior will be
 * undefined.
 */
class Queue : public Waitable{
	Semaphore sem;

	Mutex mut;

	Message *first,
			*last;

#if defined(__WIN32__)
	//use additional semaphore to implement Waitable on Windows
	HANDLE eventForWaitable;
#elif defined(__linux__)
	//use eventfd()
	int eventFD;
#else
	//use pipe to implement Waitable in *nix systems
	int pipeEnds[2];
#endif

	//forbid copying
	Queue(const Queue&);
	Queue& operator=(const Queue&);

public:
	/**
	 * @brief Constructor, creates empty message queue.
	 */
	Queue() :
			first(0),
			last(0)
	{
		//can write will always be set because it is always possible to post a message to the queue
		this->SetCanWriteFlag();

#if defined(__WIN32__)
		this->eventForWaitable = CreateEvent(
				NULL,
				TRUE, //manual-reset
				FALSE, //not signalled initially
				NULL //no name
			);
		if(this->eventForWaitable == NULL){
			throw ting::Exc("Queue::Queue(): could not create event (Win32) for implementing Waitable");
		}
#elif defined(__linux__)
		this->eventFD = eventfd(0, EFD_NONBLOCK);
		if(this->eventFD < 0){
			std::stringstream ss;
			ss << "Queue::Queue(): could not create eventfd (linux) for implementing Waitable,"
					<< " error code = " << errno << ": " << strerror(errno);
			throw ting::Exc(ss.str().c_str());
		}
#else // assume *nix
		if(::pipe(&this->pipeEnds[0]) < 0){
			std::stringstream ss;
			ss << "Queue::Queue(): could not create pipe (*nix) for implementing Waitable,"
					<< " error code = " << errno << ": " << strerror(errno);
			throw ting::Exc(ss.str().c_str());
		}
#endif
	}



	/**
	 * @brief Destructor.
	 * When called, it also destroys all messages on the queue.
	 */
	~Queue(){
		//destroy messages which are currently on the queue
		{
			Mutex::Guard mutexGuard(this->mut);
			Message *msg = this->first;
			Message	*nextMsg;
			while(msg){
				nextMsg = msg->next;
				//use Ptr to kill messages instead of "delete msg;" because
				//the messages are passed to PushMessage() as Ptr, and thus, it is better
				//to use Ptr to delete them.
				{Ptr<Message> killer(msg);}

				msg = nextMsg;
			}
		}
#if defined(__WIN32__)
		CloseHandle(this->eventForWaitable);
#elif defined(__linux__)
		close(this->eventFD);
#else // assume *nix
		close(this->pipeEnds[0]);
		close(this->pipeEnds[1]);
#endif
	}



	/**
	 * @brief Pushes a new message to the queue.
	 * @param msg - the message to push into the queue.
	 */
	void PushMessage(Ptr<Message> msg){
		ASSERT(msg.IsValid())
		Mutex::Guard mutexGuard(this->mut);
		if(this->first){
			ASSERT(this->last && this->last->next == 0)
			this->last = this->last->next = msg.Extract();
			ASSERT(this->last->next == 0)
		}else{
			ASSERT(msg.IsValid())
			this->last = this->first = msg.Extract();

			//Set CanRead flag.
			//NOTE: in linux imlementation with epoll(), the CanRead
			//flag will also be set in WaitSet::Wait() method.
			//NOTE: set CanRead flag before event notification/pipe write, because
			//if do it after then some other thread which was witing on the WaitSet
			//may read the CanRead flag while it was not set yet.
			ASSERT(!this->CanRead())
			this->SetCanReadFlag();
			
#if defined(__WIN32__)
			if(SetEvent(this->eventForWaitable) == 0){
				throw ting::Exc("Queue::PushMessage(): setting event for Waitable failed");
			}
#elif defined(__linux__)
			if(eventfd_write(this->eventFD, 1) < 0){
				throw ting::Exc("Queue::PushMessage(): eventfd_write() failed");
			}
#else
			{
				u8 oneByteBuf[1];
				write(this->pipeEnds[1], oneByteBuf, 1);
			}
#endif
		}

		ASSERT(this->CanRead())
		//NOTE: must do signalling while mutex is locked!!!
		this->sem.Signal();
	}



	/**
	 * @brief Get message from queue, does not block if no messages queued.
	 * This method gets a message from message queue. If there are no messages on the queue
	 * it will return invalid auto pointer.
	 * @return auto-pointer to Message instance.
	 * @return invalid auto-pointer if there are no messares in the queue.
	 */
	Ptr<Message> PeekMsg(){
		Mutex::Guard mutexGuard(this->mut);
		if(this->first){
			//NOTE: Decrement semaphore value, because we take one message from queue.
			//      The semaphore value should be > 0 here, so there will be no hang
			//      in Wait().
			//      The semaphore value actually reflects the number of Messages in
			//      the queue.
			this->sem.Wait();
			Message* ret = this->first;
			this->first = this->first->next;

			if(this->first == 0){
#if defined(__WIN32__)
				if(ResetEvent(this->eventForWaitable) == 0){
					ASSERT(false)
					throw ting::Exc("Queue::Wait(): ResetEvent() failed");
				}
#elif defined(__linux__)
				{
					eventfd_t value;
					if(eventfd_read(this->eventFD, &value) < 0){
						throw ting::Exc("Queue::Wait(): eventfd_read() failed");
					}
					ASSERT(value == 1)
				}
#else
				{
					u8 oneByteBuf[1];
					read(this->pipeEnds[0], oneByteBuf, 1);
				}
#endif
			}

			if(!this->first) //clear 'can read' flag if no messages left on the queue
				this->ClearCanReadFlag();

			return Ptr<Message>(ret);
		}
		return Ptr<Message>();
	}



	/**
	 * @brief Get message from queue, blocks if no messages queued.
	 * This method gets a message from message queue. If there are no messages on the queue
	 * it will wait until any message is posted to the queue.
	 * Note, that this method, due to its implementation, is not intended to be called from
	 * multiple threads simultaneously (unlike Queue::PeekMsg()).
	 * If it is called from multiple threads the behavior will be undefined.
	 * It is also forbidden to call GetMsg() from one thread and PeekMsg() from another
	 * thread on the same Queue instance, because it will also lead to undefined behavior.
	 * @return auto-pointer to Message instance.
	 */
	Ptr<Message> GetMsg(){
		M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): enter" << std::endl)
		{
			Mutex::Guard mutexGuard(this->mut);
			if(this->first){
				//NOTE: Decrement semaphore value, because we take one message from queue.
				//      The semaphore value should be > 0 here, so there will be no hang
				//      in Wait().
				//      The semaphore value actually reflects the number of Messages in
				//      the queue.
				this->sem.Wait();
				Message* ret = this->first;
				this->first = this->first->next;

				if(this->first == 0){
#if defined(__WIN32__)
					if(ResetEvent(this->eventForWaitable) == 0){
						ASSERT(false)
						throw ting::Exc("Queue::Wait(): ResetEvent() failed");
					}
#elif defined(__linux__)
					{
						eventfd_t value;
						if(eventfd_read(this->eventFD, &value) < 0){
							throw ting::Exc("Queue::Wait(): eventfd_read() failed");
						}
						ASSERT(value == 1)
					}
#else
					u8 oneByteBuf[1];
					read(this->pipeEnds[0], oneByteBuf, 1);
#endif
				}

				if(!this->first) //clear 'can read' flag if no messages left on the queue
					this->ClearCanReadFlag();

				M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): exit without waiting on semaphore" << std::endl)
				return Ptr<Message>(ret);
			}
		}
		M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): waiting" << std::endl)
		ASSERT_EXEC(this->sem.Wait())
		M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): signalled" << std::endl)
		{
			Mutex::Guard mutexGuard(this->mut);
			ASSERT(this->first)
			Message* ret = this->first;
			this->first = this->first->next;

			if(this->first == 0){
#if defined(__WIN32__)
				if(ResetEvent(this->eventForWaitable) == 0){
					ASSERT(false)
					throw ting::Exc("Queue::Wait(): ResetEvent() failed");
				}
#elif defined(__linux__)
				{
					eventfd_t value;
					if(eventfd_read(this->eventFD, &value) < 0){
						throw ting::Exc("Queue::Wait(): eventfd_read() failed");
					}
					ASSERT(value == 1)
				}
#else
				u8 oneByteBuf[1];
				read(this->pipeEnds[0], oneByteBuf, 1);
#endif
			}

			if(!this->first) //clear 'can read' flag if no messages left on the queue
				this->ClearCanReadFlag();

			M_QUEUE_TRACE(<< "Queue[" << this << "]::GetMsg(): exit after waiting on semaphore" << std::endl)
			return Ptr<Message>(ret);
		}
	}

private:
#ifdef __WIN32__
	//override
	HANDLE GetHandle(){
		//return event handle
		return this->eventForWaitable;
	}

	u32 flagsMask;//flags to wait for

	//override
	virtual void SetWaitingEvents(u32 flagsToWaitFor){
		//It is not allowed to wait on queue for write,
		//because it is always possible to push new message to queue.
		ASSERT((flagsToWaitFor & Waitable::WRITE) == 0)
		
		this->flagsMask = flagsToWaitFor;
	}

	//returns true if signalled
	virtual bool CheckSignalled(){
		return (this->readinessFlags & this->flagsMask) != 0;
	}
#elif defined(__linux__)
	//override
	int GetHandle(){
		return this->eventFD;
	}
#else
	//override
	int GetHandle(){
		//return read end of pipe
		return this->pipeEnds[0];
	}
#endif
};//~class Queue



/**
 * @brief a base class for threads.
 * This class should be used as a base class for thread objects, one should override the
 * Thread::Run() method.
 */
class Thread{
//Tread Run function
#ifdef __WIN32__
	static unsigned int __stdcall RunThread(void *data)
#elif defined(__SYMBIAN32__)
	static TInt RunThread(TAny *data)
#elif defined(M_PTHREAD) //pthread
	static void* RunThread(void *data)
#else
#error "unknown system"
#endif
	{
		ting::Thread *thr = reinterpret_cast<ting::Thread*>(data);
		try{
			thr->Run();
		}catch(ting::Exc& e){
			ASSERT_INFO(false, "uncaught ting::Exc exception in Thread::Run(): " << e.What())
		}catch(std::exception& e){
			ASSERT_INFO(false, "uncaught std::exception exception in Thread::Run(): " << e.what())
		}catch(...){
			ASSERT_INFO(false, "uncaught unknown exception in Thread::Run()")
		}

		{
			//protect by mutex to avoid changing the
			//this->state variable before Join() or Start() has finished.
			ting::Mutex::Guard mutexGuard(Thread::Mutex2());
			
			thr->state = STOPPED;
		}

#ifdef __WIN32__
		//Do nothing, _endthreadex() will be called   automatically
		//upon returning from the thread routine.
#elif defined(M_PTHREAD) //pthread
		pthread_exit(0);
#endif
		return 0;
	}

	static inline ting::Mutex& Mutex1(){
		static ting::Mutex m;
		return m;
	}
	static inline ting::Mutex& Mutex2(){
		static ting::Mutex m;
		return m;
	}

	enum E_State{
		NEW,
		RUNNING,
		STOPPED,
		JOINED
	} state;

	//system dependent handle
#if defined(WIN32)
	HANDLE th;
#elif defined(__SYMBIAN32__)
	RThread th;
#elif defined(M_PTHREAD)
	pthread_t th;
#else
#error "unknown system"
#endif

	//forbid copying
	Thread(const Thread& ){
		ASSERT(false)
	}
	
	Thread& operator=(const Thread& ){
		return *this;
	}

public:
	inline Thread() :
			state(Thread::NEW)
	{
#if defined(__WIN32__)
		this->th = NULL;
#elif defined(__SYMBIAN32__)
		//do nothing
#elif defined(M_PTHREAD)
		//do nothing
#else
#error "unknown system"
#endif
	}

	virtual ~Thread(){
		ASSERT_INFO(
				this->state == JOINED || this->state == NEW,
				"~Thread() destructor is called while the thread was not joined before. "
				<< "Make sure the thread is joined by calling Thread::Join() "
				<< "before destroying the thread object."
			)

		//NOTE: it is incorrect to put this->Join() to this destructor, because
		//thread shall already be stopped at the moment when this destructor
		//is called. If it is not, then the thread will be still running
		//when part of the thread object is already destroyed, since thread object is
		//usually a derived object from Thread class and the destructor of this derived
		//object will be called before ~Thread() destructor.
	}



	/**
	 * @brief This should be overriden, this is what to be run in new thread.
	 * Pure virtual method, it is called in new thread when thread runs.
	 */
	virtual void Run() = 0;



	/**
	 * @brief Start thread execution.
	 * Starts execution of the thread. Thread's Thread::Run() method will
	 * be run as separate thread of execution.
	 * @param stackSize - size of the stack in bytes which should be allocated for this thread.
	 *                    If stackSize is 0 then system default stack size is used.
	 */
	//0 stacksize stands for default stack size (platform dependent)
	void Start(unsigned stackSize = 0){
		//Protect by mutex to avoid several Start() methods to be called
		//by concurrent threads simultaneously and to protect call to Join() before Start()
		//has returned.
		ting::Mutex::Guard mutexGuard1(Thread::Mutex1());
		//Protect by mutex to avoid incorrect state changing in case when thread
		//exits before the Start() method retruned.
		ting::Mutex::Guard mutexGuard2(Thread::Mutex2());

		if(this->state != NEW)
			throw ting::Exc("Thread::Start(): Thread is already running or stopped");

#ifdef __WIN32__
		this->th = reinterpret_cast<HANDLE>(
				_beginthreadex(
						NULL,
						unsigned(stackSize),
						&RunThread,
						reinterpret_cast<void*>(this),
						0,
						NULL
					)
			);
		if(this->th == NULL)
			throw ting::Exc("Thread::Start(): starting thread failed");
#elif defined(__SYMBIAN32__)
		if(this->th.Create(_L("ting thread"), &RunThread,
					stackSize == 0 ? KDefaultStackSize : stackSize,
					NULL, reinterpret_cast<TAny*>(this)) != KErrNone
				)
		{
			throw ting::Exc("Thread::Start(): starting thread failed");
		}
		this->th.Resume();//start the thread execution
#elif defined(M_PTHREAD)
		{
			pthread_attr_t attr;

			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
			pthread_attr_setstacksize(&attr, size_t(stackSize));

			int res = pthread_create(&this->th, &attr, &RunThread, this);
			if(res != 0){
				pthread_attr_destroy(&attr);
				TRACE_AND_LOG(<< "Thread::Start(): pthread_create() failed, error code = " << res
						<< " meaning: " << strerror(res) << std::endl)
				std::stringstream ss;
				ss << "Thread::Start(): starting thread failed,"
						<< " error code = " << res << ": " << strerror(res);
				throw ting::Exc(ss.str().c_str());
			}
			pthread_attr_destroy(&attr);
		}
#else
#error "unknown system"
#endif
		this->state = RUNNING;
	}



	/**
	 * @brief Wait for thread to finish its execution.
	 * This functon waits for the thread finishes its execution,
	 * i.e. until the thread returns from its Thread::Run() method.
	 * Note: it is safe to call Join() on not started threads,
	 *       in that case it will return immediately.
	 */
	void Join(){
//		TRACE(<< "Thread::Join(): enter" << std::endl)

		//protect by mutex to avoid several Join() methods to be called by concurrent threads simultaneously
		ting::Mutex::Guard mutexGuard(Thread::Mutex1());

		if(this->state == NEW){
			//thread was not started, do nothing
			return;
		}

		if(this->state == JOINED){
			throw ting::Exc("Thread::Join(): thread is already joined");
		}

		ASSERT(this->state == RUNNING || this->state == STOPPED)

#ifdef __WIN32__
		WaitForSingleObject(this->th, INFINITE);
		CloseHandle(this->th);
		this->th = NULL;
#elif defined(__SYMBIAN32__)
		TRequestStatus reqStat;
		this->th.Logon(reqStat);
		User::WaitForRequest(reqStat);
		this->th.Close();
#elif defined(M_PTHREAD)
		pthread_join(this->th, 0);
#else
#error "unknown system"
#endif

		//NOTE: at this point the thread's Run() method should already exit and state
		//should be set to STOPPED
		ASSERT(this->state == STOPPED)

		this->state = JOINED;

//		TRACE(<< "Thread::Join(): exit" << std::endl)
	}



	/**
	 * @brief Suspend the thread for a given number of milliseconds.
	 * Suspends the thread which called this function for a given number of milliseconds.
	 * This function guarantees that the calling thread will be suspended for
	 * AT LEAST 'msec' milliseconds.
	 * @param msec - number of milliseconds the thread should be suspended.
	 */
	static void Sleep(unsigned msec = 0){
#ifdef __WIN32__
		SleepEx(DWORD(msec), FALSE);// Sleep() crashes on mingw (I do not know why), this is why I use SleepEx() here.
#elif defined(__SYMBIAN32__)
		User::After(msec * 1000);
#elif defined(M_PTHREAD)
		if(msec == 0){
	#if defined(sun) || defined(__sun)
			sched_yield();
	#else
			pthread_yield();
	#endif
		}else{
			usleep(msec * 1000);
		}
#else
#error "unknown system"
#endif
	}



	/**
	 * @brief get current thread ID.
	 * Returns unique identifier of the currently executing thread. This ID can further be used
	 * to make assertions to make sure that some code is executed in a specific thread. E.g.
	 * assert that methods of some object are executed in the same thread where this object was
	 * creatged.
	 * @return uniqie thread identifier.
	 */
	static inline unsigned GetCurrentThreadID(){
#ifdef __WIN32__
		return unsigned(GetCurrentThreadId());
#elif defined(M_PTHREAD)
		return unsigned(pthread_self());
#else
#error "unknown system"
#endif
	}
};



/**
 * @brief a thread with message queue.
 * This is just a facility class which already contains message queue and boolean 'quit' flag.
 */
class MsgThread : public Thread{
	friend class QuitMessage;
	
protected:
	volatile bool quitFlag;//looks like it is not necessary to protect this flag by mutex, volatile will be enough

	Queue queue;

public:
	MsgThread() :
			quitFlag(false)
	{}

	/**
	 * @brief Send 'Quit' message to thread's queue.
	 */
	inline void PushQuitMessage();//see implementation below



	/**
	 * @brief Send 'no operation' message to thread's queue.
	 */
	inline void PushNopMessage();//see implementation below



	/**
	 * @brief Send a message to thread's queue.
	 * @param msg - a message to send.
	 */
	inline void PushMessage(Ptr<ting::Message> msg){
		this->queue.PushMessage(msg);
	}
};



/**
 * @brief Tells the thread that it should quit its execution.
 * The handler of this message sets the quit flag (Thread::quitFlag)
 * of the thread which this message is sent to.
 */
class QuitMessage : public Message{
	MsgThread *thr;
  public:
	QuitMessage(MsgThread* thread) :
			thr(thread)
	{
		if(!this->thr)
			throw ting::Exc("QuitMessage::QuitMessage(): thread pointer passed is 0");
	}

	//override
	void Handle(){
		this->thr->quitFlag = true;
	}
};



/**
 * @brief do nothing message.
 * The handler of this message dos nothing when the message is handled. This message
 * can be used to unblock thread which is waiting infinitely on its message queue.
 */
class NopMessage : public Message{
  public:
	NopMessage(){}

	//override
	void Handle(){
		//Do nothing, nop
	}
};



inline void MsgThread::PushNopMessage(){
	this->PushMessage(Ptr<Message>(new NopMessage()));
}



inline void MsgThread::PushQuitMessage(){
	//TODO: post preallocated quit message?
	this->PushMessage(Ptr<Message>(new QuitMessage(this)));
}



}//~namespace ting

//NOTE: do not put semicolon after namespace, some compilers issue a warning on this thinking that it is a declaration.



//if Microsoft MSVC compiler, restore warnings state
#ifdef _MSC_VER
#pragma warning(pop) //pop warnings state
#endif


