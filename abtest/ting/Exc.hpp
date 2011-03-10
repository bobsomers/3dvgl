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
 * @file Exc.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Basic Exception class.
 */

#pragma once

#include <string.h>
#include <exception>
#include <new>                    //for std::nothrow

namespace ting{

/**
 * @brief Basic exception class
 */
#ifdef __SYMBIAN32__ // we have symbian which does not have std c++ library
class Exc
#else
class Exc : public std::exception
#endif
{
	char *msg;
public:
	/**
	 * @brief Exception constructor.
	 * @param message - Pointer to the exception message null-terminated string.
	 *                  Constructor will copy the string into objects internal memory buffer.
	 *                  It is legal to supply 0.
	 */
	Exc(const char* message = 0){
		if(!message)
			message = "no exception info";

		size_t len = strlen(message);

	#ifdef __SYMBIAN32__
		//if I'm right in symbian simple new operator does not throw or leave, it will return 0 in case of error
		this->msg = new char[len+1];
	#else
		//we do not want another exception, use std::nothrow
		this->msg = new(std::nothrow) char[len+1];
	#endif
		if(!this->msg)
			return;

		memcpy(this->msg, message, len);
		this->msg[len] = 0;//null-terminate
	}



	virtual ~Exc()throw(){//use throw() because base class (std::exception) uses it.
		delete[] this->msg;
	}



	/**
	 * @brief Returns a pointer to exception message.
	 * @return a pointer to objects internal memory buffer holding
	 *         the exception message null-terminated string.
	 *         Note, that after the exception object is destroyed
	 *         the pointer returned by this method become invalid.
	 */
	inline const char *What()const{
		return this->what();
	}



private:
	//override from std::exception
	const char *what()const throw(){//use throw() because base class (std::exception) uses it.
		return this->msg;//this->msg is never 0 (see Exc constructor for more info).
	}
};

}//~namespace ting
