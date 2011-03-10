/* The MIT License:

Copyright (c) 2009-2011 Ivan Gagis

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
 * @file PoolStored.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Memory Pool.
 * Alternative memory allocation functions for simple objects.
 * The main purpose of this facilities is to prevent memory fragmentation.
 */

#pragma once

#include <new>
#include <list>

#include "debug.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "Thread.hpp"
#include "Exc.hpp"
#include "Array.hpp"

//#define M_ENABLE_POOL_TRACE
#ifdef M_ENABLE_POOL_TRACE
#define M_POOL_TRACE(x) TRACE(<<"[POOL] ") TRACE(x)
#else
#define M_POOL_TRACE(x)
#endif

namespace ting{

//make sure theat we align PoolElem by int size when using MSVC compiler.
STATIC_ASSERT(sizeof(int) == 4)


/**
 * @brief Base class for pool-stored objects.
 * If the class is derived from PoolStored it will override 'new' and 'delete'
 * operators for that class so that the objects would be stored in the
 * memory pool instead of using standard memory manager to allocate memory.
 * Storing objects in memory pool allows to avoid memory fragmentation.
 * PoolStored is only useful for systems with large ammount of small and
 * simple objects which have to be allocated dynamically (i.e. using new/delete
 * operators).
 * For example, PoolStored is used in ting::Ref (reference counted objects)
 * class to allocate reference counting objects which holds number of references  and
 * pointer to reference counted object.
 * NOTE: class derived from PoolStored SHALL NOT be used as a base class further.
 */
template <class T> class PoolStored{
	
	template <unsigned ElemSize, unsigned NumElemsInChunk> class MemPool{
		struct BufHolder{
			u8 buf[ElemSize];
		};

		M_DECLARE_ALIGNED_MSVC(4) struct PoolElem : public BufHolder{
			bool isFree;
			PoolElem() :
					isFree(true)
			{}
		}
		//Align by sizeof(int) boundary, just to be more safe.
		//I once had a problem with pthread mutex when it was not aligned by 4 byte bounday,
		//so I resolved this by declaring PoolElem struct as aligned by sizeof(int).
		M_DECLARE_ALIGNED(sizeof(int));


		struct Chunk : public ting::Array<PoolElem>{
			unsigned numAllocated;
			Chunk() :
					ting::Array<PoolElem>(NumElemsInChunk),
					numAllocated(0)
			{}

			Chunk(const Chunk& c) :
					ting::Array<PoolElem>(c),
					numAllocated(c.numAllocated)
			{
				const_cast<Chunk&>(c).numAllocated = 0;//to prevent assert in destructor
				M_POOL_TRACE(<< "Chunk::Chunk(copy): invoked" << std::endl)
			}

			~Chunk(){
				ASSERT_INFO(this->numAllocated == 0, "this->numAllocated = " << this->numAllocated << " should be 0")
			}

		private:
			Chunk& operator=(const Chunk&);//assignment is not allowed (no operator=() implementation provided)
		};

		
		struct ChunksList{
			typedef std::list<Chunk> T_List;
			typedef typename T_List::iterator T_Iter;
			T_List chunks;
			ting::Mutex mutex;

			~ChunksList(){
//				TRACE(<< "PoolStored::ChunksList::~ChunksList(): invoked" << std::endl)
				ASSERT_INFO(this->chunks.size() == 0, "PoolStored: cannot destroy chunk list because it is not empty. Check for static PoolStored objects, they are not allowed, e.g. static Ref/WeakRef are not allowed!")
			}
		};
		
		static ChunksList& Chunks(){
			static ChunksList chunks;
			return chunks;
		}

	public:
		static void* Alloc(){
			ChunksList &cl = Chunks();
			
			ting::Mutex::Guard mutlock(cl.mutex);

			//find chunk with free cell
			Chunk *chunk = 0;
			for(typename ChunksList::T_Iter i = cl.chunks.begin(); i != cl.chunks.end(); ++i){
				if((*i).numAllocated < (*i).Size()){
					chunk = &(*i);
				}
			}
			
			//create new chunk if necessary
			if(chunk == 0){
				cl.chunks.push_back(Chunk());
				chunk = &cl.chunks.back();
			}

			ASSERT(chunk)
			M_POOL_TRACE(<< "Alloc(): Free chunk = " << chunk << std::endl)

			//find free cell
			for(PoolElem* i = chunk->Begin(); i != chunk->End(); ++i){
				if(i->isFree){
					ASSERT(chunk->numAllocated < chunk->Size())
					i->isFree = false;
					++chunk->numAllocated;
					M_POOL_TRACE(<< "Alloc(): Free cell found = " << i << " sizeof(PoolElem) = " << sizeof(PoolElem) << std::endl)
					M_POOL_TRACE(<< "Alloc(): returning " << static_cast<BufHolder*>(i) << std::endl)
					return ASS(reinterpret_cast<void*>(static_cast<BufHolder*>(i)));
				}
			}
			ASSERT(false)
			return 0;
		}

		static void Free(void* p){
			M_POOL_TRACE(<< "Free(): p = " << p << std::endl)
			if(!p)
				return;
			
			ChunksList &cl = Chunks();

			ting::Mutex::Guard mutlock(cl.mutex);
			
			//find chunk the "p" belongs to
			for(typename ChunksList::T_Iter i = cl.chunks.begin(); i != cl.chunks.end(); ++i){
				ASSERT((*i).numAllocated != 0)
				if((*i).Begin() <= p && p < (*i).End()){
					Chunk *chunk = &(*i);
					M_POOL_TRACE(<< "Free(): chunk found = " << chunk << std::endl)
					--(chunk->numAllocated);
					if(chunk->numAllocated == 0){
						cl.chunks.erase(i);
					}else{
						static_cast<PoolElem*>(
								ASS(reinterpret_cast<BufHolder*>(p))
							)->isFree = true;
					}
					return;
				}
			}
			ASSERT(false)
		}
	};//~template class MemPool

protected:
	//this should only be used as a base class
	PoolStored(){}

public:

#define M_MEMPOOL_TYPEDEF \
typedef MemPool< \
		sizeof(T), \
		((8192 / sizeof(T)) < 32) ? 32 : (8192 / sizeof(T)) \
	> T_MemoryPool;

	static void* operator new(size_t size){
		M_POOL_TRACE(<< "new(): size = " << size << std::endl)
		if(size != sizeof(T))
			throw ting::Exc("PoolStored::operator new(): attempt to allocate memory block of incorrect size");

		M_MEMPOOL_TYPEDEF

		return T_MemoryPool::Alloc();
	}

	static void operator delete(void *p){
		M_MEMPOOL_TYPEDEF
		
		T_MemoryPool::Free(p);
	}
	
#undef M_MEMPOOL_TYPEDEF

private:
};

}//~namespace ting
