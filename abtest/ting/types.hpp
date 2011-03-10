/* The MIT License:

Copyright (c) 2008-2010 Ivan Gagis

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
 * @file types.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief General types definitions.
 */

#pragma once

#include "debug.hpp"

namespace ting{

/**
 * @brief Unsigned 8 bit type.
 */
typedef unsigned char u8;

/**
 * @brief Signed 8 bit type.
 */
typedef signed char s8;

/**
 * @brief Unsigned 16 bit type.
 */
typedef unsigned short int u16;

/**
 * @brief Signed 16 bit type.
 */
typedef signed short int s16;

/**
 * @brief Unsigned 32 bit type.
 */
typedef unsigned int u32;

/**
 * @brief Signed 32 bit type.
 */
typedef signed int s32;

/**
 * @brief Unsigned 64 bit type.
 */
typedef unsigned long long int u64;

/**
 * @brief Signed 64 bit type.
 */
typedef long long int s64;


#ifndef M_DOC_DONT_EXTRACT //for doxygen

STATIC_ASSERT(u8(-1) == 0xff)//assert that byte consists of exactly 8 bits, e.g. some systems have 10 bits per byte!!!
STATIC_ASSERT(sizeof(u8) == 1)
STATIC_ASSERT(sizeof(s8) == 1)
STATIC_ASSERT(sizeof(u16) == 2)
STATIC_ASSERT(sizeof(s16) == 2)
STATIC_ASSERT(sizeof(u32) == 4)
STATIC_ASSERT(sizeof(s32) == 4)
STATIC_ASSERT(sizeof(u64) == 8)
STATIC_ASSERT(u64(-1) == 0xffffffffffffffffLL)
STATIC_ASSERT(sizeof(s64) == 8)

#endif //~M_DOC_DONT_EXTRACT //for doxygen



/**
 * @brief Thin wrapper above bool C++ built-in type.
 * Thin wrapper above bool type which allows declaration
 * of a bool type variable initialized to some value which is defined by template
 * parameter. This is useful when declaring class members of type bool, so you can
 * indicate to which value the variable should be initialized without using
 * constructor initialization list or assigning the value in the constructor body.
 * Auto-conversions to/from C++ built-in bool type are supported.
 *
 * Typical usage:
 * @code
 * #include <ting/types.hpp>
 *
 * //...
 *
 * class Widget{
 *     ting::Bool<false> isHidden; //initialized to false upon object construction
 * public:
 *     void Draw(){
 *         if(this->isHidden)
 *             return;
 *
 *         //object is visible, draw it on the screen
 *         //...
 *
 *     }
 *
 *     inline void SetHidden(bool hide){
 *         this->isHidden = hide;
 *     }
 *
 *     inline bool IsHidden()const{
 *         return this->isHidden;
 *     }
 * }
 * @endcode
 */
template <bool Value> class Bool{
	bool value;
public:
	inline Bool(bool value = Value) :
			value(value)
	{}

	inline operator bool()const{
		return this->value;
	}
};



/**
 * @brief Thin wrapper above any C++ built-in type allowing initialization from int.
 * Thin wrapper above any C++ built-in type which allows initialization from C++ int type.
 * This wrapper allows initialize the variable to some int value right
 * in place of declaration.
 * This is useful when declaring class members, so you can
 * indicate to which value the variable should be initialized without using
 * constructor initialization list or assigning the value in the constructor body.
 * Auto-conversions to/from the original type are supported.
 *
 * Typical usage:
 * @code
 * #include <ting/types.hpp>
 *
 * //...
 *
 * class SampleInitedRect{
 * public:
 *     ting::Inited<float, 0> x; //initialized to 0 upon object construction
 *     ting::Inited<float, -10> y; //initialized to -10 upon object construction
 *     ting::Inited<float, 10> width; //initialized to 10 upon object construction
 *     ting::Inited<float, 20> height; //initialized to 20 upon object construction
 * }
 * @endcode
 */
template <class T, int V> class Inited{
	T value;
public:
	inline Inited() :
			value(T(V))
	{}

	inline Inited(T value) :
			value(value)
	{}

	inline T& operator=(const T& val){
		return this->value = val;
	}

	inline operator T()const{
		return this->value;
	}

	inline T& operator+=(const T& val){
		return (this->value += val);
	}

	inline T& operator-=(const T& val){
		return (this->value -= val);
	}

	inline T& operator*=(const T& val){
		return (this->value *= val);
	}

	inline T& operator/=(const T& val){
		return (this->value /= val);
	}

	inline T& operator%=(const T& val){
		return (this->value %= val);
	}

	inline T& operator^=(const T& val){
		return (this->value ^= val);
	}

	inline T& operator&=(const T& val){
		return (this->value &= val);
	}

	inline T& operator|=(const T& val){
		return (this->value |= val);
	}

	//prefix increment
	inline T& operator++(){
		return (++this->value);
	}

	//postfix increment
	inline T operator++(int){
		return (this->value++);
	}

	//prefix decrement
	inline T& operator--(){
		return (--this->value);
	}

	//postfix decrement
	inline T operator--(int){
		return (this->value--);
	}

	inline T* operator&(){
		return &this->value;
	}

	inline const T* operator&()const{
		return &this->value;
	}
};



}//~namespace ting
