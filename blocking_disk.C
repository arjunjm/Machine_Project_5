#include "blocking_disk.H"
#include "console.H"
#include "machine.H"
#include "Scheduler.H"

class Scheduler;

extern Scheduler* SYSTEM_SCHEDULER;

BlockingDisk::BlockingDisk(DISK_ID disk_id, unsigned int size) : SimpleDisk(disk_id, size)
{
    /*
     * Initializing thread queue to NULL
     *
     */
    for (int i = 0 ; i < MAX_SIZE; i++)
    {
        thread_queue[i] = NULL;
    }

    /*
     * Initializing thread count to 0
     */
    front_index = 0;
    rear_index = 0;
    thread_count = 0;

}

void BlockingDisk::read(unsigned long block_no, char * buffer)
{
    issue_operation(READ, block_no);
    Thread * current_thread = Thread::CurrentThread();
    push(current_thread, READ, block_no, buffer);

    // Pass on CPU
    SYSTEM_SCHEDULER->resume(current_thread);
    SYSTEM_SCHEDULER->yield();
}

void BlockingDisk::write(unsigned long block_no, char * buffer)
{
    issue_operation(WRITE, block_no);
    Thread * current_thread = Thread::CurrentThread();
    push(current_thread, WRITE, block_no, buffer);

    // Pass on CPU
    SYSTEM_SCHEDULER->resume(current_thread);
    SYSTEM_SCHEDULER->yield();
}

void BlockingDisk::read_from_port(unsigned long block_no, char * _buf)
{
   /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = inportw(0x1F0);
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
  pop();
}

void BlockingDisk::write_to_port(unsigned long block_no, char * _buf)
{
  /* write data to port */
  int i; 
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    outportw(0x1F0, tmpw);
  }
  pop();
}

threadInfoStructT thread_info_struct;

void BlockingDisk::push(Thread *thread, DISK_OPERATION disk_oper, unsigned long block_no, char *buffer)
{
    if (thread_count > MAX_SIZE)
    {
        Console::puts("Max thread count exceeded \n");
        return;
    }

    thread_info_struct.thread = thread;
    thread_info_struct.disk_oper = disk_oper;
    thread_info_struct.block_no = block_no;
    thread_info_struct.buffer = buffer;

    thread_queue[rear_index] = &thread_info_struct;

    rear_index = (rear_index + 1) % MAX_SIZE;
    thread_count += 1;
}

void BlockingDisk::pop()
{
    if (thread_count == 0)
    {
        Console::puts("No blocked thread in queue \n");
        return;
    }
    thread_queue[front_index] = NULL;
    thread_count -= 1;
    front_index = (front_index + 1) % MAX_SIZE;
}

BOOLEAN BlockingDisk::is_device_ready()
{
    return is_ready();
}

void BlockingDisk::set_thread_to_null(Thread * thread)
{
    if (thread == NULL)
    {
        Console::puts("Thread NULL...returning\n");
        return;
    }

    if (thread_count == 0)
    {
        Console::puts("No blocked thread in queue \n");
        return;
    }
    for (int i = 0 ; i < MAX_SIZE; i++)
    {
        if (thread_queue[i]->thread == thread)
        {
            thread_queue[i] = NULL;
        }
    }
}

threadInfoStructT** BlockingDisk::get_blocked_thread_queue()
{
    return thread_queue;
}    
