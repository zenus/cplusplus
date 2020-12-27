#include<iostream>
#include<memory>
#include<assert.h>
#include<sched.h>
#include<algorithm>
using namespace std;

#define AtomicAdd(a_ptr,a_count) __sync_fetch_and_add (a_ptr, a_count)
/// @brief atomically substracts a_count from the variable pointed by a_ptr
/// @return the value that had previously been in memory
#define AtomicSub(a_ptr,a_count) __sync_fetch_and_sub (a_ptr, a_count)

/// @brief Compare And Swap
///        If the current value of *a_ptr is a_oldVal, then write a_newVal into *a_ptr
/// @return true if the comparison is successful and a_newVal was written
#define CAS(a_ptr, a_oldVal, a_newVal) __sync_bool_compare_and_swap(a_ptr, a_oldVal, a_newVal)


template<typename T, uint32_t S>
class lockFreeQueue
{
  public:
   lockFreeQueue();
   virtual ~lockFreeQueue();
   uint32_t size();

   bool push(const T& x);
   bool pop( T& x);
  private:
    T m_queue[SIZE];
    volatile uint32_t m_count;
    volatile uint32_t m_writeIndex;
    volatile uint32_t m_readIndex;

    /// @brief maximum read index for multiple producer queues
    /// If it's not the same as m_writeIndex it means
    /// there are writes pending to be "committed" to the queue, that means,
    /// the place for the data was reserved (the index in the array) but  
    /// data is still not in the queue, so the thread trying to read will have 
    /// to wait for those other threads to save the data into the queue
    ///
    /// note this index is only used for MultipleProducerThread queues
    volatile uint32_t m_maximumReadIndex;

    /// @brief calculate the index in the circular array that corresponds
    /// to a particular "count" value
    inline uint32_t countToIndex(uint32_t a_count);
};

  template<typename T, uint32_t S>
  lockFreeQueue<T,S>::lockFreeQueue():
  m_writeIndex(0),
  m_readIndex(0),
  m_maximumReadIndex(0),
  m_count(0)
  {}

  template<typename T, uint32_t S>
  lockFreeQueue<T,S>::~lockFreeQueue(){}

  template<typename T, uint32_t S>
  inline unint32_t lockFreeQueue<T,S>::countToIndex(uint32_t a_count)
  {
    return (a_count % S);
  }

  template<typename T, uint32_t S>
  unint32_t lockFreeQueue<T,S>::size()
  {
    return m_count;
  }

  template<typename T, uint32_t S>
  bool lockFreeQueue<T,S>::push(const T& x)
  {
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;
    do{
      currentReadIndex = m_readIndex;
      currentWriteIndex = m_writeIndex;
      if(countToIndex(currentWriteIndex+1) ==  countToIndex(currentReadIndex))
      {
        //the queue is full
        return false;
      }

    }while(!CAS(&m_writeIndex,currentWriteIndex,(currentWriteIndex+1)))

    //we know that this index is reserved for us, Use it to save the data
    m_queue[countToIndex(currentWriteIndex)] = x;

    //update the maximum read index after saving the data, it wouldn't fail if there is only one thread
    // insert in the queue. it may fail if there are more than 1 producer threads because this operation
    // has to be done in the same order as the previous CAS
    while(!CAS(&m_maximumReadIndex,currentWriteIndex,(currentWriteIndex+1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
      sched_yield();
    }
    AtomicAdd(&m_count,1);
    return true;
  }

  template<typename T, uint32_t S>
  bool lockFreeQueue<T,S>::pop(T& x)
  {
    uint32_t  currentMaximumReadIndex;
    uint32_t  currentReadIndex;
    do{
        currentReadIndex = m_readIndex;
        currentMaximumReadIndex = m_maximumReadIndex;
        if(countToIndex(currentReadIndex) == countToIndex(currentMaximumReadIndex))
        {
            // the queue is empty or
              // a producer thread has allocate space in the queue but is 
              // waiting to commit the data into it
          return false;
        }
                // retrieve the data from the queue
        x = m_queue[countToIndex(currentReadIndex)];
          // try to perfrom now the CAS operation on the read index. If we succeed
          // a_data already contains what m_readIndex pointed to before we 
          // increased it
        if(CAS(&m_readIndex,currentReadIndex,(currentReadIndex+1)))
        {
          AtomicSub(&m_count,1);
          return true;
        }

    }while(1)

    // Something went wrong. it shouldn't be possible to reach here
    assert(0);

    // Add this return statement to avoid compiler warnings
    return false;
  }
