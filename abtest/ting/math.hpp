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
 * @file math.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Math utilities.
 */


#pragma once

#include <cmath>
#include <limits> //this is for std::numeric_limits<float>::quiet_NaN(); etc.

namespace ting{

//======================
//Utility math functions
//======================

/**
 * @brief Get sign of a number.
 * Template function which returns the sign of a number.
 * General implementation of this template is as easy as:
 * @code
 * template <typename T_Type> inline T_Type Sign(T_Type n){
 *     return n > 0 ? (1) : (-1);
 * }
 * @endcode
 * @param n - number to get sign of.
 * @return -1 if the argument is negative.
 * @return 1 if the number is positive.
 */
template <typename T_Type> inline T_Type Sign(T_Type n){
	return n < 0 ? (-1) : (1);
}



/**
 * @brief Get absolute value of a number.
 * General implementation of this function is as follows:
 * @code
 * template <typename T_Type> inline T_Type Abs(T_Type n){
 *     return n > 0 ? n : (-n);
 * }
 * @endcode
 * @param n - number to get absolute value of.
 * @return absolute value of the passed argument.
 */
template <typename T_Type> inline T_Type Abs(T_Type n){
	return n < 0 ? (-n) : (n);
}



/**
 * @brief Get number Pi.
 * @return number Pi.
 */
template <typename T> inline T DPi(){
	return T(3.14159265358979323846264338327950288);
}



/**
 * @brief Get 2 * Pi.
 * @return 2 * Pi.
 */
template <typename T> inline T D2Pi(){
	return T(2) * DPi<T>();
}



/**
 * @brief Get natural logarithm of 2, i.e. ln(2).
 * @return natural logarithm of 2.
 */
template <typename T> inline T DLnOf2(){
	return T(0.693147181);
}



/**
 * @brief Get Not-A-Number.
 * @return Not-A-Number value.
 */
template <typename T> inline T DNaN(){
	return std::numeric_limits<T>::quiet_NaN();
}



/**
 * @brief Get infinity value.
 * @return infinity value.
 */
template <typename T> inline T DInf(){
	return std::numeric_limits<T>::infinity();
}



//Power functions

/**
 * @brief Calculate x^2.
 * @param x - value.
 * @return x * x.
 */
template <typename T_Type> inline T_Type Pow2(T_Type x){
	return x * x;
}

/**
 * @brief Calculate x^3.
 */
template <typename T_Type> inline T_Type Pow3(T_Type x){
	return Pow2(x) * x;
}

/**
 * @brief Calculate x^4.
 */
template <typename T_Type> inline T_Type Pow4(T_Type x){
	return Pow2(Pow2(x));
}

/**
 * @brief Calculate x^5.
 */
template <typename T_Type> inline T_Type Pow5(T_Type x){
	return Pow2(x) * Pow3(x);
}

/**
 * @brief Calculates x^p.
 * @param x - value
 * @param p - power
 * @return x^p
 */
template <typename T> inline T Pow(T x, T p){
	return ::pow(x, p);
}

/**
 * @brief Calculate cubic root of a number.
 */
template <typename T_Type> inline T_Type CubicRoot(T_Type x){
	if(x > 0)
		return exp(::log(x) / 3);
	else if(x == 0)
		return 0;
	else
		return -exp(::log(-x) / 3);
}



/**
 * @brief Calculate argument of a complex number.
 * Any complex number
 *     C = x + i * y
 * can be represented in the form
 *     C = |C| * exp(i * arg)
 * where 'arg' is the argument of a complex number.
 * @param x - real part of a complex number.
 * @param y - imaginary part of a complex number.
 * @return argument of a complex number.
 */
template <typename T_Type> inline T_Type Arg(T_Type x, T_Type y){
	T_Type a;
	if(x == 0)
		a = DPi<T_Type>() / 2;
	else if(x > 0)
		a = T_Type(::atan(Abs(y / x)));
	else
		a = DPi<T_Type>() - T_Type(::atan(Abs(y / x)));

	if(y >= 0)
		return a;
	else
		return -a;
}



/**
 * @brief Calculate sine of an angle.
 */
template <typename T> inline T Sin(T x){
	return x.Sin();
}



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline float Sin<float>(float x){
	return ::sin(x);
}
#endif



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline double Sin<double>(double x){
	return ::sin(x);
}
#endif



/*
//TODO:
template <> inline long double Sin<long double>(long double x){
	return ::sin(x);
}
*/



/**
 * @brief Calculate cosine of an angle.
 */
template <typename T> inline T Cos(T x){
	return x.Cos();
}



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline float Cos<float>(float x){
	return ::cos(x);
}
#endif



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline double Cos<double>(double x){
	return ::cos(x);
}
#endif



/*
//TODO:
template <> inline long double Cos<long double>(long double x){
	return ::cos(x);
}
*/



/**
 * @brief Calculate arccosine of a number.
 */
template <typename T> inline T Acos(T x){
	return T(::acos(x));
}



/**
 * @brief Calculate square root of a number.
 */
template <typename T> inline T Sqrt(T x){
	return T(::sqrt(x));
}



/**
 * @brief Calculate e^x.
 */
template <typename T> inline T Exp(T x){
	return x.Exp();
}



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline float Exp<float>(float x){
	return ::exp(x);
}
#endif



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline double Exp<double>(double x){
	return ::exp(x);
}
#endif



/*
//TODO:
template <> inline long double Exp<long double>(long double x){
	return ::exp(x);
}
*/



/**
 * @brief Calculate ln(x).
 * Calculate natural logarithm of x.
 */
template <typename T> inline T Ln(T x){
	return x.Ln();
}



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline float Ln<float>(float x){
	return ::log(x);
}
#endif



#ifndef M_DOC_DONT_EXTRACT //for doxygen
template <> inline double Ln<double>(double x){
	return ::log(x);
}
#endif



}//~namespace ting
