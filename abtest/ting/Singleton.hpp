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
 * @file Singleton.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Singleton base class (template).
 */

#pragma once

#include "Exc.hpp"
#include "debug.hpp"

namespace ting{

/**
 * @brief singleton base class.
 * This is a basic singleton template.
 * Usage as follows:
 * @code
 *	class MySingleton : public ting::Singleton<MySingleton>{
 *	public:
 *		void DoSomething(){
 *			//...
 *		}
 *  };
 *
 *	int main(int, char**){
 *		MySingleton mySingleton;
 *
 *		MySingleton::Inst().DoSomething();
 *	}
 * @endcode
 */
template <class T> class Singleton{
	
	inline static T*& StaticMemoryBlock(){
		static T* instance = 0;
		return instance;
	}

protected://use only as a base class
	Singleton(){
		if(Singleton::StaticMemoryBlock() != 0)
			throw ting::Exc("Singleton::Singleton(): instance is already created");

		Singleton::StaticMemoryBlock() = static_cast<T*>(this);
	}

private:

	//copying is not allowed
	Singleton(const Singleton&){
		ASSERT(false)
	}
	Singleton& operator=(const Singleton&){
		ASSERT(false)
	}

public:
	/**
	 * @brief tells if singleton object is created or not.
	 * Note, this function is not thread safe.
	 * @return true if object is created.
	 * @return false otherwise.
	 */
	inline static bool IsCreated(){
		return Singleton::StaticMemoryBlock() != 0;
	}

	/**
	 * @brief get singleton instance.
	 * @return reference to singleton object instance.
	 */
	inline static T& Inst(){
		ASSERT_INFO(Singleton::IsCreated(), "Singleton::Inst(): Singleton object is not created")
		return *(Singleton::StaticMemoryBlock());
	}

	~Singleton(){
		ASSERT(Singleton::StaticMemoryBlock() == static_cast<T*>(this))
		Singleton::StaticMemoryBlock() = 0;
	}
};

}//~namespace ting
