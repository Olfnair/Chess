#include "Thread.h"

// semaphore
semaphore::semaphore(int count) : next_ticket(0), ticket_turn(0), count(count)
{
}
void semaphore::wait()
{
  std::unique_lock<std::mutex> lock(mutex);
  unsigned int my_ticket = next_ticket++;
  while (count <= 0 || my_ticket != ticket_turn)
  {
    cv.wait(lock);  // libère le mutex pendant l'attente
  }
  --count;
  ++ticket_turn;
}
void semaphore::signal()
{
  std::unique_lock<std::mutex> lock(mutex);
  ++count;
  cv.notify_all();
}
// fin semaphore

// VTask
bool VTask::isRunning() const
{
  return running;
}
bool VTask::isOver() const
{
  return over;
}
void VTask::halt()
{
  over = true;
}
void VTask::signal()
{
  sem.signal();
}
void VTask::lock()
{
  task_mutex.lock();
}
void VTask::unlock()
{
  task_mutex.unlock();
}

// protected:
VTask::VTask() : running(false), over(false)
{
}
void VTask::wait()
{
  sem.wait();
}
void VTask::begin()
{
  over = false;
  running = true;
}
void VTask::end()
{
  running = false;
  over = true;
}
// fin VTask
