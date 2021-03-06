/* 
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/04/05

    Description: File System.
    

*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB * (0x1 << 20)
#define KB * (0x1 << 10)
#define MAX_DISK_SIZE ((10 MB)) 
#define BLOCK_SIZE 512
#define MAX_NUMBER_OF_BLOCKS ( (MAX_DISK_SIZE) / (BLOCK_SIZE) )

#define IS_SET(var, pos) ((var) & (1<<(pos)))
#define SET_BIT(var, pos) ((var) |= (1<<(pos)))
#define TOGGLE_BIT(var, pos) ((var) ^= (1<<(pos)))


/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/
typedef struct INode
{
    unsigned int file_id;
    unsigned int file_size;
    unsigned int block_no[10];
    unsigned int number_of_blocks_used;
}INode_T;

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

class FileSystem;

/*--------------------------------------------------------------------------*/
/* F i l e */
/*--------------------------------------------------------------------------*/

class File {
    friend class FileSystem;
private: 
     /* -- DEFINE YOUR FILE DATA STRUCTURES HERE. */
     
     unsigned int current_position;
     unsigned long starting_block;
     unsigned long current_block;
     unsigned int  current_block_index;
     unsigned int * file_blocks;
     unsigned int file_size;
     FileSystem   * file_system;
     char cached_block[BLOCK_SIZE];
     unsigned int   file_id;

     /* -- You may want to store other information, such as 
             .. position in the file
             .. cached block(s)
             .. etc.
      */

public:

    File();
    /* Constructor for the file handle. Set the 'current
       position' to be at the beginning of the file. */

    unsigned int Read(unsigned int _n, char * _buf);
    /* Read _n characters from the file starting at the 
       current location and copy them in _buf.
       Return the number of characters read. */

    unsigned int Write(unsigned int _n, char * _buf);
    /* Write _n characters to the file starting at the current 
       location, if we run past the end of file, we increase 
       the size of the file as needed. 
     */

    void Reset();
    /* Set the 'current position' at the beginning of the file. */

    void Rewrite();
    /* Erase the content of the file. Return any freed blocks. 
       Note: This function does not delete the file! It just erases its
       content. */

    BOOLEAN EoF();
    /* Is the current location for the file at the end of the file? */

    /* Helper functions */
    void PrintFileAttributes();

};

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class FileSystem {

    friend class File;

private:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */
     
     static SimpleDisk * disk;
     static unsigned int size;
     static BOOLEAN is_mounted;
     static unsigned long * free_block_map;
     static unsigned long number_of_blocks;
     static unsigned long number_of_inodes;
     static unsigned long inode_mgmt_blocks;
     
public:

   FileSystem();   
   /* Just initializes local data structures. Does not connect to disk yet. */

   BOOLEAN Mount(SimpleDisk * _disk);
   /* Associates the file system with a disk. We limit ourselves to at most one
      file system per disk. Returns TRUE if 'Mount' operation successful (i.e. there
      is indeed a file system on the disk. */

   static BOOLEAN Format(SimpleDisk * _disk, unsigned int _size);
   /* Wipes any file system from the given disk and installs a new, empty, file
      system that supports up to _size Byte. */

   BOOLEAN LookupFile(int _file_id, File * _file);
   /* Find file with given id in file system. If found, initialize the file 
      object and return TRUE. Otherwise, return FALSE. */

   BOOLEAN CreateFile(int _file_id);
   /* Create file with given id in the file system. If file exists already,
      abort and return FALSE. Otherwise, return TRUE. */

   BOOLEAN DeleteFile(int _file_id);
   /* Delete file with given id in the file system and free any disk block
      occupied by the file. */

   /* Helper functions */
   unsigned long GetFreeBlockNumber();
   void ReleaseBlock(unsigned long block_no);

   // Returns the array with the block numbers of the file if the file is found
   // Returns NULL otherwise
   unsigned int* GetFileBlocks(int _file_id);

   // Update INODE with new block number
   void UpdateINodeWithNewBlockNumber(int file_id, int block_no);
   
   // Update INODE with new file size
   void UpdateINodeWithNewFileSize(File *_file, int file_id, int file_size_increase);
};
#endif
