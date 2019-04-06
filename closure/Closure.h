

/* Closure.h ****************************************************************


To Do : ---------------------------------------------------------------------

  Bugs:

  - ...

  High priority:

  - ...

  Medium priority:

  - memStream:
    - EOS: max size & signal size has been exceeded

  Low priority:

  - ...


Notes: ----------------------------------------------------------------------


-------------------------------------------------------------------------- */


// build controls -----------------------------------------------------------

#pragma once


// includes -----------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <stdio.h>

#include <windows.h>


// forward references -------------------------------------------------------

class pageBlock;
class rawHeap;


// debug forward references


// controls -----------------------------------------------------------------

// debug controls


// macros -------------------------------------------------------------------

#define ROUND_UP(n, u)  ((uint )(((n) + ((u) - 1)) / (u)) * (u))

#define ZERO(s)   memset(&(s), 0, sizeof(s))

#define NOOP      ((void )NULL)

#define TRACE     printf

#define THROW_HRESULT_EXCEPTION(hr) (printf("[THROW_HRESULT_EXCEPTION]\n"), exit(1))

#define THROW_WIN32_EXCEPTION()     (printf("[THROW_WIN32_EXCEPTION]\n"), exit(1))

#define THROW_RESOURCE_EXCEPTION()  (printf("[THROW_RESOURCE_EXCEPTION]\n"), exit(1))

#define THROW_LOGIC_EXCEPTION()     (printf("[THROW_LOGIC_EXCEPTION]\n"), exit(1))

#define STATUS_OF(e)      ((e) ? SUCCESS_S : FAILURE_S) // expr   -> status
#define STATUS_OF_BOOL(f) ((status )(f))                // bool   -> status
#define BOOL_OF_STATUS(s) ((s) ? true : false)          // status -> bool


// constants ----------------------------------------------------------------


// types --------------------------------------------------------------------

typedef unsigned char uint8;
typedef unsigned int  uint32;

typedef uint32 uint;

typedef
  enum
    { FAILURE_S = false
    , SUCCESS_S = true
    }
  status;


// Call types ///////////////////////////////////////////////////////////////
//
//    Call types are especially relevant to the construction and operation of
// closures.  See external documentation for detailed discussion of call
// types, closures & currying.
//
//    Here 'dword' is shorthand for any dword-sized parameter, any argument
// that is actually passed as a 32-bit value by the compiler, including ints,
// pointers, and references.  Note parameters smaller than a dword should be
// considered equivalent to dword-sized.  Any additional parameters beyond
// the ones explicitly mentioned in the call type can be of any size or type.
//
// Name:        A function call expecting:
// ----------   -------------------------
// std0Call   - Zero or possibly more parameters via stack.
// std1Call   - One dword and possibly more parameters via stack.
// std2Call   - Two dwords and possibly more parameters via stack.
// std3Call   - Three dwords and possibly more parameters via stack.
//
// fast2Call  - Two dwords via registers, possibly more parameters via stack.
// fast3Call  - 2 dword reg params, 1 dword & possibly more stack params.
//
typedef void *(__stdcall   *std0Call  ) (void);
typedef void *(__stdcall   *std1Call  ) (void *a);
typedef void *(__stdcall   *std2Call  ) (void *a, void *b);
typedef void *(__stdcall   *std3Call  ) (void *a, void *b, void *c);
typedef void *(__fastcall  *fast2Call ) (void *a, void *b);
typedef void *(__fastcall  *fast3Call ) (void *a, void *b, void *c);


// classes ------------------------------------------------------------------

// memBlock class ///////////////////////////////////////////////////////////
//
//    A block of memory within a pageBlock.
//
class memBlock
  {

  // public constants .......................................................

  public: static const uint UNIT;


  // public class operators .................................................

  public: static void *operator new(size_t size, uint8 *addr)
    {return addr;}

  public: static void operator delete(void *addr0, uint8 *addr1)
    {}

  public: static void operator delete(void *addr)
    {}


  // public class services ..................................................

  public: static void RawFree(uint8 *&memP);

  public: static uint SizeRound(uint size)
    {return ROUND_UP(size, UNIT) + UNIT;}

  public: static uint Count(void)
    {return _Count;}


  // public transtruction ...................................................

  public: explicit memBlock(pageBlock &parent);

  public: ~memBlock(void);


  // public instance operators ..............................................

  public: virtual uint8 &operator[](uint i) const;


  // public instance services ...............................................

  public: status  alloc (uint size, uint8 *&memP);
  public: void    free  (bool mergeF = true);
  public: uint    size  (void) const;

  public: void    dump  (void) const;

  public: bool freeQ(void) const        // get
    {return _freeF;}

  public: void freeQ(bool freeF)        // set
    {_freeF = freeF;}

  public: pageBlock &parent(void) const
    {return const_cast<pageBlock &>(_parent);}

  public: memBlock *&prev(void) const
    {return const_cast<memBlock *&>(_prev);}

  public: memBlock *&next(void) const
    {return const_cast<memBlock *&>(_next);}

  public: memBlock *&freeNext(void) const
    {return const_cast<memBlock *&>(_freeNext);}

  public: uint8 *base(void) const
    {return ((uint8 *)this) + UNIT;}

  public: uint8 *end(void) const
    {return base() + size();}


  // private class services .................................................

  private: static memBlock &Object(uint8 *mem)
    {return *(memBlock *)(mem - UNIT);}


  // private instance services ..............................................

  private: uint8 *objectBase(void) const
    {return (uint8 *)&Object(base());}

  // disallowed
  //
  private: memBlock(const rawHeap &);             // copy constructor
  private: memBlock &operator=(const rawHeap &);  // copy assignment


  // private class data .....................................................

  private: static uint _Count;


  // private instance data ..................................................

  private: pageBlock &_parent;
  private: memBlock  *_prev;
  private: memBlock  *_next;
  private: memBlock  *_freeNext;
  private: bool       _freeF;

  };


// pageBlock class //////////////////////////////////////////////////////////
//
//    A contiguos block of memory pages within a rawHeap.
//
class pageBlock
  {

  // public constants .......................................................

  public: static const int MIN_SIZE = 0x100;  // minimum acceptable page size


  // public class services ..................................................

  public: static uint Count(void)
    {return _Count;}


  // public transtruction ...................................................

  public: explicit pageBlock  ( rawHeap  &parent
                              , uint      size        = 1
                              , uint      protectType = PAGE_EXECUTE_READWRITE
                              ) ;

  public: ~pageBlock(void);


  // public instance operators ..............................................

  public: virtual uint8 &operator[](uint i) const;


  // public instance services ...............................................

  public: status  alloc (uint size, uint8 *&memP);

  public: bool    memBlockAdd     (memBlock &mb, bool mergeF = false);
  public: void    memBlockRemove  (memBlock &mb);

  public: bool    memBlockFreeAdd     (memBlock &mb, bool mergeF = true);
  public: bool    memBlockFreeMerge   (memBlock &mb);
  public: void    memBlockFreeRemove  (memBlock &mb);
  public: bool    memBlockFreeFind    ( const memBlock &mb
                                      , memBlock ***prevPP = NULL
                                      ) const;

  public: uint    stats ( uint *freeSizeP   = NULL
                        , uint *allocSizeP  = NULL
                        ) const;

  public: void    dump (void) const;

  public: uint freeSize(void) const
    {uint size; stats(&size); return size;}

  public: uint allocSize(void) const
    {uint size; stats(NULL, &size); return size;}

  public: uint overheadSize(void) const
    {return size() - stats();}

  public: rawHeap &parent(void) const
    {return const_cast<rawHeap &>(_parent);}

  public: uint8 *base(void) const
    {return const_cast<uint8 *>(_base);}

  public: uint8 *end(void) const
    {return base() + size();}

  public: pageBlock *&next(void) const
    {return const_cast<pageBlock *&>(_next);}

  public: uint size(void) const
    {return _size;}


  // private instance services ..............................................

  private: memBlock *&memBlockFirst(void) const
    {return *const_cast<memBlock **>(&_mbList);}

  private: memBlock *&memBlockFreeFirst(void) const
    {return *const_cast<memBlock **>(&_freeList);}

  // disallowed
  //
  private: pageBlock(const pageBlock &);            // copy constructor
  private: pageBlock &operator=(const pageBlock &); // copy assignment


  // private class data .....................................................

  private: static uint _Count;


  // private instance data ..................................................

  // fixed
  //
  private: rawHeap   &_parent;
  private: uint8     *_base;
  private: uint       _size;

  // changing
  //
  private: pageBlock *_next;
  private: memBlock  *_mbList;
  private: memBlock  *_freeList;

  };


// rawHeap class ////////////////////////////////////////////////////////////
//
//    A heap built from raw memory pages with control over the Win32 memory
// allocation type & options.
//
class rawHeap
  {

  // public class services ..................................................

  public: static uint Count(void)
    {return _Count;}


  // public transtruction ...................................................

  public: explicit rawHeap  ( uint size         = 1
                            , uint protectType  = PAGE_EXECUTE_READWRITE
                            ) ;

  public: ~rawHeap(void);


  // public instance services ...............................................

  public: uint8  *alloc (uint size);                // throw on failure
  public: status  alloc (uint size, uint8 *&memP);

  public: void pageBlockAdd     (pageBlock &pb);
  public: void pageBlockRemove  (pageBlock &pb);

  public: void dump             (void) const;

  public: uint protectType(void) const
    {return _protectType;}


  // private instance services ..............................................

  private: bool pageBlockFind ( pageBlock &pb
                              , pageBlock ***prevPP = NULL
                              ) const;

  private: pageBlock *pageBlockFirst(void) const
    {return const_cast<pageBlock *>(_pbList);}


  // disallowed
  //
  private: rawHeap(const rawHeap &);            // copy constructor
  private: rawHeap &operator=(const rawHeap &); // copy assignment


  // private class data .....................................................

  private: static uint _Count;


  // private instance data ..................................................

  private: uint       _protectType;
  private: pageBlock *_pbList;

  };


// memStream class //////////////////////////////////////////////////////////
//
//    Acts as a sink for writing bytes to memory, such as when building an
// instruction stream (manufacturing a function at runtime).  Can be used to
// measure memory needs without actually writing to memory by constructing
// with a NULL memory pointer.
//    Includes growing list of helper methods that append machine
// instructions to the stream, named similarly to the assembly mnemonics for
// the respective instructions.
//    Note value semantics: copy construction & copy assignment are allowed.
//
class memStream
  {

  // public transtruction ...................................................

  public: explicit memStream(uint8 *mem = NULL);


  // public instance operators ..............................................

  public: virtual uint8 &operator[] (uint i) const;

  public: virtual void operator+=(uint i)
    {p(p() + i);}

  public: virtual void operator++(void)
    {*this += 1;}


  // public instance services ...............................................

  public: void write8     (uint8    data8);
  public: void write32    (uint32   data32);
  public: void write32    (void    *addr32);
  public: void write32Rel (void    *addr32);

  public: bool emptyQ(void) const
    {return nullQ() || !size();}

  public: bool nullQ(void) const
    {return NULL == base();}

  public: uint size(void) const
    {return end() - base();}

  public: uint8 *p(void) const          // get
    {return _p;}

  public: void p(uint8 *p)              // set
    {if ((_p = p) > _end) _end = _p;}

  public: uint8 *base(void) const
    {return _base;}

  public: uint8 *end(void) const
    {return _end;}

  // write instruction services

  public: void jmp      (void  *a32) {write8(0xE9); write32Rel ( a32);}
  public: void mov_ecx  (uint32 d32) {write8(0xB9); write32    ( d32);}
  public: void mov_ecx  (void  *a32) {write8(0xB9); write32    ( a32);}
  public: void mov_edx  (uint32 d32) {write8(0xBA); write32    ( d32);}
  public: void mov_edx  (void  *a32) {write8(0xBA); write32    ( a32);}
  public: void pop_eax  (void      ) {write8(0x8F); write8     (0xC0);}
  public: void push     (uint32 d32) {write8(0x68); write32    ( d32);}
  public: void push     (void  *a32) {write8(0x68); write32    ( a32);}
  public: void push_eax (void      ) {write8(0xFF); write8     (0xF0);}


  // private instance data ..................................................

  private: uint8 *_base;
  private: uint8 *_end;
  private: uint8 *_p;

  };


// dynFunc class ////////////////////////////////////////////////////////////
//
//    Class for creating 'dynamic functions', functions manufactured at
// runtime.
//
//    The dynFunc class is a base class for creating dynamic function objects
// that have standard C++ semantics (such as the option for automatic
// lifetime).  A derived class can use the dynFunc helper static services
// (BuildingQ & Alloc) to create the code, then use the dynFunc class as a
// container, relying on the destructor to release the memory.
//
class dynFunc
  {

  // public class services ..................................................

  public: static bool BuildingQ (memStream &ms);

  public: static memStream Alloc(uint size)
    {return memStream(_Heap.alloc(size));}

  public: static const rawHeap &Heap(void)
    {return _Heap;}


  // public transtruction ...................................................

  public: virtual ~dynFunc(void)
    {memBlock::RawFree(_mem);}


  // public instance services ...............................................

  public: uint8 *mem(void) const
    {return const_cast<uint8 *>(_mem);}


  // protected transtruction ................................................

  protected: explicit dynFunc(uint8 *mem)
    : _mem(mem)
    {}


  // private instance services ..............................................

  // disallowed
  //
  private: dynFunc(const dynFunc &);            // copy constructor
  private: dynFunc &operator=(const dynFunc &); // copy assignment


  // private class data .....................................................

  public: static rawHeap _Heap;


  // private instance data ..................................................

  public: uint8 *_mem;

  };


// closure class ////////////////////////////////////////////////////////////
//
//    Class for creating closure-like entities in C++.  Closures are
// functions manufactured at runtime, that are used here to call another
// function with some of the parameters 'filled in' already.  Useful for
// 'currying' functions, or to put it another way adding data to function
// pointers.  See external documentation for detailed discussion of closures
// in general and this class in particular.  See also the 'call types'
// comments.
//
//    The closure class is both a container for static functions for creating
// and freeing 'raw' closures (a simple function pointer which must be
// manually freed, C-style, to release the associated memory) and also a base
// class for creating closure objects that have standard C++ semantics (such
// as the option for automatic lifetime).
//
class closure : public dynFunc
  {

  // public class services ..................................................

  public: static std0Call Std0RawClosure  (std1Call func, void *a);
  public: static std0Call Std0RawClosure  (std2Call func, void *a, void *b);
  public: static std0Call Std0RawClosure  ( std3Call func
                                          , void *a
                                          , void *b
                                          , void *c
                                          ) ;
  public: static std0Call Std0RawClosure  (fast2Call func, void *a, void *b);
  public: static std0Call Std0RawClosure  ( fast3Call func
                                          , void *a
                                          , void *b
                                          , void *c
                                          ) ;

  public: static void RawClosureFree(std0Call &rawClosure)
    {memBlock::RawFree(*(uint8 **)&rawClosure);}


  // protected transtruction ................................................

  protected: explicit closure(uint8 *mem)
    : dynFunc(mem)
    {}

  };


// std0Closure class ////////////////////////////////////////////////////////
//
//    Create a std0Call-signature closure from various-signatured functions.
//
//    The actual signatures of the input and output functions may of course
// in actuality be supersets of the signatures used here.  The user can
// manually cast the function pointers on the way in and / or out, or derive
// a closure class that performs the casts automatically, using this as a
// base class.  See 'call type' comments.
//
class std0Closure : public closure
  {

  // public transtruction ...................................................

  public: std0Closure(std1Call func, void *a)
    : closure((uint8 *)Std0RawClosure(func, a))
    {}

  public: std0Closure(std2Call func, void *a, void *b)
    : closure((uint8 *)Std0RawClosure(func, a, b))
    {}

  public: std0Closure(std3Call func, void *a, void *b, void *c)
    : closure((uint8 *)Std0RawClosure(func, a, b, c))
    {}

  public: std0Closure(fast2Call func, void *a, void *b)
    : closure((uint8 *)Std0RawClosure(func, a, b))
    {}

  public: std0Closure(fast3Call func, void *a, void *b, void *c)
    : closure((uint8 *)Std0RawClosure(func, a, b, c))
    {}

  // public instance operators ..............................................

  public: operator std0Call(void)
    {return func();}


  // public instance services ...............................................

  public: std0Call func(void)
    {return (std0Call )mem();}

  };


// class inline services ----------------------------------------------------

// memStream inline services ................................................

// memStream::memStream /////////////////////////////////////////////////////
//
//    Construct memory stream.  A NULL mem pointer is valid and will result
// in a stream that accepts data but does not write it to memory, useful for
// measuring extents of streams before allocating memory.
//
inline memStream::memStream(uint8 *mem)
  : _base(mem)
  , _end(mem)
  , _p(mem)
  {

  }


inline uint8 &memStream::operator[](uint i) const
  {

  if (i >= size())
    {
    TRACE("Error: memStream::operator[]: Index out of range.\n");
    THROW_LOGIC_EXCEPTION();
    }

  return const_cast<uint8 &>(base()[i]);
  }


// memStream::write8 ////////////////////////////////////////////////////////
//
//    Write an 8-bit value to stream, if non-null.  Advance stream pointer
// regardless.
//
inline void memStream::write8(uint8 data8)
  {

  if (!nullQ())
    *p() = data8;

  ++*this;

  }


// memStream::write32 (1 of 2) //////////////////////////////////////////////
//
//    Write a 32-bit value to stream, if non-null.  Advance stream pointer
// regardless.  Stream pointer need not be aligned to any boundary.
//
inline void memStream::write32(uint32 data32)
  {

  if (!nullQ())
    memcpy(p(), &data32, sizeof(data32));

  *this += sizeof(data32);

  }


// memStream::write32 (2 of 2) //////////////////////////////////////////////
//
//    Write a 32-bit address to stream, if non-null.  Advance stream pointer
// regardless.  Stream pointer need not be aligned to any boundary.
//
inline void memStream::write32(void *addr32)
  {
  uint data32 = (uint )addr32;

  if (!nullQ())
    memcpy(p(), &data32, sizeof(data32));

  *this += sizeof(data32);

  }


// memStream::write32Rel ////////////////////////////////////////////////////
//
//    Write a 32-bit relative address to stream, if non-null.  Advance stream
// pointer regardless.  Stream pointer need not be aligned to any boundary.
// Address is calculated relative to the first stream address after the
// stream memory used to store the relative address itself.  This is the form
// of relative address used by IA32 jump instructions.
//
inline void memStream::write32Rel(void *addr32)
  {
  uint data32 = (uint8 *)addr32 - (p() + sizeof(data32));

  if (!nullQ())
    memcpy(p(), &data32, sizeof(data32));

  *this += sizeof(data32);

  }


// memBlock inline services .................................................

inline uint8 &memBlock::operator[](uint i) const
  {

  if (i >= size())
    {
    TRACE("Error: memBlock::operator[]: Index out of range.\n");
    THROW_LOGIC_EXCEPTION();
    }

  return const_cast<uint8 &>(base()[i]);
  }


// pageBlock inline services ................................................

inline uint8 &pageBlock::operator[](uint i) const
  {

  if (i >= size())
    {
    TRACE("Error: pageBlock::operator[]: Index out of range.\n");
    THROW_LOGIC_EXCEPTION();
    }

  return const_cast<uint8 &>(base()[i]);
  }


/* junkyard -----------------------------------------------------------------


end of file -------------------------------------------------------------- */

