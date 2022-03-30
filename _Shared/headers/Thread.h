#ifndef THREAD_H
#define THREAD_H

#include <atomic>

#ifdef __MINGW32__
#include <mingw.condition_variable.h>
#include <mingw.mutex.h>
#include <mingw.thread.h>
#else
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

class semaphore
{
 public:
  semaphore(int count = 0);
  ~semaphore() = default;
  semaphore(const semaphore&) = delete;
  semaphore(semaphore&&) = delete;
  void wait();
  void signal();

 private:
  unsigned int next_ticket;
  unsigned int ticket_turn;
  int count;
  std::mutex mutex;
  std::condition_variable cv;
};

template<typename Type> struct __ret_value_container
{
  std::atomic<Type> data;
};
template<> struct __ret_value_container<void>
{
};

class ITask
{
 public:
  virtual bool isRunning() const = 0;
  virtual bool isOver() const = 0;
  virtual void halt() = 0;
  virtual void signal() = 0;

  virtual ~ITask() = default;
  ITask(const ITask&) = delete;
  ITask(ITask&&) = delete;

 protected:
  ITask() = default;
};

class VTask : public ITask
{
 public:
  virtual ~VTask() = default;
  VTask(const VTask&) = delete;
  VTask(VTask&&) = delete;
  virtual void run() = 0;
  virtual bool isRunning() const;
  virtual bool isOver() const;
  virtual void halt();
  virtual void signal();
  void lock();
  void unlock();

 protected:
  VTask();
  void wait();
  void begin();
  void end();

  std::mutex task_mutex;

 private:
  semaphore sem;
  std::atomic<bool> running;
  std::atomic<bool> over;
};

template<typename T> class Task : public VTask
{
 public:
  Task() = default;
  virtual ~Task() = default;
  Task(const Task&) = delete;
  Task(Task&&) = delete;
  virtual void run()
  {
    begin();
    ret_value.data = main();
    end();
  }
  T getReturnValue() const
  {
    return ret_value.data;
  }

 protected:
  virtual T main() = 0;

 private:
  __ret_value_container<T> ret_value;
};

template<> void Task<void>::getReturnValue() const = delete;
template<> inline void Task<void>::run()
{
  begin();
  main();
  end();
}

template<typename Task> class Thread : public Task
{
 public:
  Thread() : Task(), started(false)
  {
  }
  template<typename... Args> Thread(Args... args)
      : Task(args...), started(false)
  {
  }
  ~Thread()
  {
    join();
  }
  Thread(const Thread&) = delete;
  Thread(Thread&&) = delete;
  void join()
  {
    if (started && t.joinable())
    {
      t.join();
      started = false;
    }
  }
  void detach()
  {
    if (started)
      t.detach();
  }
  bool hasStarted() const
  {
    return started;
  }
  bool start()
  {
    if (started)
      return false;
    started = true;
    t = std::thread(&this->run, this);
    return true;
  }

 private:
  std::thread t;
  std::atomic<bool> started;
};

#endif  // THREAD_H
