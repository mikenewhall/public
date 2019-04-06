

/* Closure.cpp **************************************************************


To Do : ---------------------------------------------------------------------

  Bugs:

  - ...

  High priority:


  Medium priority:


  Low priority:



Notes: ----------------------------------------------------------------------


-------------------------------------------------------------------------- */


// includes -----------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN

#include "Closure.h"


// forward references -------------------------------------------------------

// debug forward references


// controls -----------------------------------------------------------------

// debug controls


// macros -------------------------------------------------------------------


// private constants --------------------------------------------------------


// private types ------------------------------------------------------------


// private classes ----------------------------------------------------------


// private data -------------------------------------------------------------


// private inline services --------------------------------------------------


// private class implementations --------------------------------------------


// public class implementations ---------------------------------------------

// memBlock class implementation ////////////////////////////////////////////

// memBlock constants .......................................................

const uint memBlock::UNIT = ROUND_UP(sizeof(memBlock), 8);


// memBlock data ............................................................

uint memBlock::_Count = 0;


// memBlock services ........................................................

void memBlock::RawFree(uint8 *&memP)
  {
  memBlock &mb(Object(memP));

  if (NULL == memP)
    {
    TRACE("Error: memBlock::RawFree: Attempt to free NULL pointer.\n");
    THROW_LOGIC_EXCEPTION();
    }

  mb.free(true);

  memP = NULL;

  }


memBlock::memBlock(pageBlock &parent)
  : _parent(parent)
  , _prev(NULL)
  , _next(NULL)
  , _freeNext(NULL)
  , _freeF(false)
  {

  this->parent().memBlockAdd(*this);

  ++_Count;

  }


memBlock::~memBlock(void)
  {

  --_Count;

  parent().memBlockRemove(*this);

  if (!_Count) TRACE("[all memBlocks freed]\n");

  }


void memBlock::dump(void) const
  {

  TRACE
    ( "base: 0x%08X End: 0x%08X Size: 0x%04X Status: %s\n"
    , base(), end(), size(), freeQ() ? "FREE" : "alloc'd"
    ) ;

  }


uint memBlock::size(void) const
  {
  memBlock *nextP = next();

  // calculate size from distance to base of next block in memory, if any;
  // else for last block calculate distance to end of containing page block
  //
  if (NULL != nextP)
    return nextP->objectBase() - base();
  else
    return parent().end() - base();
  }


// memBlock::alloc //////////////////////////////////////////////////////////
//
//    Note memP must be NULL when called.
//
status memBlock::alloc(uint size, uint8 *&memP)
  {
  uint newSize = SizeRound(size);
  uint oldSize = this->size();

  if (NULL != memP)
    {
    TRACE("Error: memBlock::alloc: Receiving pointer not NULL.\n");
    THROW_LOGIC_EXCEPTION();
    }

  // check if mem block has room for requested space
  //
  if (newSize <= oldSize)
    {
    int extra = oldSize - newSize;

    memP = base();
    parent().memBlockFreeRemove(*this);

    // if left-over space is large enough for a minimum-size mem block,
    // create a new mem block from it
    //
    if (extra >= 2 * UNIT)
      new(objectBase() + newSize) memBlock(parent());

    return SUCCESS_S;
    }

  return FAILURE_S;
  }


void memBlock::free(bool mergeF)
  {

  parent().memBlockFreeAdd(*this, mergeF);

  }


// pageBlock class implementation ///////////////////////////////////////////

// pageBlock data ...........................................................

uint pageBlock::_Count = 0;


// pageBlock services .......................................................

// pageBlock::pageBlock /////////////////////////////////////////////////////
//
//    Note Widows will always allocate an integral number of pages, rounded
// up, so the default 'size' of one byte will allocate one page.
//
pageBlock::pageBlock
  ( rawHeap  &parent
  , uint      size
  , uint      protectType
  )
  : _parent(parent)
  , _next(NULL)
  , _mbList(NULL)
  , _freeList(NULL)
  {
  MEMORY_BASIC_INFORMATION mbi;

  size = memBlock::SizeRound(size);

  _base
    = (uint8 *)::VirtualAlloc
        (NULL, size, MEM_RESERVE | MEM_COMMIT, protectType);

  if (NULL == base())
    {
    TRACE("Error: pageBlock::pageBlock: Failed to allocate pages.\n");
    THROW_RESOURCE_EXCEPTION();
    }

  if (sizeof(mbi) != ::VirtualQuery(base(), &mbi, sizeof(mbi)))
    {
    TRACE("Error: pageBlock::pageBlock: Allocated zero bytes.\n");
    THROW_RESOURCE_EXCEPTION();
    }

  _size = mbi.RegionSize;

  if (this->size() < MIN_SIZE)
    {
    TRACE("Error: pageBlock::pageBlock: Allocation too small.\n");
    THROW_RESOURCE_EXCEPTION();
    }

  this->parent().pageBlockAdd(*this);

  new(base()) memBlock(*this);

  ++_Count;

  }


pageBlock::~pageBlock(void)
  {

  --_Count;

  TRACE("[~pageBlock: freeing %i memBlocks]\n", memBlock::Count());

  while (NULL != memBlockFirst())
    delete memBlockFirst();

  parent().pageBlockRemove(*this);

  ::VirtualFree(base(), 0, MEM_RELEASE);

  if (!_Count) TRACE("[all pageBlocks freed]\n");

  }


void pageBlock::dump(void) const
  {
  uint mbCount = 0;

  for (memBlock *mb = memBlockFirst(); NULL != mb; mb = mb->next())
    {
    TRACE("  memBlock #%u: ", mbCount);

    mb->dump();

    ++mbCount;
    }

  TRACE("\n");
  TRACE("        Base: 0x%08X\n", base());
  TRACE("         End: 0x%08X\n", end());
  TRACE("        Size: %10u\n", size());
  TRACE("        Free: %10u\n", freeSize());
  TRACE("     Alloc'd: %10u\n", allocSize());
  TRACE("    Overhead: %10u\n", overheadSize());

  }


// pageBlock::stats /////////////////////////////////////////////////////////
//
//    Return value is total of free size and alloc size.  This is size()
// minus overhead.
//
uint pageBlock::stats(uint *freeSizeP, uint *allocSizeP) const
  {
  uint freeSize = 0, allocSize = 0;

  for (memBlock *mb = memBlockFirst(); NULL != mb; mb = mb->next())
    if (mb->freeQ())
      freeSize += mb->size();
    else
      allocSize += mb->size();

  if (NULL != freeSizeP)
    *freeSizeP = freeSize;

  if (NULL != allocSizeP)
    *allocSizeP = allocSize;

  return freeSize + allocSize;
  }


// pageBlock::memBlockFreeFind //////////////////////////////////////////////
//
//    Returns true if memBlock found in free list.  If prevPP is not NULL,
// stores pointer to pointer to found node (that is, a pointer to the link to
// the node, useful to perform inserts & deletes at this point in the list).
//
bool pageBlock::memBlockFreeFind(const memBlock &mb, memBlock ***prevPP) const
  {

  for
    ( memBlock *const *prevP = &memBlockFreeFirst()
    ; NULL != *prevP
    ; prevP = &(*prevP)->freeNext()
    )
    if (&mb == *prevP)
      {
      if (NULL != prevPP)
        *prevPP = const_cast<memBlock **>(prevP);

      if (!mb.freeQ())
        {
        TRACE
          ( "Error: pageBlock::memBlockFreeFind:"
            " Alloc'd mem block found in free list.\n"
          ) ;
        THROW_LOGIC_EXCEPTION();
        }

      return true;
      }

  if (mb.freeQ() || NULL != mb.freeNext())
    {
    TRACE
      ( "Error: pageBlock::memBlockFreeFind:"
        " Free mem block not found in free list.\n"
      ) ;
    THROW_LOGIC_EXCEPTION();
    }

  return false;
  }


void pageBlock::memBlockFreeRemove(memBlock &mb)
  {
  memBlock **prevP;

  if (memBlockFreeFind(mb, &prevP))
    {
    mb.freeQ(false);
    *prevP        = mb.freeNext();
    mb.freeNext() = NULL;
    }
  else if (NULL != mb.freeNext())
    {
    TRACE
      ( "Error: pageBlock::memBlockFreeRemove:"
        " Attempt to remove alloc'd mem block from free list.\n"
      ) ;
    THROW_LOGIC_EXCEPTION();
    }

  }


// pageBlock::memBlockFreeMerge /////////////////////////////////////////////
//
//    Merge this free mem block with free neighbor(s), if any.  Returns true
// if a merge happened that required the current mem block to be deleted, in
// which case the memory block will be invalid after the call.  If to be
// deleted, will remove block from free list only if it is marked as free
// (allows avoiding adding to the list just to remove it a moment later).
//
bool pageBlock::memBlockFreeMerge(memBlock &mb)
  {

  // if this free mem block is followed by another free mem block, delete the
  // neighbor, merging the free space of both by letting this block take
  // responsibility for the whole range (automatically)
  //
  if (NULL != mb.next() && mb.next()->freeQ())
    delete mb.next();

  // if this free mem block is preceded by another free mem block, delete
  // this one, merging the free space of both by letting the lower-addressed
  // neighbor block take responsibility for the whole range (automatically)
  //
  if (NULL != mb.prev() && mb.prev()->freeQ())
    {
    if (mb.freeQ())                     // don't assume block is in free list
      memBlockFreeRemove(mb);

    delete &mb;

    return true;
    }

  return false;
  }


// pageBlock::memBlockFreeAdd ///////////////////////////////////////////////
//
//    Add mem block to free list.  Optionally perform a memBlockFreeMerge.
// Returns true if mem block is invalid (was deleted) due to a merge.
//
bool pageBlock::memBlockFreeAdd(memBlock &mb, bool mergeF)
  {

  if (mb.freeQ() || NULL != mb.freeNext())
    {
    TRACE
      ( "Error: pageBlock::memBlockFreeAdd:"
        " Attempt to add free block to free list.\n"
      ) ;
    THROW_LOGIC_EXCEPTION();
    }

  if (mergeF && memBlockFreeMerge(mb))
    return true;

  mb.freeNext()       = memBlockFreeFirst();
  memBlockFreeFirst() = &mb;
  mb.freeQ(true);

  return false;
  }


// pageBlock::memBlockAdd ///////////////////////////////////////////////////
//
//    Add mem block to page block.  Optionally perform a memBlockFreeMerge.
// Returns true if mem block is invalid (was deleted) due to a merge.
//
bool pageBlock::memBlockAdd(memBlock &mb, bool mergeF)
  {
  memBlock   *prev = NULL, *i;
  memBlock  **prevP;

  // check that mem block is not already a child of some page block, and that
  // it is within the address range of the parent page block
  //
  if
    (   NULL != mb.prev()
    ||  NULL != mb.next()
    ||  mb.base()                 <   base()
    ||  (mb.base() + 2 * mb.UNIT) >=  end()
    )
    {
    TRACE("Error: pageBlock::memBlockAdd: Cannot make child of page block.\n");
    THROW_LOGIC_EXCEPTION();
    }

  // loop until mem block found with higher address than one being added, or
  // end of list reached
  //
  for
    ( prevP = &memBlockFirst()
    ; NULL != (i = *prevP) && i < &mb
    ; prevP = &i->next()
    )
    prev = i;

  if (NULL != i)                        // if not inserting at end of list
    {
    if (&mb == i)                       // if mem block is in list already
      {
      TRACE
        ( "Error: pageBlock::memBlockAdd:"
          " Mem block already child of page block.\n"
        ) ;
      THROW_LOGIC_EXCEPTION();
      }

    i->prev() = &mb;
    }

  mb.next() = i;
  mb.prev() = prev;
  *prevP    = &mb;

  return memBlockFreeAdd(mb, mergeF);   // add to free list
  }


void pageBlock::memBlockRemove(memBlock &mb)
  {
  memBlock *prev = mb.prev(), *next = mb.next();

  // link previous node, if any, to next node
  //
  if (NULL != prev)
    {
    prev->next()  = next;
    mb.prev()     = NULL;
    }
  else
    //
    // if this was the first node in the list, update the list head pointer
    //
    memBlockFirst() = next;

  // link next node, if any, to previous node
  //
  if (NULL != next)
    {
    next->prev()  = prev;
    mb.next()     = NULL;
    }

  // remove block from free list, if on it
  //
  if (mb.freeQ())
    memBlockFreeRemove(mb);

  }


// pageBlock::alloc /////////////////////////////////////////////////////////
//
//    Note memP must be NULL when called.
//
status pageBlock::alloc(uint size, uint8 *&memP)
  {

  if (NULL != memP)
    {
    TRACE("Error: pageBlock::alloc: Receiving pointer not NULL.\n");
    THROW_LOGIC_EXCEPTION();
    }

  // check each existing mem block, if any, to find room for requested space
  //
  for (memBlock *mb = memBlockFirst(); NULL != mb; mb = mb->next())
    if (mb->alloc(size, memP))
      return SUCCESS_S;

  return FAILURE_S;
  }


// rawHeap class implementation /////////////////////////////////////////////

// rawHeap data .............................................................

uint rawHeap::_Count = 0;


// rawHeap services .........................................................

rawHeap::rawHeap(uint size, uint protectType)
  : _protectType(protectType)
  , _pbList(NULL)
  {

  if (size)
    pageBlock(*this, size, this->protectType());

  ++_Count;

  }


rawHeap::~rawHeap(void)
  {

  --_Count;

  TRACE("[~rawHeap: freeing %i pageBlocks]\n", pageBlock::Count());

  while (NULL != _pbList)
    delete _pbList;

  if (!_Count) TRACE("[all rawHeaps freed]\n");

  }


void rawHeap::dump(void) const
  {
  uint pbCount = 0;

  for (pageBlock *pb = pageBlockFirst(); NULL != pb; pb = pb->next())
    {
    TRACE("pageBlock #%u:\n", pbCount);

    pb->dump();

    ++pbCount;
    }

  TRACE("\n");

  TRACE("   Unit size: %10u\n", memBlock::UNIT);

  TRACE("\n");

  TRACE("Protect Type:");
  if ((PAGE_EXECUTE           & protectType())) TRACE(" PAGE_EXECUTE");
  if ((PAGE_EXECUTE_READ      & protectType())) TRACE(" PAGE_EXECUTE_READ");
  if ((PAGE_EXECUTE_READWRITE & protectType())) TRACE(" PAGE_EXECUTE_READWRITE");
  if ((PAGE_EXECUTE_WRITECOPY & protectType())) TRACE(" PAGE_EXECUTE_WRITECOPY");
  if ((PAGE_NOACCESS          & protectType())) TRACE(" PAGE_NOACCESS");
  if ((PAGE_READONLY          & protectType())) TRACE(" PAGE_READONLY");
  if ((PAGE_READWRITE         & protectType())) TRACE(" PAGE_READWRITE");
  if ((PAGE_WRITECOPY         & protectType())) TRACE(" PAGE_WRITECOPY");
  if ((PAGE_GUARD             & protectType())) TRACE(" PAGE_GUARD");
  if ((PAGE_NOCACHE           & protectType())) TRACE(" PAGE_NOCACHE");
  if ((PAGE_WRITECOMBINE      & protectType())) TRACE(" PAGE_WRITECOMBINE");
  TRACE("\n");

  TRACE("\n");

  TRACE("   MemBlocks: %10u\n", memBlock::Count());
  TRACE("  PageBlocks: %10u\n", pageBlock::Count());
  TRACE("    RawHeaps: %10u\n", rawHeap::Count());

  TRACE("\n");

  }


// rawHeap::pageBlockFind ///////////////////////////////////////////////////
//
//    Returns true if pageBlock found.  If prevPP is not NULL, stores pointer
// to pointer to found node (that is, a pointer to the link to the node,
// useful to perform inserts & deletes at this point in the list).
//
bool rawHeap::pageBlockFind(pageBlock &pb, pageBlock ***prevPP) const
  {

  for
    ( pageBlock *const *prevP = &_pbList
    ; NULL != *prevP
    ; prevP = &(*prevP)->next()
    )
    if (&pb == *prevP)
      {
      if (NULL != prevPP)
        *prevPP = const_cast<pageBlock **>(prevP);

      return true;
      }

  return false;
  }


void rawHeap::pageBlockAdd(pageBlock &pb)
  {

  if (pageBlockFind(pb) || NULL != pb.next())
    {
    TRACE("Error: rawHeap::pageBlockAdd: Cannot add page block to heap.\n");
    THROW_LOGIC_EXCEPTION();
    }

  pb.next() = _pbList;
  _pbList   = &pb;

  }


void rawHeap::pageBlockRemove(pageBlock &pb)
  {
  pageBlock **prevP;

  if (!pageBlockFind(pb, &prevP))
    {
    TRACE("Error: rawHeap::pageBlockRemove: Page block not found in heap.\n");
    THROW_LOGIC_EXCEPTION();
    }

  *prevP    = pb.next();
  pb.next() = NULL;

  }


// rawHeap::alloc ///////////////////////////////////////////////////////////
//
//    Note memP must be NULL when called.
//
status rawHeap::alloc(uint size, uint8 *&memP)
  {

  if (NULL != memP)
    {
    TRACE("Error: rawHeap::alloc: Receiving pointer not NULL.\n");
    THROW_LOGIC_EXCEPTION();
    }

  // ask each existing page block, if any, to allocate requested space
  //
  for (pageBlock *pb = pageBlockFirst(); NULL != pb; pb = pb->next())
    if (pb->alloc(size, memP))
      return SUCCESS_S;

  // if request not satisfied, allocate a new page block large enough to
  // accomodate request, then request space from it

  pageBlock &newPB(*new pageBlock(*this, size, _protectType));

  return newPB.alloc(size, memP);
  }


uint8 *rawHeap::alloc(uint size)
  {
  uint8 *mem = NULL;

  if (!alloc(size, mem))
    {
    TRACE("Error: rawHeap::alloc: Memory allocation failed.\n");
    THROW_RESOURCE_EXCEPTION();
    }

  return mem;
  }


// dynFunc class implementation /////////////////////////////////////////////

// dynFunc data .............................................................

rawHeap dynFunc::_Heap;


// dynFunc services .........................................................

// dynFunc::BuildingQ ///////////////////////////////////////////////////////
//
//    Convenience service meant to be used in dynFunc-making functions.  A
// while loop that injects code into a mem stream can use BuildingQ() as the
// loop condition.  The first time through the loop, the mem stream will be
// nullQ(), and BuildingQ() will return 'true'.  The second time, BuildingQ
// will allocate enough space for the code based on the size measurement made
// by the 'dry run' loop iteration, set the mem stream to point to this
// space, and return 'true' again.  The third time the test is made,
// BuildingQ() will return 'false' because the dynFunc is complete and the
// loop will exit.
//
bool dynFunc::BuildingQ(memStream &ms)
  {

  if (!ms.nullQ())
    return false;

  if (ms.size())
    ms = Alloc(ms.size());

  return true;
  }


// closure class implementation /////////////////////////////////////////////

// closure services .........................................................

// closure::Std0RawClosure (1 of 5) /////////////////////////////////////////
//
//    Returns a stdcall function that calls the fastcall function 'func' with
// the two dword parameters 'a' and 'b'.  The actual signature (as opposed to
// the zero-parameter return type signature) of the returned function will be
// the same as func's sans the first two dword-sized parameters (which need
// not be the first two parameters).  Note parameters smaller than a dword
// should be considered equivalent to dword-sized.
//
std0Call closure::Std0RawClosure(fast2Call func, void *a, void *b)
  {
  memStream ms;

  while (BuildingQ(ms))
    {
    ms.mov_edx  (b);                    // Pass dword argument via register
    ms.mov_ecx  (a);                    // Pass dword argument via register
    ms.jmp      (func);                 // Pass control to function address
    }

  return (std0Call )ms.base();
  }


// closure::Std0RawClosure (2 of 5) /////////////////////////////////////////
//
//    Returns a stdcall function that calls the fastcall function 'func' with
// the three parameters 'a', 'b' and 'c'.  The actual signature (as opposed
// to the zero-parameter return type signature) of the returned function will
// be the same as func's sans the first three parameters, which must all be
// dword-sized.  Note parameters smaller than a dword should be considered
// equivalent to dword-sized.
//
std0Call closure::Std0RawClosure(fast3Call func, void *a, void *b, void *c)
  {
  memStream ms;

  while (BuildingQ(ms))
    {
    ms.pop_eax  ();                     // Remove return address from stack
    ms.push     (c);                    // Pass dword argument via stack
    ms.mov_edx  (b);                    // Pass dword argument via register
    ms.mov_ecx  (a);                    // Pass dword argument via register
    ms.push_eax ();                     // Restore return address to stack
    ms.jmp      (func);                 // Pass control to function address
    }

  return (std0Call )ms.base();
  }


// closure::Std0RawClosure (3 of 5) /////////////////////////////////////////
//
//    Returns a stdcall function that calls the stdcall function 'func' with
// the parameter 'a'.  The actual signature (as opposed to the zero-parameter
// return type signature) of the returned function will be the same as func's
// sans the first parameter, which must be dword-sized.  Note parameters
// smaller than a dword should be considered equivalent to dword-sized.
//
std0Call closure::Std0RawClosure(std1Call func, void *a)
  {
  memStream ms;

  while (BuildingQ(ms))
    {
    ms.pop_eax  ();                     // Remove return address from stack
    ms.push     (a);                    // Pass dword argument via stack
    ms.push_eax ();                     // Restore return address to stack
    ms.jmp      (func);                 // Pass control to function address
    }

  return (std0Call )ms.base();
  }


// closure::Std0RawClosure (4 of 5) /////////////////////////////////////////
//
//    Returns a stdcall function that calls the fastcall function 'func' with
// the two parameters 'a' and 'b'.  The actual signature (as opposed to the
// zero-parameter return type signature) of the returned function will be the
// same as func's sans the first two parameters, which must both be dword-
// sized.  Note parameters smaller than a dword should be considered
// equivalent to dword-sized.
//
std0Call closure::Std0RawClosure(std2Call func, void *a, void *b)
  {
  memStream ms;

  while (BuildingQ(ms))
    {
    ms.pop_eax  ();                     // Remove return address from stack
    ms.push     (b);                    // Pass dword argument via stack
    ms.push     (a);                    // Pass dword argument via stack
    ms.push_eax ();                     // Restore return address to stack
    ms.jmp      (func);                 // Pass control to function address
    }

  return (std0Call )ms.base();
  }


// closure::Std0RawClosure (5 of 5) /////////////////////////////////////////
//
//    Returns a stdcall function that calls the fastcall function 'func' with
// the three parameters 'a', 'b' and 'c'.  The actual signature (as opposed
// to the zero-parameter return type signature) of the returned function will
// be the same as func's sans the first three parameters, which must all be
// dword-sized.  Note parameters smaller than a dword should be considered
// equivalent to dword-sized.
//
std0Call closure::Std0RawClosure(std3Call func, void *a, void *b, void *c)
  {
  memStream ms;

  while (BuildingQ(ms))
    {
    ms.pop_eax  ();                     // Remove return address from stack
    ms.push     (c);                    // Pass dword argument via stack
    ms.push     (b);                    // Pass dword argument via stack
    ms.push     (a);                    // Pass dword argument via stack
    ms.push_eax ();                     // Restore return address to stack
    ms.jmp      (func);                 // Pass control to function address
    }

  return (std0Call )ms.base();
  }


// private services ---------------------------------------------------------


// debug private services ...................................................


/* junkyard -----------------------------------------------------------------


end of file -------------------------------------------------------------- */

