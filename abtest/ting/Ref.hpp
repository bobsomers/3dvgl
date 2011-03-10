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
 * @file Ref.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Reference counting mechanism base classes.
 */

#pragma once

#include "debug.hpp"
#include "types.hpp"
#include "Thread.hpp"
#include "PoolStored.hpp"

//#define M_ENABLE_REF_PRINT
#ifdef M_ENABLE_REF_PRINT
#define M_REF_PRINT(x) TRACE(<<"[REF]" x)
#else
#define M_REF_PRINT(x)
#endif

namespace ting{

template <class T> class Ref;//forward declaration
template <class T> class WeakRef;//forward declaration

/**
 * @brief base class for a reference counted object.
 * All object which are supposed to be used with reference counted pointers (ting::Ref)
 * should be derived from ting::RefCounted.
 * Typical usage:
 * @code
 *	class Test : public ting::RefCounted{
 *		Test(int theA, int theB) :
 *				a(theA),
 *				b(theB)
 *		{}
 *	public:
 *		int a;
 *		int b;
 *
 *		static inline ting::Ref<Test> New(int theA, int theB){
 *			return ting::Ref<Test>(new Test(theA, theB));
 *		}
 *	}
 *
 *	//...
 *
 *	{
 *		ting::Ref<Test> t = Test::New(10, -13);
 * 
 *		t->a = 23;
 *		//...
 *
 *		ting::Ref<Test> t2 = t;
 *		t2->b = 45;
 *
 *		//...
 *
 *	}//the object will be destroyed here
 * @endcode
 *
 * Note, the constructor of class Test in this example was done private and static method New() introduced
 * to construct objects of class Test. It is recommended to do this for all ting::RefCounted
 * objects because they are not supposed to be accessed via ordinary pointers, only via ting::Ref.
 * It is a good practice and it will make your code less error prone.
 */
class RefCounted{
	template <class T> friend class Ref;
	template <class T> friend class WeakRef;

	
private:
	//NOTE: reference may be added to a constant instance of the object, this is
	//why this method should be const.
	inline unsigned AddRef(){
		ASSERT(this->counter)
		M_REF_PRINT(<< "RefCounted::AddRef(): invoked, old numStrongRefs = " << (this->counter->numStrongRefs) << std::endl)
		Mutex::Guard mutexGuard(this->counter->mutex);
		M_REF_PRINT(<< "RefCounted::AddRef(): mutex locked " << std::endl)
		return ++(this->counter->numStrongRefs);
	}



	//NOTE: reference may be removed from a constant instance of the object, this is
	//why this method should be const.
	inline unsigned RemRef()const{
		ASSERT(this->counter)
		M_REF_PRINT(<< "RefCounted::RemRef(): invoked, old numStrongRefs = " << (this->counter->numStrongRefs) << std::endl)
		this->counter->mutex.Lock();
		M_REF_PRINT(<< "RefCounted::RemRef(): mutex locked" << std::endl)
		ASSERT(this->counter->numStrongRefs != 0)//if someone has called RemRef() then there should be at least 1 strong reference
		M_REF_PRINT(<< "RefCounted::RemRef(): decrementing" << std::endl)
		unsigned n = --(this->counter->numStrongRefs);
		M_REF_PRINT(<< "RefCounted::RemRef(): decremented, new numStrongRefs = " << (this->counter->numStrongRefs) << std::endl)

		if(n == 0){//if no more strong references to the RefCounted
			if(this->counter->numWeakRefs > 0){
				//there are weak references, they will now own the Counter object,
				//therefore, do not delete Counter, just clear the pointer to RefCounted.

				//Zero the pointer to object in the counter. This will indicate to WeakRef that the
				//object is not existing and the Counter object is owned by WeakRefs now.
				//It is possible that there are no strong refs to the object but the object is still
				//existing and the counter object is owned by object, not by WeakRefs. This happens when
				//object was just created and no strong refs were created to the object, in that case,
				//it is possible that WeakRef will be created and destroyed before any strong refs
				//are created.
				this->counter->p = 0;

				ASS(this->counter)->mutex.Unlock();
				const_cast<RefCounted*>(this)->counter = 0;//zero the pointer to counter to prevent it from deleting in destructor
			}else{//no weak references
				//Unlock before deleting because the mutex object is in Counter.
				//The Counter object will be deleted in destructor, as the destructor will be called shortly
				//because number of strong references reached 0.
				this->counter->mutex.Unlock();
				M_REF_PRINT(<< "RefCounted::RemRef(): mutex unlocked" << std::endl)
			}
			return 0;//zero strong references left
		}
		ASS(this->counter)->mutex.Unlock();
		M_REF_PRINT(<< "RefCounted::RemRef(): mutex unlocked" << std::endl)

		return n;
	}



	struct Counter : public PoolStored<Counter>{
		RefCounted *p;
		Mutex mutex;
		unsigned numStrongRefs;
		unsigned numWeakRefs;
		inline Counter(RefCounted *ptr) :
				p(ptr),
				numStrongRefs(0),
				numWeakRefs(0)
		{
			M_REF_PRINT(<< "Counter::Counter(): counter object created" << std::endl)
		}

		inline ~Counter(){
			M_REF_PRINT(<< "Counter::~Counter(): counter object destroyed" << std::endl)
		}
	};



	Counter *counter;



protected:
	//only base classes can construct this class
	//i.e. use of this class is allowed only as a base class
	inline RefCounted(){
		//NOTE: do not create Counter object in RefCounted constructor
		//      initializer list because MSVC complains about usage of "this"
		//      keyword in initializer list.
		this->counter = new Counter(this);
		ASSERT(this->counter)
		
		ASSERT_INFO(
				this->counter->p == this,
				"this->counter->p = " << this->counter->p << " this = " << this
			)
	}



	//operator new is protected to enforce objects creation
	//through static New() method, to avoid using of RefCounted objects
	//via ordinary pointers.
	inline static void* operator new(size_t s){
		return ::operator new(s);
	}

protected:
	//operator delete is protected because only ting::Ref can delete the
	//RefCounted object.
	inline static void operator delete(void *p){
		::operator delete(p);
	}


public:
	//destructor shall be virtual!!!
	virtual ~RefCounted(){
		//Pointer to counter object can be 0 if there were weak references left upon the
		//moment of the object destruction. So, check for 0.
		if(this->counter){
			delete this->counter;
		}
	}

public:
	inline unsigned NumRefs()const{
		return ASS(this->counter)->numStrongRefs;
	}

private:
	//copy constructor is private, no copying
	inline RefCounted(const RefCounted&);
	inline RefCounted& operator=(const RefCounted&);
};//~class RefCounted



/**
 * @brief Reference to a reference counted object.
 * Pointer (reference) to a reference counted object.
 * As soon as there is at least one ting::Ref object pointing to some reference counted
 * object, this object will be existing. As soon as all ting::Ref objects cease to exist
 * (going out of scope) the reference counted object they are pointing ti will be deleted.
 */
//T should be RefCounted!!!
template <class T> class Ref{
	friend class WeakRef<T>;
	template <class TS> friend class Ref;

	T *p;


	
public:
	/**
	 * @brief cast statically to another class.
	 * Performs standard C++ static_cast().
	 * @return reference to object of casted class.
	 */
	template <class TS> inline Ref<TS> StaticCast()const{
		return Ref<TS>(static_cast<TS*>(this->operator->()));
	}



	/**
	 * @brief cast dynamically.
	 * Performs standard C++ dynamic_cast() operation.
	 * @return valid reference to object of casted class if dynamic_cast() succeeds, i.e. if the
	 *         object can be cast to requested class.
	 * @return invalid reference otherwise, i. e. if the object cannot be cast to requested class.
	 */
	template <class TS> inline const Ref<TS> DynamicCast()const{
		const TS* t = dynamic_cast<const TS*>(this->operator->());
		if(t)
			return Ref<TS>(const_cast<TS*>(t));
		else
			return Ref<TS>();
	}



	/**
	 * @brief default constructor.
	 * @param v - this parameter is ignored. Its intention is just to make possible
	 *            auto-conversion form int to invalid reference. This allows writing
	 *            simply 'return 0;' in functions to return invalid reference.
	 *            Note, any integer passed (even not 0) will result in invalid reference.
	 */
	//NOTE: the int argument is just to make possible
	//auto conversion from 0 to invalid Ref object
	//i.e. it will be possible to write 'return 0;'
	//from the function returning Ref
	inline Ref(int v = 0) :
			p(0)
	{
		M_REF_PRINT(<< "Ref::Ref(): invoked, p=" << (this->p) << std::endl)
	}



	/**
	 * @brief construct reference to given object.
	 * Constructs a reference to a given reference counted object.
	 * Note, that it is supposed that first reference will be constructed
	 * right after object creation, and further work with object will only be done
	 * via ting::Ref references, not ordinary pointers.
	 * Note, that this constructor is explicit, this is done to prevent undesired
	 * automatic conversions from ordinary pointers to Ref.
	 * @param rc - ordinary pointer to ting::RefCounted object.
	 */
	//NOTE: this constructor should be explicit to prevent undesired conversions from T* to Ref<T>
	explicit inline Ref(T* rc) :
			p(rc)
	{
		M_REF_PRINT(<< "Ref::Ref(rc): invoked, p = " << (this->p) << std::endl)
		ASSERT_INFO(this->p, "Ref::Ref(rc): rc is 0")

		//NOTE: in order to make sure that passed object inherits RefCounted
		//do static_cast() to RefCounted. Since the T type may be a const type
		//(e.g. Ref<const int>), so cast to const RefCounted and then use const_cast().
		if(this->p){
			const_cast<RefCounted*>(
					static_cast<const RefCounted*>(this->p)
				)->AddRef();
		}
		M_REF_PRINT(<< "Ref::Ref(rc): exiting" << (this->p) << std::endl)
	}



	/**
	 * @brief Construct reference from weak reference.
	 * @param r - weak reference.
	 */
	inline Ref(const WeakRef<T> &r);



private:
	template <class TS>inline void InitFromStrongRef(const Ref<TS>& r){
		this->p = r.p; //should downcast automaticly
		if(this->p){
			//NOTE: first, static cast to const RefCounted, because even if T is a const type
			//it is possible to do. After that we can cast to non-const RefCounted and call the
			//AddRef() method which is not const.
			//Thus, we also ensure at compile time that T inherits RefCounted.
			const_cast<RefCounted*>(
					static_cast<const RefCounted*>(this->p)
				)->AddRef();
		}
	}


public:
	/**
	 * @brief Copy constructor.
	 * Creates new reference object which referes to the same object as 'r'.
	 * @param r - existing Ref object to make copy of.
	 */
	//copy constructor
	inline Ref(const Ref& r){
		M_REF_PRINT(<< "Ref::Ref(copy): invoked, r.p = " << (r.p) << std::endl)
		this->InitFromStrongRef<T>(r);
	}



	//downcast / to-const cast constructor
	template <class TS> inline Ref(const Ref<TS>& r){
		M_REF_PRINT(<< "Ref::Ref(copy): invoked, r.p = " << (r.p) << std::endl)
		this->InitFromStrongRef<TS>(r);
	}



	inline ~Ref(){
		M_REF_PRINT(<< "Ref::~Ref(): invoked, p = " << (this->p) << std::endl)
		this->Destroy();
	}



	/**
	 * @brief tells whether the reference is pointing to some object or not.
	 * @return true if reference is pointing to valid object.
	 * @return false if the reference does not point to any object.
	 */
	//returns true if the reference is valid (not 0)
	inline bool IsValid()const{
		M_REF_PRINT(<<"Ref::IsValid(): invoked, this->p="<<(this->p)<<std::endl)
		return (this->p != 0);
	}



	/**
	 * @brief tells whether the reference is pointing to some object or not.
	 * Inverse of ting::Ref::IsValid().
	 * @return false if reference is pointing to valid object.
	 * @return true if the reference does not point to any object.
	 */
	inline bool IsNotValid()const{
		M_REF_PRINT(<<"Ref::IsNotValid(): invoked, this->p="<<(this->p)<<std::endl)
		return !this->IsValid();
	}



	/**
	 * @brief tells if 2 references are equal.
	 * @param r - reference to compare this reference to.
	 * @return true if both references are pointing to the same object or both are invalid.
	 * @return false otherwise.
	 */
	template <class TS> inline bool operator==(const Ref<TS>& r)const{
		return this->p == r.p;
	}



	/**
	 * @brief tells if 2 references are not equal.
	 * @param r - reference to compare this reference to.
	 * @return false if both references are pointing to the same object or both are invalid.
	 * @return true otherwise.
	 */
	template <class TS> inline bool operator!=(const Ref<TS>& r)const{
		return !(this->operator==<TS>(r));
	}



	/**
	 * @brief tells if the reference is invalid.
	 * @return true if the reference is invalid.
	 * @return false if the reference is valid.
	 */
	inline bool operator!()const{
		return !this->IsValid();
	}



	typedef void (Ref::*unspecified_bool_type)();
	


	/**
	 * @brief tells if the reference is valid.
	 * This operator is a more type-safe version of conversion-to-bool operator.
	 * Usage of standard 'operator bool()' is avoided because it may lead to undesired
	 * automatic conversions to int and other types.
	 * It is intended to be used as follows:
	 * @code
	 *	ting::Ref r = TestClass::New();
	 *	if(r){
	 *		//r is valid.
	 *		ASSERT(r)
	 *		r->DoSomethig();
	 *	}else{
	 *		//r is invalid
	 *	}
	 * @endcode
	 */
	//NOTE: Safe conversion to bool type.
	//      Because if using simple "operator bool()" it may result in chained automatic
	//      conversion to undesired types such as int.
	inline operator unspecified_bool_type() const{
		return this->IsValid() ? &Ref::Reset : 0;//Ref::Reset is taken just because it has matching signature
	}

	//NOTE: do not use this type of conversion, see NOTE to "operator unspecified_bool_type()"
	//      for details.
//	inline operator bool(){
//		return this->IsValid();
//	}

	

	/**
	 * @brief make this ting::Ref invalid.
	 * Resets this reference making it invalid and destroying the
	 * object it points to if necessary (if no references to the object left).
	 */
	void Reset(){
		this->Destroy();
		this->p = 0;
	}



	/**
	 * @brief assign reference.
	 * Note, that if this reference was pointing to some object, the object will
	 * be destroyed if there are no other references. And this reference will be assigned
	 * a new value.
	 * @param r - reference to assign to this reference.
	 * @return reference to this Ref object.
	 */
	Ref& operator=(const Ref& r){
		M_REF_PRINT(<< "Ref::operator=(): invoked, p = " << (this->p) << std::endl)
		if(this == &r)
			return *this;//detect self assignment

		this->Destroy();
		this->InitFromStrongRef<T>(r);
		return *this;
	}



	//downcast / to-const assignment
	template <class TS> Ref<T>& operator=(const Ref<TS>& r){
		//self-assignment should be impossible
		ASSERT(reinterpret_cast<const void*>(this) != reinterpret_cast<const void*>(&r))

		this->Destroy();
		this->InitFromStrongRef<TS>(r);

		return *this;
	}



	//NOTE: the operator is const because const Ref does not mean that the object it points to cannot be changed,
	//it means that the Ref itself cannot be changed to point to another object.
	inline T& operator*()const{
		M_REF_PRINT(<< "Ref::operator*(): invoked, p = " << (this->p) << std::endl)
		ASSERT_INFO(this->p, "Ref::operator*(): this->p is zero")
		return static_cast<T&>(*this->p);
	}



	//NOTE: the operator is const because const Ref does not mean that the object it points to cannot be changed,
	//it means that the Ref itself cannot be changed to point to another object.
	inline T* operator->()const{
		M_REF_PRINT(<< "Ref::operator->(): invoked, p = " << (this->p) << std::endl)
		ASSERT_INFO(this->p, "Ref::operator->(): this->p is zero")
		return this->p;
	}



private:
	inline void Destroy(){
		if(this->IsValid()){
			if(this->p->RemRef() == 0){
				ASSERT(this->IsValid())
				M_REF_PRINT(<< "Ref::Destroy(): deleting " << (this->p) << std::endl)
				//deleting should be ok without type casting, because RefCounted
				//destructor is virtual.
				delete this->p;
				M_REF_PRINT(<< "Ref::Destroy(): object " << (this->p) << " deleted" << std::endl)
			}
		}
	}



	//Ref objects can only be created on stack
	//or as a member of other object or array,
	//thus, make operator-new private.
	inline static void* operator new(size_t size){
		ASSERT_ALWAYS(false)//forbidden
		return 0;
	}
};//~class Ref



/**
 * @brief Weak Reference to a reference counted object.
 * A weak reference to reference counted object. The object is destroyed as soon as
 * there are no references to the object, even if there are weak references.
 * Thus, weak reference can point to some object but pointing to that object cannot
 * prevent deletion of this object.
 * To work with the object one needs to construct a hard reference (ting::Ref) to that
 * object first, a hard reference can be constructed from weak reference:
 * @code
 *	ting::Ref<TestCalss> t = TestClass::New();
 *	
 *	ting::WeakRef<TestClass> weakt = t; //weakt points to object
 * 
 *	//...
 * 
 *	if(ting::Ref<TestClass> obj = weakt){
 *		//object still exists
 *		obj->DoSomething();
 *	}
 *
 *	t.Reset();//destroy the object
 *	//the object of class TestClass will be destroyed here,
 *	//despite there is a weak reference to that object.
 *	//From now on, the weak reference does not point to any object.
 *
 *	if(ting::Ref<TestClass> obj = weakt){
 *		//we will not get there
 *		ASSERT(false)
 *	}
 * @endcode
 */
//T should be RefCounted!!!
template <class T> class WeakRef{
	friend class Ref<T>;
	template <class TS> friend class WeakRef;

	RefCounted::Counter *counter;
	T* p;//this pointer is only valid if counter is not 0 and counter->p is not 0
	


	inline void InitFromRefCounted(T *rc){
		M_REF_PRINT(<< "WeakRef::InitFromRefCounted(): invoked " << std::endl)
		if(!rc){
			this->counter = 0;
			return;
		}

		ASSERT(rc->counter)

		M_REF_PRINT(<< "WeakRef::InitFromRefCounted(): rc->counter->p = " << rc->counter->p << " rc = " << rc << std::endl)
		M_REF_PRINT(<< "WeakRef::InitFromRefCounted(): casted rc = " << static_cast<const RefCounted*>(rc) << std::endl)

		ASSERT_INFO(
				rc->counter->p == static_cast<const RefCounted*>(rc),
				"rc->counter->p = " << rc->counter->p
						<< " rc = " << static_cast<const RefCounted*>(rc)
			)
		
		this->InitFromCounter(ASS(rc->counter));

		this->p = rc;
		
		ASSERT_INFO(
				this->counter->p == static_cast<const RefCounted*>(this->p),
				"this->counter->p = " << this->counter->p
						<< " this->p = " << static_cast<const RefCounted*>(this->p)
			)
	}


	
	inline void InitFromStrongRef(Ref<T> &r){
		M_REF_PRINT(<< "WeakRef::InitFromStrongRef(): invoked " << std::endl)
		if(r.IsNotValid()){
			this->counter = 0;
			return;
		}

		this->InitFromRefCounted(r.operator->());
	}



	template <class TS> inline void InitFromWeakRef(const WeakRef<TS>& r){
		M_REF_PRINT(<< "WeakRef::InitFromWeakRef(): invoked " << std::endl)
		if(r.counter == 0){
			this->counter = 0;
			return;
		}
		ASSERT(r.counter)
		this->InitFromCounter(r.counter);

		this->p = r.p;//should cast automaticly
		//NOTE: if you get 'invalid conversion' error here, then you must be trying
		//to do automatic cast to unrelated type.
	}



	inline void InitFromCounter(RefCounted::Counter *c){
		ASSERT(c)
		ASSERT(c->p)
		this->counter = c;

		this->counter->mutex.Lock();
		++(this->counter->numWeakRefs);
		M_REF_PRINT(<< "WeakRef::InitFromCounter(): new numWeakRefs = " << (this->counter->numWeakRefs) << std::endl)
		this->counter->mutex.Unlock();
	}



	inline void Destroy(){
		if(this->counter == 0)
			return;

		this->counter->mutex.Lock();
		ASSERT(this->counter->numWeakRefs > 0)

		--this->counter->numWeakRefs;

		M_REF_PRINT(<< "WeakRef::Destroy(): new numWeakRefs = " << (this->counter->numWeakRefs) << std::endl)

		if(
				this->counter->numWeakRefs == 0 &&
				this->counter->numStrongRefs == 0 &&
				this->counter->p == 0 //this means that the Counter object is owned by WeakRef's
			)
		{
			this->counter->mutex.Unlock();
			M_REF_PRINT(<< "WeakRef::Destroy(): deleting counter" << std::endl)
			delete this->counter;
			return;
		}else{
			this->counter->mutex.Unlock();
		}
	}


	
public:
	//TODO:make it private and add static(?) method to RefCounted
	inline WeakRef(T* rc = 0){
		M_REF_PRINT(<< "WeakRef::WeakRef(T*): invoked" << std::endl)
		this->InitFromRefCounted(rc);
	}



	inline WeakRef(const Ref<T> &r){
		M_REF_PRINT(<< "WeakRef::WeakRef(const Ref<T>&): invoked" << std::endl)
		this->InitFromStrongRef(const_cast<Ref<T>&>(r));
	}



	//copy constructor
	inline WeakRef(const WeakRef& r){
		M_REF_PRINT(<< "WeakRef::WeakRef(const WeakRef&): invoked" << std::endl)
		this->InitFromWeakRef<T>(r);
	}



	//downcast / to-const cast constructor
	template <class TS> inline WeakRef(const WeakRef<TS>& r){
		M_REF_PRINT(<< "WeakRef::WeakRef(const WeakRef<TS>&): invoked" << std::endl)
		this->InitFromWeakRef<TS>(r);
	}



	inline ~WeakRef(){
		M_REF_PRINT(<< "WeakRef::~WeakRef(): invoked" << std::endl)
		this->Destroy();
	}



	inline WeakRef& operator=(T* rc){
		ASSERT(rc)
		M_REF_PRINT(<< "WeakRef::operator=(T*): invoked" << std::endl)

		//TODO: double mutex lock/unlock (one in destructor and one in Init). Optimize?
		this->Destroy();
		this->InitFromRefCounted(rc);
		return *this;
	}



	inline WeakRef& operator=(const Ref<T> &r){
		M_REF_PRINT(<< "WeakRef::operator=(const Ref<T>&): invoked" << std::endl)
		//TODO: double mutex lock/unlock (one in destructor and one in Init). Optimize?
		this->Destroy();
		this->InitFromStrongRef(const_cast<Ref<T>&>(r));
		return *this;
	}



	inline WeakRef& operator=(const WeakRef& r){
		M_REF_PRINT(<< "WeakRef::operator=(const WeakRef<TS>&): invoked" << std::endl)
		//TODO: double mutex lock/unlock (one in destructor and one in Init). Optimize?
		this->Destroy();
		this->InitFromWeakRef<T>(r);
		return *this;
	}



	//template for downcasting
	template <class TS> inline WeakRef& operator=(const WeakRef<TS>& r){
		M_REF_PRINT(<< "WeakRef::operator=(const WeakRef<TS>&): invoked" << std::endl)
		//TODO: double mutex lock/unlock (one in destructor and one in Init). Optimize?
		this->Destroy();
		this->InitFromWeakRef<TS>(r);
		return *this;
	}



	/**
	 * @brief Reset this reference.
	 * After calling this method the reference becomes invalid, i.e. it
	 * does not refer to any object.
	 */
	inline void Reset(){
		M_REF_PRINT(<< "WeakRef::Reset(): invoked" << std::endl)
		this->Destroy();
		this->counter = 0;
	}



	inline bool IsSurelyInvalid()const{
		return this->counter == 0 || this->counter->p == 0;
	}


private:
	inline static void* operator new(size_t size){
		ASSERT_ALWAYS(false)//forbidden
		//WeakRef objects can only be created on stack
		//or as a memer of other object or array
		return 0;
	}
};//~class WeakRef



template <class T> inline Ref<T>::Ref(const WeakRef<T> &r){
	if(r.counter == 0){
		this->p = 0;
		return;
	}

	ASS(r.counter)->mutex.Lock();

	if(r.counter->p){
		//NOTE: static_cast() to const(!) RefCounted because T may be a const type.
		ASSERT(r.counter->p == static_cast<const RefCounted*>(r.p))
		this->p = ASS(r.p);
		++(r.counter->numStrongRefs);
	}else{
		this->p = 0;
	}
	
	r.counter->mutex.Unlock();
}



}//~namespace
