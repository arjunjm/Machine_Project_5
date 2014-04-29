#include "file_system.H"
#include "console.H"

/*
 * File class function definitions
 */
File::File()
{
    /*
     * Initialize file attributes
     */
    current_position = 0;
    current_block = -1;
    starting_block = -1;
    file_size = 0;
    file_id = -1;
    current_block_index = 1;
    file_blocks = NULL;
}

unsigned int File::Read(unsigned int n, char * buffer)
{
    if (current_block == -1 || starting_block == -1)
    {
        Console::puts("File has not been initialized\n");
        return 0;
    }

    file_blocks = file_system->GetFileBlocks(file_id);
    int number_of_char_read = 0;
    current_block = file_blocks[current_block_index - 1];
    char block_data[BLOCK_SIZE];
    file_system->disk->read(current_block, block_data);

    while(n > 0 && !EoF())
    {
        buffer[number_of_char_read] = block_data[current_position];
        number_of_char_read++;
        current_position++;
        if (current_position >= BLOCK_SIZE)
        {
            current_position = 0;
            current_block_index++;
            if (current_block_index >= 10)
            /*
             * A file can contain only 10 blocks
             */
            {
                return number_of_char_read;
            }
            file_blocks = file_system->GetFileBlocks(file_id);
            current_block = file_blocks[current_block_index - 1];
            file_system->disk->read(current_block, block_data);
        }
        n--;
    }
    return number_of_char_read;
}

unsigned int File::Write(unsigned int n, char * buffer)
{
    if (current_block == -1 || starting_block == -1)
    {
        Console::puts("File has not been initialized\n");
        return 0;
    }

    int position  = current_position; 
    int block_to_write = current_block;
    int char_written = 0;
    char curr_block_data[BLOCK_SIZE];
    file_system->disk->read(block_to_write, curr_block_data);
    while (char_written < n || buffer[char_written] == '\0')
    {
        curr_block_data[position] = buffer[char_written];
        position++;
        char_written++;
        if(position >= BLOCK_SIZE)
        {
            position = 0;
            file_system->disk->write(block_to_write, curr_block_data);
            int new_block_number = file_system->GetFreeBlockNumber(); 
            file_system->UpdateINodeWithNewBlockNumber(file_id, new_block_number);
            file_system->UpdateINodeWithNewFileSize(this, file_id, BLOCK_SIZE - current_position); 
            block_to_write = new_block_number;
            file_system->disk->read(block_to_write, curr_block_data);
        }
    }

    file_system->UpdateINodeWithNewFileSize(this, file_id, position); 
    // indicates EOF
    curr_block_data[position++] = -1;
    file_system->disk->write(block_to_write, curr_block_data);
    return char_written;
}

void File::Reset()
{
    current_position = 0;
    current_block = starting_block;
}

void File::Rewrite()
{
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    for (int i = 0; i < 10; i++)
    {
        if (file_blocks[i] != 0)
        {
            file_system->disk->write(file_blocks[i], buf);
            file_system->ReleaseBlock(file_blocks[i]);
            file_blocks[i] = 0;
        }
        else
        {
            break;
        }
    }
}

BOOLEAN File::EoF()
{
    char block_data[BLOCK_SIZE];
    file_system->disk->read(current_block, block_data);
    if(block_data[current_position] == -1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void File::PrintFileAttributes()
{

    Console::puts("\nFile ID: ");
    Console::puti(file_id);
    Console::puts("\n");

    Console::puts("File Size : ");
    Console::puti(file_size);
    Console::puts("\n");


    Console::puts("Starting Block : ");
    Console::puti(starting_block);
    Console::puts("\n");

    Console::puts("Current Block : ");
    Console::puti(current_block);
    Console::puts("\n");

    Console::puts("Current Position : ");
    Console::puti(current_position);
    Console::puts("\n\n");

}
/*
 * Filesystem function defintions
 */

/*
 * Defining static variables
 */

SimpleDisk * FileSystem::disk;
unsigned int FileSystem::size;
BOOLEAN FileSystem::is_mounted;
unsigned long* FileSystem::free_block_map;
unsigned long FileSystem::number_of_blocks;
unsigned long FileSystem::number_of_inodes;
unsigned long FileSystem::inode_mgmt_blocks;

FileSystem::FileSystem()
{
    FileSystem::disk = NULL;
    is_mounted = FALSE;
    size = 0;
}

BOOLEAN FileSystem::Mount(SimpleDisk *_disk)
{
    if (_disk && !is_mounted)
    {
        FileSystem::is_mounted = TRUE;
        FileSystem::disk = _disk;
        return TRUE;
    }
    else
        return FALSE;
}

BOOLEAN FileSystem::Format(SimpleDisk *_disk, unsigned int size)
{
    if (size > MAX_DISK_SIZE)
    {
        Console::puts("File system size cannot exceed maximum disk size! Exiting\n");
        return FALSE;
    }
    FileSystem::disk = _disk;
    FileSystem::size = size;
    FileSystem::number_of_blocks = size / BLOCK_SIZE;
    memset(free_block_map, 0, ((FileSystem::number_of_blocks)/32) * sizeof(unsigned long));

    char buffer[BLOCK_SIZE];
    memset(buffer, 0, BLOCK_SIZE);

    for(int i = 0 ; i < FileSystem::number_of_blocks; i++)
    {
        FileSystem::disk->write(i, buffer);
    }
    
    // This is a design choice.
    FileSystem::number_of_inodes = FileSystem::number_of_blocks / 10;

    /*
     * We require some blocks to store the inode information. 
     * We are using the first few blocks in the filesystem for the 
     * management information of inodes
     */
    FileSystem::inode_mgmt_blocks = (number_of_inodes * sizeof(INode_T)) / BLOCK_SIZE ;
    
    int number_of_full_frames = inode_mgmt_blocks / 32;
    int left_over = inode_mgmt_blocks % 32;
    
    int i = 0;
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
                //Initialize the passed file object
                file->current_position = 0;
                file->file_size = 0;
                file->current_block_index = 1;

                file->starting_block = inode_list[j].block_no[0];
                file->current_block  = inode_list[j].block_no[0];
                file->file_id = file_id;
                file->file_system = this;
                file->file_blocks = GetFileBlocks(file_id); 

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
        return FALSE;
    }

    File *file;
    if(LookupFile(file_id, file))
    {
        Console::puts("File already exists! Not creating again\n");
        return FALSE;
    }
    
    char buffer[BLOCK_SIZE];
    for(int i = 0; i < inode_mgmt_blocks; i++)
    {
        disk->read(i, buffer); 
        INode_T *inode_list = (INode_T*)buffer;
        int number_of_inodes_in_list = BLOCK_SIZE/sizeof(INode_T);
        for (int j = 0; j < number_of_inodes_in_list; j++)
        {
            if (inode_list[j].file_id == 0)
            {
                inode_list[j].file_id   = file_id;
                inode_list[j].file_size = 0;
                inode_list[j].block_no[0] = GetFreeBlockNumber();
                inode_list[j].number_of_blocks_used = 0;
                disk->write(i, buffer);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOLEAN FileSystem::DeleteFile(int file_id)
{
    if(!is_mounted || file_id == 0)
    {
        Console::puts("File system not mounted or invalid file ID!! Returning\n");
        return FALSE;
    }

    char buffer[BLOCK_SIZE];
    for(int i = 0; i < inode_mgmt_blocks; i++)
    {
        disk->read(i, buffer); 
        INode_T *inode_list = (INode_T*)buffer;
        int number_of_inodes_in_list = BLOCK_SIZE/sizeof(INode_T);
        for (int j = 0; j < number_of_inodes_in_list; j++)
        {
            if (inode_list[j].file_id == file_id)
            {
                inode_list[j].file_id   = 0;
                inode_list[j].file_size = 0;
                for (int k = 0; k < 10; k++)
                {
                    if (inode_list[j].block_no[k])
                        ReleaseBlock(inode_list[j].block_no[k]);
                    inode_list[j].block_no[k] = 0;
                }
                inode_list[j].number_of_blocks_used = 0;
                disk->write(i, buffer);
                return TRUE;
            }
        }
    }
    return FALSE;
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

void FileSystem::ReleaseBlock(unsigned long block_no)
{
    int bit_position = block_no % 32;
    int index_of_frame = block_no / 32;
    if (IS_SET (free_block_map[index_of_frame], bit_position))
    {
        free_block_map[index_of_frame] = TOGGLE_BIT(free_block_map[index_of_frame], bit_position);
    }
}

unsigned int* FileSystem::GetFileBlocks(int file_id)
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
                return inode_list[j].block_no;
            }
        }
    }
    return NULL;
   
}

void FileSystem::UpdateINodeWithNewBlockNumber(int file_id, int block_no)
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
                inode_list[j].number_of_blocks_used++;
                inode_list[j].block_no[inode_list[j].number_of_blocks_used] = block_no;
                disk->write(i, buffer);
            }
        }
    }
}


void FileSystem::UpdateINodeWithNewFileSize(File *_file, int file_id, int file_size_increase)
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
                inode_list[j].file_size += file_size_increase;
                _file->file_size = inode_list[j].file_size;
                disk->write(i, buffer);
            }
        }
    }
}
