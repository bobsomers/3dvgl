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
 * @file Signal.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Templates to implement signal-slot callback technique.
 */

#pragma once

#include <vector>
#include "codegen.h"
#include "Ref.hpp"
#include "Thread.hpp"
#include "debug.hpp"


#ifndef M_MAX_NUM_SIG_PARAMETERS
#define M_MAX_NUM_SIG_PARAMETERS 10
#endif

//output ", class T_Pn", the comma is written only if n==0
#define M_TEMPLATE_PARAM(n, a) M_COMMA_IF_NOT_0(n) class T_P##n

//output "class T_P0, ... class T_Pn"
#define M_TEMPLATE_PARAMS(n) M_REPEAT1(n, M_TEMPLATE_PARAM, )

//output "template <class T_P0, ... class T_Pn>"
#define M_TEMPLATE(n) M_IF_NOT_0(n, template <, ) M_TEMPLATE_PARAMS(n) M_IF_NOT_0(n, >, )



//output ", TPn pn", the comma is written only if n==0
#define M_FUNC_PARAM_FULL(n, a) M_COMMA_IF_NOT_0(n) T_P##n p##n

//output "T_P0 p0, ... T_Pn pn"
#define M_FUNC_PARAMS_FULL(n) M_REPEAT1(n, M_FUNC_PARAM_FULL, )


#define M_FUNC_PARAM_TYPE(n, a) M_COMMA_IF_NOT_0(n) T_P##n

//output "T_P0, ... T_Pn"
#define M_FUNC_PARAM_TYPES(n) M_REPEAT1(n, M_FUNC_PARAM_TYPE, )
//#define M_FUNC_PARAM_TYPES(n) M_FUNC_PARAM_TYPES_I(n)

//output ", pn", the comma is written only if n==0
#define M_FUNC_PARAM_NAME(n, a) M_COMMA_IF_NOT_0(n) p##n

//output "p0, ... pn"
#define M_FUNC_PARAM_NAMES(n) M_REPEAT1(n, M_FUNC_PARAM_NAME, )


#define M_FUNC_SLOT(num_func_params, num_sig_params) \
template <class T_Ret> class FuncSlot##num_func_params : public SlotLink{ \
public: \
	T_Ret(*f)(M_FUNC_PARAM_TYPES(num_func_params)); \
\
	FuncSlot##num_func_params(T_Ret(*function)(M_FUNC_PARAM_TYPES(num_func_params))) : \
			f(function) \
	{} \
	virtual bool Execute(M_FUNC_PARAMS_FULL(num_sig_params)){ \
		(*this->f)(M_FUNC_PARAM_NAMES(num_func_params)); \
		return false; \
	} \
};


//output the template for method slot with n parameters
#define M_METHOD_SLOT(num_meth_params, num_sig_params) \
template <class T_Ob, class T_Ret> class MethodSlot##num_meth_params : public SlotLink{ \
public: \
	T_Ob* o; \
	T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params)); \
\
	MethodSlot##num_meth_params(T_Ob* object, T_Ret(T_Ob::*method)(M_FUNC_PARAM_TYPES(num_meth_params))) : \
			o(object), \
			m(method) \
	{} \
	virtual bool Execute(M_FUNC_PARAMS_FULL(num_sig_params)){ \
		(this->o->*(this->m))(M_FUNC_PARAM_NAMES(num_meth_params)); \
		return false; \
	} \
};
//#define M_METHOD_SLOT(num_meth_params, num_sig_params) M_METHOD_SLOT_I(num_meth_params, num_sig_params)

//output the template for method slot for WeakRef object with n parameters
#define M_METHOD_WEAKREF_SLOT(num_meth_params, num_sig_params) \
template <class T_Ob, class T_Ret> class WeakRefMethodSlot##num_meth_params : public SlotLink{ \
public: \
	WeakRef<T_Ob> o; \
	T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params)); \
\
	WeakRefMethodSlot##num_meth_params(WeakRef<T_Ob>& object, T_Ret(T_Ob::*method)(M_FUNC_PARAM_TYPES(num_meth_params))) : \
			o(object), \
			m(method) \
	{} \
	virtual bool Execute(M_FUNC_PARAMS_FULL(num_sig_params)){ \
		if(Ref<T_Ob> r = this->o){ \
			(r.operator->()->*(this->m))(M_FUNC_PARAM_NAMES(num_meth_params)); \
			return false; \
		}else{\
			return true; \
		} \
	} \
\
	virtual bool IsAlive(){ \
		return !this->o.IsSurelyInvalid(); \
	} \
};


//func Connect
#define M_CONNECT_FUNC(num_func_params, unused) \
template <class T_Ret> void Connect(T_Ret(*f)(M_FUNC_PARAM_TYPES(num_func_params))){ \
	ASSERT(f) \
	Ptr<SlotLink> sl( \
			static_cast<SlotLink*>( new FuncSlot##num_func_params<T_Ret>(f)) \
		); \
	ting::Mutex::Guard mutexGuard(this->mutex); \
	this->slotLink.push_back(sl); \
}

//method and pointer to object Connect
#define M_CONNECT_METH(num_meth_params, unused) \
template <class T_Ob, class T_Ret> void Connect(T_Ob* o, T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params))){ \
	ASSERT(o) \
	ASSERT(m) \
	Ptr<SlotLink> sl( \
			static_cast<SlotLink*>(new MethodSlot##num_meth_params<T_Ob, T_Ret>(o, m)) \
		); \
	ting::Mutex::Guard mutexGuard(this->mutex); \
	this->slotLink.push_back(sl); \
}

//Weak ref and method Connect
#define M_CONNECT_METH_WEAKREF(num_meth_params, unused) \
template <class T_Ob, class T_Ret> void Connect(WeakRef<T_Ob> o, T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params))){ \
	ASSERT(m) \
	Ptr<SlotLink> sl( \
			static_cast<SlotLink*>(new WeakRefMethodSlot##num_meth_params<T_Ob, T_Ret>(o, m)) \
		); \
	ting::Mutex::Guard mutexGuard(this->mutex); \
	this->slotLink.push_back(sl); \
\
	for(T_SlotLinkIter i = this->slotLink.begin(); i != this->slotLink.end();){ \
		if(!(*i)->IsAlive()){ \
			i = this->slotLink.erase(i); \
		}else{ \
			++i; \
		} \
	} \
}



//Search for existing connection to a given function
#define M_SEARCH_FUNCSLOT(num_func_params, num_sig_params) \
template <class T_Ret> T_SlotLinkIter SearchFuncSlot(T_Ret(*f)(M_FUNC_PARAM_TYPES(num_func_params))){ \
	ASSERT(f) \
	for(T_SlotLinkIter i = this->slotLink.begin(); i != this->slotLink.end(); ++i){ \
		FuncSlot##num_func_params<T_Ret> *slot = \
				dynamic_cast<FuncSlot##num_func_params<T_Ret>* >( \
						(*i).operator->() \
					) \
			; \
		if(slot){ \
			if(slot->f == f){ \
				return i; \
			} \
		} \
	} \
	return this->slotLink.end(); \
}

//Search for existing connection to a given object-method slot
#define M_SEARCH_METHSLOT(num_meth_params, unused) \
template <class T_Ob, class T_Ret> T_SlotLinkIter SearchMethSlot( \
		T_Ob* o, \
		T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params)) \
	) \
{ \
	ASSERT(o) \
	ASSERT(m) \
	for(T_SlotLinkIter i = this->slotLink.begin(); i != this->slotLink.end(); ++i){ \
		MethodSlot##num_meth_params<T_Ob, T_Ret> *slot = \
				dynamic_cast<MethodSlot##num_meth_params<T_Ob, T_Ret>* >( \
						(*i).operator->() \
					) \
			; \
		if(slot){ \
			if(slot->m == m && slot->o == o){ \
				return i; \
			} \
		} \
	} \
	return this->slotLink.end(); \
}

//Search for existing connection to a given WeakRef_object-method slot
#define M_SEARCH_METHSLOT_WEAKREF(num_meth_params, unused) \
template <class T_Ob, class T_Ret> T_SlotLinkIter SearchWeakRefMethSlot( \
		ting::WeakRef<T_Ob>& o, \
		T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params)) \
	) \
{ \
	ASSERT(m) \
	ting::Ref<T_Ob> ho(o); \
	for(T_SlotLinkIter i = this->slotLink.begin(); i != this->slotLink.end(); ++i){ \
		WeakRefMethodSlot##num_meth_params<T_Ob, T_Ret> *slot = \
				dynamic_cast<WeakRefMethodSlot##num_meth_params<T_Ob, T_Ret>* >( \
						(*i).operator->() \
					) \
			; \
		if(slot){ \
			ting::Ref<T_Ob> hso(slot->o); \
			if(slot->m == m && ho == hso){ \
				return i; \
			} \
		} \
	} \
	return this->slotLink.end(); \
}



//Disconnect function slot
#define M_DISCONNECT_FUNC(num_func_params, unused) \
template <class T_Ret> bool Disconnect(T_Ret(*f)(M_FUNC_PARAM_TYPES(num_func_params))){ \
	ASSERT(f) \
	ting::Mutex::Guard mutexGuard(this->mutex); \
	T_SlotLinkIter i = this->SearchFuncSlot(f); \
	if(i != this->slotLink.end()){ \
		this->slotLink.erase(i); \
		return true; \
	} \
	return false; \
}

//Disconnect object-method slot
#define M_DISCONNECT_METH(num_meth_params, unused) \
template <class T_Ob, class T_Ret> bool Disconnect( \
		T_Ob* o, \
		T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params)) \
	) \
{ \
	ASSERT(o) \
	ASSERT(m) \
	ting::Mutex::Guard mutexGuard(this->mutex); \
	T_SlotLinkIter i = this->SearchMethSlot(o, m); \
	if(i != this->slotLink.end()){ \
		this->slotLink.erase(i); \
		return true; \
	} \
	return false; \
}

//Disconnect WeakRef_object-method slot
#define M_DISCONNECT_METH_WEAKREF(num_meth_params, unused) \
template <class T_Ob, class T_Ret> bool Disconnect( \
		ting::WeakRef<T_Ob>& o, \
		T_Ret(T_Ob::*m)(M_FUNC_PARAM_TYPES(num_meth_params)) \
	) \
{ \
	ASSERT(m) \
	ting::Mutex::Guard mutexGuard(this->mutex); \
	T_SlotLinkIter i = this->SearchWeakRefMethSlot(o, m); \
	if(i != this->slotLink.end()){ \
		this->slotLink.erase(i); \
		return true; \
	} \
	return false; \
}



//TODO:
//  - add IsConnected(obj, method) family of functions

//
// M   M        SSSS IIIII  GGG  N   N  AAA  L
// MM MM       S       I   G     NN  N A   A L
// M M M        SSS    I   G  GG N N N A   A L
// M   M           S   I   G   G N  NN AAAAA L
// M   M _____ SSSS  IIIII  GGG  N   N A   A LLLLL
//
//output a template for a signal with n parameters
#define M_SIGNAL(n, unused) \
M_TEMPLATE(n) class Signal##n{ \
	class SlotLink{ \
	public: \
		virtual ~SlotLink(){} \
		virtual bool Execute(M_FUNC_PARAMS_FULL(n)) = 0; \
		virtual bool IsAlive(){ \
			return true; \
		} \
	}; \
\
	M_REPEAT2(M_INCREMENT(n), M_METHOD_SLOT, n) \
	M_REPEAT2(M_INCREMENT(n), M_FUNC_SLOT, n) \
	M_REPEAT2(M_INCREMENT(n), M_METHOD_WEAKREF_SLOT, n) \
\
	typedef std::vector<Ptr<SlotLink> > T_SlotLinkList; \
	typedef M_IF_NOT_0(n, typename, ) T_SlotLinkList::iterator T_SlotLinkIter; \
	T_SlotLinkList slotLink; \
\
	M_REPEAT2(M_INCREMENT(n), M_SEARCH_FUNCSLOT, n) \
	M_REPEAT2(M_INCREMENT(n), M_SEARCH_METHSLOT, n) \
	M_REPEAT2(M_INCREMENT(n), M_SEARCH_METHSLOT_WEAKREF, n) \
\
	ting::Mutex mutex; \
\
public: \
	void Emit(M_FUNC_PARAMS_FULL(n)){ \
		ting::Mutex::Guard mutexGuard(this->mutex); \
		for(T_SlotLinkIter i = this->slotLink.begin(); i != this->slotLink.end();){ \
			if((*i)->Execute(M_FUNC_PARAM_NAMES(n))){ \
				i = this->slotLink.erase(i); \
			}else{ \
				++i; \
			} \
		} \
	} \
\
	M_REPEAT2(M_INCREMENT(n), M_CONNECT_FUNC, ) \
	M_REPEAT2(M_INCREMENT(n), M_CONNECT_METH, ) \
	M_REPEAT2(M_INCREMENT(n), M_CONNECT_METH_WEAKREF, ) \
\
	M_REPEAT2(M_INCREMENT(n), M_DISCONNECT_FUNC, ) \
	M_REPEAT2(M_INCREMENT(n), M_DISCONNECT_METH, ) \
	M_REPEAT2(M_INCREMENT(n), M_DISCONNECT_METH_WEAKREF, ) \
\
	void DisconnectAll(){ \
		ting::Mutex::Guard mutexGuard(this->mutex); \
		this->slotLink.clear(); \
	} \
\
	inline unsigned NumConnections()const{ \
		return this->slotLink.size(); \
	} \
};



namespace ting{

M_REPEAT3(M_MAX_NUM_SIG_PARAMETERS, M_SIGNAL, )

}//~namespace ting
