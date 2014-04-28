#include "file_system.H"

/*
 * File class function definitions
 */

File::File()
{
    current_position = 0;
}

unsigned int File::Read(unsigned int n, char * buffer)
{
}

unsigned int File::Write(unsigned int n, char * buffer)
{
}

void File::Reset()
{
    current_position = 0;
}

void File::Rewrite()
{
}

BOOLEAN File::EoF()
{
}

/*
 * Filesystem function defintions
 */

FileSystem::FileSystem()
{
    this->disk = NULL;
    is_mounted = FALSE;
    size = 0;
}

BOOLEAN FileSystem::Mount(SimpleDisk *_disk)
{
    if (_disk && !is_mounted)
    {
        this->is_mounted = TRUE;
        this->disk = _disk;
        return TRUE;
    }
    else
        return FALSE;
}

BOOLEAN FileSystem::Format(SimpleDisk *_disk, unsigned int size)
{
    if (size > SYSTEM_DISK_SIZE)
    {
        Console::puts("File system size cannot exceed maximum disk size! Exiting\n");
        return FALSE;
    }
    this->disk = _disk;
    this->size = size;
    this->number_of_blocks = size / BLOCK_SIZE;
    memset(free_block_map, 0, ((this->number_of_blocks)/32) * sizeof(unsigned long));

    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);

    for(int i = 0 ; i < this->number_of_blocks; i++)
    {
        this->disk->write(i, buffer);
    }
    
    // This is a design choice.
    this->number_of_inodes = this->number_of_blocks / 10;

    /*
     * We require some blocks to store the inode information. 
     * We are using the first few blocks in the filesystem for the 
     * management information of inodes
     */
    this->inode_mgmt_blocks = (number_of_inodes * sizeof(INode_T)) / BLOCK_SIZE ;
    
    int number_of_full_frames = inode_mgmt_blocks / 32;
    int left_over = inode_mgmt_blocks % 32;
    
    int i;
    for (i = 0; i < number_of_full_frames; i++)
        free_block_map[i] = 0xFFFFFFFF;

    for (int j = 0; j < left_over; j++)
        free_block_map[i] = SET_BIT(free_block_map[i], j);

    return TRUE;
}

BOOLEAN FileSystem::LookupFile(int file_id, File *file)
{
    char buffer[BLOCK_SIZE];
    for (int i = 0; i < inode_mgmt_blocks; i++)
    {
        disk->read(i, buffer);
        INode_T *inode_list = (INode_T*) buffer;
        int number_of_inodes_in_list = BLOCK_SIZE/sizeof(INode_T);
        for (int j = 0; j < number_of_inodes_in_list; j++)
        {
            if (inode_list[j].file_id == file_id)
            {
                //TODO
                //Initialize the passed file object
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOLEAN FileSystem::CreateFile(int file_id)
{
    if(!is_mounted || file_id == 0)
    {
        Console::puts("File system not mounted or invalid file ID!! Returning\n");
        return;
    }

    File *file;
    if(LookupFile(file_id, file))
    {
        Console::puts("File already exists! Not creating again\n");
        return;
    }
    
    char buffer[BLOCK_SIZE];
    for(int i = 0; i < inode_mgmt_blocks; i++)
    {
        disk->read(i, buffer); 
    }
}

BOOLEAN FileSystem::DeleteFile(int file_id)
{
}

unsigned long FileSystem::GetFreeBlockNumber()
{
    BOOLEAN block_found = false;
    int i, j;
    unsigned long block_number;
    for ( i = 0 ; i < number_of_blocks / 32; i++)
    {
        if (free_block_map[i] != 0xFFFFFFFF)
        {
            for ( j = 0; j <= 31; j++ )
            {
                if (IS_SET(free_block_map[i], j))
                    continue;
                else
                {
                    free_block_map[i] = TOGGLE_BIT(free_block_map[i], j);
                    block_found = true;
                    break;
                }
            }
            if (block_found)
                break;
        }
    }
    if (block_found)
    {
        block_number = j + i*32;
        return block_number;
    }
    else
    {
        return 0;
    }
} 
