#include "utils.H"
#include "Scheduler.H"
#include "console.H"
#include "assert.H"
#include "simple_disk.H"
#include "blocking_disk.H"

extern BlockingDisk * SYSTEM_DISK;

Scheduler::Scheduler()
{
      front_index = 0;
      next_index = 0;
      for (int i=0; i< MAX_QUEUE_SIZE; i++)
      ready_Queue[i] = NULL;
}

void Scheduler::yield()
{
   static int tempcount = 0;
   if((front_index == next_index) && (ready_Queue[front_index] == NULL))
   {
       Console::puts("\nReady Queue is empty\n");
   }
   else
   {
       //printQueue();
       //if (tempcount == 8)
       //    while(true);
       tempcount++;
       Thread *ready_thread = ready_Queue[front_index];
       if(ready_thread != NULL)
       {
           threadInfoStructT **blocked_thread_queue = SYSTEM_DISK->get_blocked_thread_queue();
           for (int i = 0; i < MAX_QUEUE_SIZE; i++)
           {
                Thread * thread = blocked_thread_queue[i]->thread;
                unsigned long block_no = blocked_thread_queue[i]->block_no;
                char * buffer = blocked_thread_queue[i]->buffer;
                DISK_OPERATION thread_oper = blocked_thread_queue[i]->disk_oper;

                /*
                if (thread == Thread::CurrentThread())
                {
                    Console::puts("Continuing..\n");
                }*/
                if (thread == ready_thread)
                {
                   if (SYSTEM_DISK->is_device_ready())
                   {
                       if (thread_oper == READ)
                       {
                           SYSTEM_DISK->read_from_port(block_no, buffer);
                       }
                       else if (thread_oper == WRITE)
                       {
                           SYSTEM_DISK->write_to_port(block_no, buffer);
                       }
                   }
                   else
                   {
                       resume(thread);
                       yield();
                   }
                }
                
           }
           Thread::dispatch_to(ready_thread);
           front_index = (front_index +1 ) % MAX_QUEUE_SIZE;
       }
   }


}

void Scheduler::add(Thread *_thread)
{
   Console::puts("Adding a thread into the readyQueue");
   // we call the resume function
   resume(_thread);

}

void Scheduler::resume(Thread *_thread)
{
    if ( (next_index == front_index) && (ready_Queue[front_index] !=NULL))
    {
       Console::puts("ready Queue is full need to increase size\n");  
    }
    else
    {
       // Check if thread already exists in the queue.
       for (int i = front_index; i != next_index; i = (i+1) % MAX_QUEUE_SIZE)
       {
           if (ready_Queue[i] == _thread)
           {
               Console::puts("Thread already exists in the ready queue. Not scheduling again");
               return;
           }
       }
       ready_Queue[next_index] = _thread;
       next_index = (next_index + 1) % MAX_QUEUE_SIZE;
    }
}

void Scheduler::terminate(Thread *_thread)
{
   int i=0,j=0,count=0;
   i = front_index;

   Console::puts(" terminate is called\n"); 
 //  count = MAX_QUEUE_SIZE;

   while (ready_Queue[i] != NULL && count < MAX_QUEUE_SIZE) 
   {

      if(ready_Queue[i] == _thread)
           break;

       i = (i + 1) % MAX_QUEUE_SIZE ;
       count++; 
   }

   if (count == MAX_QUEUE_SIZE)
   {
       Console::puts(" No thread found with that id list is full\n");  
   }
   else
   {
       if (ready_Queue[i] == NULL)
       {

           Console::puts(" No thread found with that id\n");  
       }    
       else // thread found
       {
       	   // copy the elements from next position to this position
          
           j = (i+1) % MAX_QUEUE_SIZE; 
           while (j != next_index)
           {

               ready_Queue[i] = ready_Queue[j];
               j = (i+1) % MAX_QUEUE_SIZE; 
               i = (i+1) % MAX_QUEUE_SIZE;
           }
           ready_Queue[i] = NULL;
           next_index = i;
       }

   }
 
}

void Scheduler::printQueue()
{
    for (int i = front_index; i != next_index; i = (i+1)%MAX_QUEUE_SIZE)
    {
        Thread *thread = ready_Queue[i];
        if (thread != NULL)
        {
            Console::puti(thread->ThreadId());
            Console::puts(" ");
        }
        else
        {
            Console::puts(" NULL "); 
        }
    }
}
