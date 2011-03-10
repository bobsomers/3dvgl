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
 * @file utils.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Utility functions and classes.
 */

#pragma once

//#ifdef _MSC_VER //If Microsoft C++ compiler
//#pragma warning(disable:4290)
//#endif

#include <vector>

#include "debug.hpp" //debugging facilities
#include "types.hpp"
#include "Thread.hpp"

//define macro used to align structures in memory
#ifdef _MSC_VER //If Microsoft C++ compiler
#define M_DECLARE_ALIGNED(x)
#define M_DECLARE_ALIGNED_MSVC(x) __declspec(align(x))

#elif defined(__GNUG__)//GNU g++ compiler
#define M_DECLARE_ALIGNED(x) __attribute__ ((aligned(x)))
#define M_DECLARE_ALIGNED_MSVC(x)

#else
#error "unknown compiler"
#endif


namespace ting{



/**
 * @brief Clamp value top.
 * This inline template function can be used to clamp the top of the value.
 * Example:
 * @code
 * int a = 30;
 *
 * //Clamp to 40. Value of 'a' remains unchanged,
 * //since it is already less than 40.
 * ting::ClampTop(a, 40);
 * std::cout << a << std::endl;
 *
 * //Clamp to 27. Value of 'a' is changed to 27,
 * //since it is 30 which is greater than 27.
 * ting::ClampTop(a, 27);
 * std::cout << a << std::endl;
 * @endcode
 * As a result, this will print:
 * @code
 * 30
 * 27
 * @endcode
 * @param v - reference to the value which top is to be clamped.
 * @param top - value to clamp the top to.
 */
template <class T> inline void ClampTop(T& v, const T top){
	if(v > top){
		v = top;
	}
}


/**
 * @brief Clamp value bottom.
 * Usage is analogous to ting::ClampTop.
 * @param v - reference to the value which bottom is to be clamped.
 * @param bottom - value to clamp the bottom to.
 */
template <class T> inline void ClampBottom(T& v, const T bottom){
	if(v < bottom){
		v = bottom;
	}
}



/**
 * @brief serialize 16 bit value.
 * Serialize 16 bit value, less significant byte first.
 * @param value - the value.
 * @param out_buf - pointer to the 2 byte buffer where the result will be placed.
 */
inline void Serialize16(u16 value, u8* out_buf){
	out_buf[0] = value & 0xff;
	out_buf[1] = value >> 8;

	//NOTE: the following method of serializing is incorrect because,
	//	    according to C++ standard, it is undefined behavior.
	//      On most compilers it will work, but, to be on a safe side, we do not use it.
	//*reinterpret_cast<u16*>(out_buf) = value;//assume little-endian
}



/**
 * @brief serialize 32 bit value.
 * Serialize 32 bit value, less significant byte first.
 * @param value - the value.
 * @param out_buf - pointer to the 4 byte buffer where the result will be placed.
 */
inline void Serialize32(u32 value, u8* out_buf){
	*out_buf = u8(value & 0xff);
	++out_buf;
	*out_buf = u8((value >> 8) & 0xff);
	++out_buf;
	*out_buf = u8((value >> 16) & 0xff);
	++out_buf;
	*out_buf = u8((value >> 24) & 0xff);

	//NOTE: the following method of serializing is incorrect because,
	//	    according to C++ standard, it is undefined behavior.
	//      On most compilers it will work, but, to be on a safe side, we do not use it.
	//*reinterpret_cast<u32*>(out_buf) = value;//assume little-endian
}



/**
 * @brief deserialize 16 bit value.
 * Deserialize 16 bit value from the sequence of bytes. Assume that less significant
 * byte goes first in the input byte sequence.
 * @param buf - pointer to buffer containig 2 bytes to convert from network format.
 * @return 16 bit unsigned integer converted from network byte order to native byte order.
 */
inline u16 Deserialize16(const u8* buf){
	u16 ret;

	//assume little-endian
	ret = u16(*buf);
	++buf;
	ret |= (u16(*buf)) << 8;

	return ret;

	//NOTE: the following method of serializing is incorrect because,
	//	    according to C++ standard, it is undefined behavior.
	//      On most compilers it will work, but, to be on a safe side, we do not use it.
	//return *reinterpret_cast<const u16*>(buf);//assume little-endian
}



/**
 * @brief deserialize 32 bit value.
 * Deserialize 32 bit value from the sequence of bytes. Assume that less significant
 * byte goes first in the input byte sequence.
 * @param buf - pointer to buffer containig 4 bytes to convert from network format.
 * @return 32 bit unsigned integer converted from network byte order to native byte order.
 */
inline u32 Deserialize32(const u8* buf){
	u32 ret;

	//assume little-endian
	ret = u32(*buf);
	++buf;
	ret |= (u32(*buf)) << 8;
	++buf;
	ret |= (u32(*buf)) << 16;
	++buf;
	ret |= (u32(*buf)) << 24;

	return ret;

	//NOTE: the following method of serializing is incorrect because,
	//	    according to C++ standard, it is undefined behavior.
	//      On most compilers it will work, but, to be on a safe side, we do not use it.
	//return *reinterpret_cast<const u32*>(buf);//assume little-endian
}



/**
 * @brief Maximal value of unsigned integer type.
 * @return Maximal value an unsigned integer type can represent on the current platform.
 */
inline unsigned DMaxUint(){
	return unsigned(-1);
}



/**
 * @brief Maximal value of integer type.
 * @return Maximal value an integer type can represent on the current platform.
 */
inline int DMaxInt(){
	return int(DMaxUint() >> 1);
}



/**
 * @brief Minimal value of integer type.
 * @return Minimal value an integer type can represent on the current platform.
 */
inline int DMinInt(){
	return ~DMaxInt();
}



}//~namespace ting

