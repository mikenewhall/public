

/* main.cpp *****************************************************************


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

class test3
  {

  public: class method0Closure : public std0Closure
    {
    public: method0Closure(test3 &obj, uint a, uint b)
      : std0Closure((fast3Call )Forwarder, (void *)&obj, (void *)a, (void *)b)
      {}

    public: static void __fastcall Forwarder(test3 &obj, uint a, uint b)
      {obj.method0(a, b);}
    };

  public: method0Closure &method0ClosureCreate(uint a, uint b)
    {return *new method0Closure(*this, a, b);}

  public: void method0(uint a, uint b)
    {
    printf
      ( "[method0 reached! (_m0=0x%08X, a=0x%08X, b=0x%08X)]\n"
      , _m0, a, b
      ) ;
    }

  public: void method1(uint a)
    {
    printf
      ( "[method1 reached! (_m0=0x%08X, a=0x%08X)]\n"
      , _m0, a
      ) ;
    }

  public: int _m0;

  };


// private data -------------------------------------------------------------


// private inline services --------------------------------------------------


// private class implementations --------------------------------------------


// private services ---------------------------------------------------------

static void __fastcall Test2(void *a, void *b, void *c, void *d)
  {

  printf
    ( "[Test2 reached! (a=0x%08X, b=0x%08X, c=0x%08X, d=0x%08X)]"
    , a, b, c, d
    ) ;

  }


static void Test()
  {
  test3 obj;

  test3::method0Closure testClosure3(obj, 0x98765432, 0x34567890);

  test3::method0Closure &testClosure4 = obj.method0ClosureCreate(0xA9876543, 0x4567890F);

  std0Closure
    testClosure5
      ( (fast2Call )test3::method0Closure::Forwarder
      , &obj
      , (void *)0xBA987654
      ) ;

  std0Closure
    testClosure6
      ( (std1Call )testClosure5.func()
      , (void *)0x56789ABC
      ) ;

  closure::Heap().dump();

  printf("\n");

  obj._m0 = 0x01233210;

  printf("Calling closure...\n");
  printf("\n");

  testClosure3();
  printf("\n");

  testClosure4();
  printf("\n");

  testClosure6();
  printf("\n");

  printf("...closure called.\n");

  delete &testClosure4;

  }


// public services ----------------------------------------------------------

int main(int paramCount, char *param[])
  {

  printf("\n");
  printf("Closure\n");
  printf("-------\n");
  printf("\n");

  Test();

  printf("\n");

  printf("\n");
  printf("Remaining  MemBlocks: %2u\n", memBlock::Count());
  printf("Remaining PageBlocks: %2u\n", pageBlock::Count());
  printf("Remaining   RawHeaps: %2u\n", rawHeap::Count());

  printf("\n");

  return 0;
  }


/* junkyard -----------------------------------------------------------------


  std0Closure
    testClosure0
      ( (fast3Call )test3::method0Closure::method0Forwarder
      , &obj
      , (void *)0x76543210
      , (void *)0x01234567
      ) ;

  std0Closure
    testClosure1
      ( (fast2Call )test3::method0Closure::method0Forwarder
      , &obj
      , (void *)0x87654321
      ) ;


  testClosure0();
  printf("\n");

  typedef void (__stdcall *std1Call)(uint);
  ((std1Call )testClosure1.func())(0x12345678);
  printf("\n");

  ((void (__stdcall *)(uint) )testClosure1.func())(0x23456789);
  printf("\n");


end of file -------------------------------------------------------------- */

