#ifndef __BLOCKING_DEV_H__
#define __BLOCKING_DEV_H__

#include "simple_disk.H"
#include "thread.H"

#define MAX_SIZE 20

typedef struct thread_info
{
    Thread * thread;
    DISK_OPERATION disk_oper;
    unsigned long block_no;
    char *buffer;
}threadInfoStructT;

class BlockingDisk : public SimpleDisk
{
    threadInfoStructT * thread_queue[MAX_SIZE];
    int front_index;
    int rear_index;
    int thread_count;

    public:
        BlockingDisk(DISK_ID _disk_id, unsigned int _size);
        /* Creates a SimpleDisk device with the given size connected to the MASTER or
         * SLAVE slot of the primary ATA controller. */
        /* DISK OPERATIONS */
        void read(unsigned long _block_no, char * _buf);
        /* Reads 512 Bytes from the given block of the disk and copies them
         * to the given buffer. No error check! */
        void write(unsigned long _block_no, char * _buf);
        /* Writes 512 Bytes from the buffer to the given block on the disk. */

        void read_from_port(unsigned long _block_no, char *_buf);
        void write_to_port(unsigned long _block_no, char *_buf);

        /* Helper functions */
        /* Push thread to the blocked thread queue */
        void push(Thread *thread, DISK_OPERATION disk_open, unsigned long _block_no, char *buffer);
        
        /* Pop thread from blocked thread queue */
        void pop();

        /* Public routine to call is_ready function of the SimpleDisk class */
        BOOLEAN is_device_ready();

        void set_thread_to_null(Thread* thread);

        /* Return the blocked thread queue */
        threadInfoStructT** get_blocked_thread_queue();
};

#endif /* __BLOCKING_DEV_H__ */ 

