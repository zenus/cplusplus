#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <future>
#include <functional>
#include <type_traits>
using namespace std;

class ThreadPool {
  public:
     ThreadPool(size_t);
     template<typename F, typename... Args>
     auto enqueue(F&& f, Args&&... args)->std::future<decltype(f(args...))>;
     ~ThreadPool();
     private:
        std::vector<thread> workers;
        std::queue<std::function<void()>> tasks;

        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
};

inline ThreadPool::ThreadPool(size_t threads) : stop(false){

  for(size_t i=0; i<threads;++i)
  {
    workers.emplace_back(
      [this]{
        for(;;)
        {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(queue_mutex);

            //当匿名函数返回false时才阻塞线程，阻塞时自动释放锁
            //当匿名函数返回true时且收到通知时解阻塞，然后枷锁
            condition.wait(lock,[this]{return stop || !tasks.empty(); });
            if(stop && tasks.empty())
            {
              return;
            }

            task = std::move(tasks.front());
            tasks.pop();
          }
          task();
        }
      }
    );
  }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<decltype(f(args...))>
{

   /***
    * 
    * C++11中的std::packaged_task是个模板类。std::packaged_task包装任何可调用目标(函数、lambda表达式、bind表达式、函数对象)以便它可以被异步调用。它的返回值或抛出的异常被存储于能通过std::future对象访问的共享状态中。
      std::packaged_task类似于std::function，但是会自动将其结果传递给std::future对象。
    * ***/
  auto task = std::make_shared<std::packaged_task<decltype(f(args...))()>>(
    std::bind(std::forward<F>(f), std::forward<Args>(args)...)
  );

  // std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    if(stop) throw std::runtime_error("enqueue on stopped pool!");
    tasks.emplace([task](){ (*task)(); });
  }
   condition.notify_one();
   return task->get_future();
}

inline ThreadPool::~ThreadPool()
{
  {
    std::unique_lock<std::mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  for(std::thread& worker : workers )
      worker.join();
}
