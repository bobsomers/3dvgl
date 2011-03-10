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
 * @file debug.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Debug utilities.
 */

#pragma once

#ifdef __SYMBIAN32__
#else
#include <iostream>
#include <fstream>
#include <typeinfo>
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

//
//
//  Logging definitions
//
//

#ifndef M_DOC_DONT_EXTRACT //for doxygen
namespace ting{
namespace ting_debug{
#ifdef __SYMBIAN32__
#else
inline std::ofstream& DebugLogger(){
	//this allows to make debug output even if main() is not called yet and even if
	//standard std::cout object is not created since static global variables initialization
	//order is undetermined in C++ if these variables are located in separate cpp files!
	static std::ofstream* logger = new std::ofstream("output.log");
	return *logger;
};
#endif
}//~namespace ting_debug
}//~namespace ting
#endif //~M_DOC_DONT_EXTRACT //for doxygen

#ifdef __SYMBIAN32__
#define LOG_ALWAYS(x)
#define TRACE_ALWAYS(x)
#define TRACE_AND_LOG_ALWAYS(x)
#else
#define LOG_ALWAYS(x) ting::ting_debug::DebugLogger() x; ting::ting_debug::DebugLogger().flush();
#define TRACE_ALWAYS(x) std::cout x; std::cout.flush();
#define TRACE_AND_LOG_ALWAYS(x) LOG_ALWAYS(x) TRACE_ALWAYS(x)
#endif

#ifdef DEBUG

#define LOG(x) LOG_ALWAYS(x)
#define TRACE(x) TRACE_ALWAYS(x)
#define TRACE_AND_LOG(x) TRACE_AND_LOG_ALWAYS(x)

#define LOG_IF_TRUE(x, y) if(x){ LOG(y) }

#define DEBUG_CODE(x) x

#else//#ifdef DEBUG

#define LOG(x)
#define TRACE(x)
#define TRACE_AND_LOG(x)
#define LOG_IF_TRUE(x, y)
#define DEBUG_CODE(x)

#endif//~#ifdef DEBUG


//
//
//  Assertion definitions
//
//

#ifdef __SYMBIAN32__

#include <e32std.h>

#define ASSERT_ALWAYS(x) __ASSERT_ALWAYS((x), User::Panic(_L("ASSERTION FAILED!"),3));
#define ASSERT_INFO_ALWAYS(x, y) ASSERT_ALWAYS(x)

#else //Non symbian

#include <cassert>

#define ASSERT_INFO_ALWAYS(x, y) if(!(x)){ \
						LOG_ALWAYS(<< "[!!!fatal] Assertion failed at:\n\t"__FILE__ << ":" << __LINE__ << "| " << y << std::endl) \
						TRACE_ALWAYS(<< "[!!!fatal] Assertion failed at:\n\t"__FILE__ << ":" << __LINE__ << "| " << y << std::endl) \
						assert(false); \
					}
#define ASSERT_ALWAYS(x) ASSERT_INFO_ALWAYS((x), "no additional info")

#endif


#ifdef DEBUG
#define ASSERT_INFO(x, y) ASSERT_INFO_ALWAYS((x), y)
#define ASSERT(x) ASSERT_ALWAYS(x)
#define ASSERT_EXEC(x) ASSERT(x)
namespace ting{
namespace ting_debug{
inline void LogAssert(const char* file, int line){
	LOG_ALWAYS(<< "[!!!fatal] Assertion failed at:\n\t"<< file << ":" << line << "| ASS() assertion macro" << std::endl) \
};
}
}

#ifdef __SYMBIAN32__
#define ASS(x) (x)
#define ASSCOND(x, cond) (x)
#else
#define ASS(x) ( (x) ? (x) : (ting::ting_debug::LogAssert(__FILE__, __LINE__), (assert(false)), (x)) )
#define ASSCOND(x, cond) ( ((x) cond) ? (x) : (ting::ting_debug::LogAssert(__FILE__, __LINE__), (assert(false)), (x)) )
#endif

#else
#define ASSERT_INFO(x, y)
#define ASSERT(x)
#define ASSERT_EXEC(x) x;
#define ASS(x) (x)
#define ASSCOND(x, cond) (x)
#endif//~#ifdef DEBUG

//==================
//=  Static assert =
//==================
#ifndef M_DOC_DONT_EXTRACT //for doxygen
namespace ting{
namespace ting_debug{
template <bool b> struct C_StaticAssert{
	virtual void STATIC_ASSERTION_FAILED() = 0;
	virtual ~C_StaticAssert(){};
};
template <> struct C_StaticAssert<true>{};
}//~namespace ting_debug
}//~namespace ting
#define M_STATIC_ASSERT_II(x, l, c) struct C_StaticAssertInst_##l##_##c{ \
	ting::ting_debug::C_StaticAssert<x> STATIC_ASSERTION_FAILED; \
};
#define M_STATIC_ASSERT_I(x, l, c) M_STATIC_ASSERT_II(x, l, c)
#endif //~M_DOC_DONT_EXTRACT //for doxygen

#if defined(__GNUG__) || (_MSC_VER >= 7100) //__COUNTER__ macro is only supported in these compilers
#define STATIC_ASSERT(x) M_STATIC_ASSERT_I(x, __LINE__, __COUNTER__)
#else
#define STATIC_ASSERT(x)
#endif

