#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
/*=====================================================================================================================================*/
/*=====================================================================================================================================*/
/*=======================================--------------------------------------------------------======================================*/
/*==================================---------------------------Tal Randi------------------------------=================================*/
/*==================================---------------------------Disk Ex--------------------------------=================================*/
/*=======================================--------------------------------------------------------======================================*/
/*=====================================================================================================================================*/
/*=====================================================================================================================================*/
using namespace std;

#define DISK_SIZE 256


/*This function returns a number of free block, and set 1 on it's bit*/
int freeBlock(int *bitVector, int bitVectorsize)
{
    for(int i = 0 ; i < bitVectorsize; i++)
    {
        if(bitVector[i] == 0)
        {
            bitVector[i] = 1;
            return i;
        }
    }
}

/*This function get an integer, and convert it to binary code*/
void decToBinary(int n , char &c) 
{ 
   // array to store binary number 
    int binaryNum[8]; 
  
    // counter for binary array 
    int i = 0; 
    while (n > 0) { 
          // storing remainder in binary array 
        binaryNum[i] = n % 2; 
        n = n / 2; 
        i++; 
    } 
  
    // printing binary array in reverse order 
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j]==1)
            c = c | 1u << j;
    }
 } 

// #define SYS_CALL
// ============================================================================ //
class fsInode {
    int fileSize;
    int block_in_use;

    int *directBlocks;
    int singleInDirect;
    int num_of_direct_blocks;
    int block_size;


    public:
    fsInode(int _block_size, int _num_of_direct_blocks) {
        fileSize = 0; 
        block_in_use = 0; 
        block_size = _block_size;
        num_of_direct_blocks = _num_of_direct_blocks;
        directBlocks = new int[num_of_direct_blocks];
		assert(directBlocks);
        for (int i = 0 ; i < num_of_direct_blocks; i++) {   
            directBlocks[i] = -1;
        }
        singleInDirect = -1;
    }

    /*Getter and setter functions*/
    int getFileSize() {
        return this->fileSize;
    }
    void setFileSize(int filesize){
        this->fileSize = filesize;
    }
    int getBlockInUse(){
        return this->block_in_use;
    }
    void setBlockInUse(int blockInUse){
        this->block_in_use = blockInUse;
    }
    int getSingleInDirect(){
        return this->singleInDirect;
    }
    void setSingleInDirect(int blockNumber){
        this->singleInDirect = blockNumber;
    }
    int getDirectBlocks(int index)
    {
        return this->directBlocks[index];
    }
    void setDirectBlocks(int index, int value)
    {
        directBlocks[index] = value;
    }

    /*Destructor*/
    ~fsInode() { 
        delete directBlocks;
    }

};

// ============================================================================ //
class FileDescriptor {
    pair<string, fsInode*> file;
    bool inUse;

    public:
    FileDescriptor(string FileName, fsInode* fsi) {
        file.first = FileName;
        file.second = fsi;
        inUse = true;

    }
    void setFileName(string s){
        this->file.first = s;
    }
    string getFileName() {
        return file.first;
    }
    fsInode* getInode() {
        return file.second;
    }
    bool isInUse() { 
        return (inUse); 
    }
    void setInUse(bool _inUse) {
        inUse = _inUse ;
    }
};
 
#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================ //
class fsDisk {
    FILE *sim_disk_fd;
 
    bool is_formated = false;

	// BitVector - "bit" (int) vector, indicate which block in the disk is free
	//              or not.  (i.e. if BitVector[0] == 1 , means that the 
	//             first block is occupied. 
    int BitVectorSize;
    int *BitVector;

    // Unix directories are lists of association structures, 
    // each of which contains one filename and one inode number.
    map<string, fsInode*> MainDir; 

    // OpenFileDescriptors --  when you open a file, 
	// the operating system creates an entry to represent that file
    // This entry number is the file descriptor. 
    vector< FileDescriptor > OpenFileDescriptors;

    int direct_enteris = 0;
    int block_size = 0;

    public:
    // ------------------------------------------------------------------------ //
    fsDisk() {
        sim_disk_fd = fopen( DISK_SIM_FILE , "r+" );
        assert(sim_disk_fd);
        for (int i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fwrite( "\0" ,  1 , 1, sim_disk_fd );
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
    }
    ~fsDisk(){

        free(this->BitVector);
        fclose(sim_disk_fd);

        for(int i = 0 ; i < this->OpenFileDescriptors.size() ; i++)
            if(this->OpenFileDescriptors[i].getFileName().compare("") != 0)
                delete this->OpenFileDescriptors[i].getInode();
    }


    // ------------------------------------------------------------------------ //
    /*This function prints all disk in 'list format', for each file descriptor - the function indicates if the file is open or close, and it's number (fd)*/
    void listAll() {
        int i = 0;    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it) {
            cout << "index: " << i << ": FileName: " << it->getFileName() <<  " , isInUse: " << it->isInUse() << endl; 
            i++;
        }
        char bufy;
        cout << "Disk content: '" ;
        for (i=0; i < DISK_SIZE ; i++) {
            int ret_val = fseek ( sim_disk_fd , i , SEEK_SET );
            ret_val = fread(  &bufy , 1 , 1, sim_disk_fd );
             cout << bufy;              
        }
        cout << "'" << endl;
    }
 
    // ------------------------------------------------------------------------ //
    /*This function get a block size and number of direct blocks, and configure the disk to work with this sizes*/
    void fsFormat(int blockSize =4, int direct_Enteris_ = 3) {
        
        /*Initilize the disk attributes*/
        this->block_size = blockSize;
        this->direct_enteris = direct_Enteris_;
        this->BitVectorSize = DISK_SIZE/this->block_size;

      
        this->BitVector = (int*)malloc(sizeof(int)*this->BitVectorSize);
        for(int i = 0 ; i < DISK_SIZE/this->block_size ; i++)
            this->BitVector[i] = 0;

        cout << "FORMAT DISK: number of block: " << DISK_SIZE/block_size << endl;

        this->is_formated = true;
    }

    // ------------------------------------------------------------------------ //
    /*This function create a new file on disk by getting from user the file name*/
    int CreateFile(string fileName) {
        if(!this->is_formated)
            return -1;
        /*An existing name on system*/
        for(int i = 0 ; i < this->OpenFileDescriptors.size(); i++)
        {
            if(fileName.compare(this->OpenFileDescriptors[i].getFileName()) == 0)
            {
                cout << fileName << " Is already exist\n" << endl;
                return -1;
            }
        } 
        fsInode* t = new fsInode(this->block_size,this->direct_enteris);
        FileDescriptor temp(fileName, t);
        this->MainDir.insert(pair<string,fsInode*>(fileName,t));
        this->OpenFileDescriptors.push_back(temp);
        int i = 0;
        /*Search the index of the new file descriptor*/    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it)
            i++;

        return (i - 1);
    }


    // ------------------------------------------------------------------------ //
    /*This function opens a close file*/
    int OpenFile(string fileName) {

        int i = 0;    
        for ( auto it = begin (OpenFileDescriptors); it != end (OpenFileDescriptors); ++it)
        {
            if(fileName.compare(this->OpenFileDescriptors[i].getFileName()) == 0)
            {
                /*If file desciptor at index i are close - open it, and return it's index*/
                if(!this->OpenFileDescriptors[i].isInUse())
                {
                    this->OpenFileDescriptors[i].setInUse(true);
                    return i;
                }
            }
            i++;        
        }
        return -1;
    }  

    // ------------------------------------------------------------------------ //
    /*This function close an open file, note that a new file is open*/
    string CloseFile(int fd) {

        /*Fd is not int he range*/
        if(fd > this->OpenFileDescriptors.size() || fd < 0)
          return "-1";
        
        if(!this->OpenFileDescriptors[fd].isInUse())
        {
            cout << "File " << fd << " is already closed" << endl;
            return "-1";
        }
        this->OpenFileDescriptors[fd].setInUse(false);
        return this->OpenFileDescriptors[fd].getFileName();
    }
    // ------------------------------------------------------------------------ //
    /*This function gets a fd number, and returns the context in this file, with 'len' chars*/
    int WriteToFile(int fd, char *buf, int len ) 
    {

        /**************************Illegal cases**************************/
        /*****************************************************************/
        if(!this->is_formated)
            return -1;
        /*Fd is not int he range*/
        if(fd > this->OpenFileDescriptors.size() || fd < 0)
            return -1;

        if(this->OpenFileDescriptors.empty())
            return -1;

        if(this->OpenFileDescriptors[fd].getInode() == NULL)
            return -1;

        if(!this->OpenFileDescriptors[fd].isInUse())
            return -1;

        if(len > ((this->direct_enteris + this->block_size)*block_size))
            return -1;
        
        /*****************************************************************/
        /*****************************************************************/

        //Full disk / not enough free place
        if((this->direct_enteris+block_size)*block_size - this->OpenFileDescriptors[fd].getInode()->getFileSize() - len < 0)
            return -1;

        for(int i = len ; i < DISK_SIZE - len ; i++)
            buf[i] = '\0';

        //Counts how many free block are exists
        int free_blocks = 0;                     
        for(int i = 0 ; i < this->BitVectorSize ; i++)
            if(this->BitVector[i] == 0)
                free_blocks++;

        int fileSize = this->OpenFileDescriptors[fd].getInode()->getFileSize();
        int i = 0;
        int temp = 0;

        //Case of this is the first time the file will be written
        if(this->OpenFileDescriptors[fd].getInode()->getBlockInUse() == 0)
        {
            this->OpenFileDescriptors[fd].getInode()->setFileSize(len); 
            if((len%block_size) != 0)
                this->OpenFileDescriptors[fd].getInode()->setBlockInUse((len/block_size) + 1);
            else
                this->OpenFileDescriptors[fd].getInode()->setBlockInUse(len/block_size);

            //Case of the text not required more then x direct blocks
            if((len <= this->direct_enteris*this->block_size) && (free_blocks >= this->direct_enteris))
            {
                //This while loop sets 1 in the relevant cells in bitVector.
                while(i < this->OpenFileDescriptors[fd].getInode()->getBlockInUse())
                {
                    temp = freeBlock(BitVector,this->BitVectorSize);
                    this->OpenFileDescriptors[fd].getInode()->setDirectBlocks(i,temp);
                    i++;
                }
                //This for loop store in the disk all data stroed in the buffer.
                for(int k = 0 ; k < this->OpenFileDescriptors[fd].getInode()->getBlockInUse(); k++)
                {   
                    fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(k)*block_size, SEEK_SET);
                    fwrite(buf, sizeof(char), block_size, sim_disk_fd);
                    buf += block_size;
                }
            }
            //Case of there are needs in allocation
            else if(len > this->direct_enteris*this->block_size && free_blocks >= this->direct_enteris)
            {
                //This while loop sets 1 in the relevant cells in bitVector and update the direct blocks array.
                while(i < this->OpenFileDescriptors[fd].getInode()->getBlockInUse())
                {
                    if(i == this->direct_enteris)
                        break;
                    temp = freeBlock(BitVector,this->BitVectorSize);
                    this->OpenFileDescriptors[fd].getInode()->setDirectBlocks(i,temp);
                    i++;
                }
                //Fills the direct blocks
                for(int k = 0 ; k < this->direct_enteris ; k++)
                {   
                    fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(k)*block_size, SEEK_SET);
                    fwrite(buf, sizeof(char), block_size, sim_disk_fd);
                    buf += block_size;
                }

                temp = freeBlock(BitVector,this->BitVectorSize);
                this->OpenFileDescriptors[fd].getInode()->setSingleInDirect(temp); //Now the singelInDirect integer stors the management block number

                //This part set the references to the data blocks, in the singel-indirect block. (till the next green line)
                char t;
                int inUse = this->OpenFileDescriptors[fd].getInode()->getBlockInUse();
                int offset = 0;                
                int index = 0;
                int required_indirect_blocks = 0; //Save how many append blocks are requireds 
                int size_to_write = 0;
                int internalBlock = 0;

                (strlen(buf)%block_size != 0) ? required_indirect_blocks = (strlen(buf)/block_size) + 1 : required_indirect_blocks = strlen(buf)/block_size;

                while(index < required_indirect_blocks)
                {
                    temp = freeBlock(BitVector,this->BitVectorSize);
                    t = '\0';
                    this->OpenFileDescriptors[fd].getInode()->setBlockInUse(inUse + 1);
                    decToBinary(temp,t);
                    fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size + index, SEEK_SET);
                    fwrite(&t, sizeof(char), 1, sim_disk_fd); 
                    index++;
                }              
                /*********************************************************************************/
                for (i=0; i < required_indirect_blocks ; i++) 
                {
                    if(strcmp(buf,"") == 0)
                        break;

                    (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);

                    if(size_to_write > block_size-offset)
                        size_to_write = block_size-offset;

                        fseek(sim_disk_fd , (this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size) + i , SEEK_SET );
                    fread(&t , sizeof(char), 1, sim_disk_fd);
                    internalBlock = (int)t;

                    fseek(sim_disk_fd, internalBlock*block_size, SEEK_SET);       
                    fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);
                    buf += size_to_write;                          
                }      
            }
        }
        //The case of it is not the first write to this file
        else if(this->OpenFileDescriptors[fd].getInode()->getBlockInUse() != 0)
        {
            this->OpenFileDescriptors[fd].getInode()->setFileSize(fileSize + len); 
            int free_direct_chars = this->direct_enteris*block_size - fileSize;
            if(free_direct_chars <= 0)
                free_direct_chars = 0;

            int offset = fileSize%block_size;
            int block_index = fileSize/block_size;
            int size_to_write = 0;

            int inUse = this->OpenFileDescriptors[fd].getInode()->getBlockInUse();        

            /*Case of buff can be stored on the remain direct blocks*/
            if(len <= free_direct_chars)
            {
                //fill the internal fregmentation
                if(offset != 0)
                {
                    (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);   

                    if(size_to_write > (block_size - offset))
                        size_to_write = block_size-offset;

                    fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(block_index)*block_size + offset, SEEK_SET);
                    fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);
                    buf += size_to_write;
                    block_index++;
                }
                /*Fill the remain direct blocks*/
                while(strcmp(buf,"") != 0)
                {
                    (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);
                    
                    temp = freeBlock(BitVector,BitVectorSize);
                    this->OpenFileDescriptors[fd].getInode()->setDirectBlocks(block_index,temp);
                    this->OpenFileDescriptors[fd].getInode()->setBlockInUse(inUse + 1);
                    fseek(sim_disk_fd,this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(block_index)*block_size, SEEK_SET);
                    fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);

                    inUse++;
                    block_index++;
                    buf += size_to_write;    
                }               
            }
            /*Case of there are needs of indirect allocation*/
            else if(len > free_direct_chars)
            {
                int internalBlock = 0;

                if(free_direct_chars != 0)
                {
                    if(offset != 0)
                    {
                        (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf); 

                        if(size_to_write > block_size-offset)
                            size_to_write = block_size-offset;
                                          
                        fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(block_index)*block_size + offset, SEEK_SET);
                        fwrite(buf, sizeof(char), block_size-size_to_write, sim_disk_fd);
                        buf += size_to_write;
                        block_index++;
                    }    
                    /*Fill the direct blocks if possible*/                
                    while(block_index < this->direct_enteris)
                    {
                        (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);  

                        temp = freeBlock(BitVector,BitVectorSize);
                        this->OpenFileDescriptors[fd].getInode()->setDirectBlocks(block_index,temp);
                        this->OpenFileDescriptors[fd].getInode()->setBlockInUse(inUse + 1);
                        fseek(sim_disk_fd,this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(block_index)*block_size, SEEK_SET);
                        fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);

                        inUse++;
                        block_index++;
                        buf += size_to_write;
                    }
                    temp = freeBlock(BitVector,this->BitVectorSize);
                    this->OpenFileDescriptors[fd].getInode()->setSingleInDirect(temp); //Now the singelInDirect integer stors the management block number

                    //This part set the references to the data blocks, in the singel-indirect block.
                    char t;
                    int index = 0;
                    int required_indirect_blocks = 0; //Save how many append blocks are requireds 

                    (strlen(buf)%block_size != 0) ? required_indirect_blocks = (strlen(buf)/block_size) + 1 : required_indirect_blocks = strlen(buf)/block_size;
        
                    while(index < required_indirect_blocks)
                    {   
                        temp = freeBlock(BitVector,this->BitVectorSize);
                        t = '\0';                        
                        decToBinary(temp,t);
                        fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size + index, SEEK_SET);
                        fwrite(&t, sizeof(char), 1, sim_disk_fd);                                   
                        index++;
                    }              
                    /*********************************************************************************/
        
                    i = 0;
                    for (i=0; i < required_indirect_blocks ; i++) 
                    {
                        fseek(sim_disk_fd , (this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size) + i , SEEK_SET );
                        fread(&t , sizeof(char), 1, sim_disk_fd);
                        internalBlock = (int)t;

                        fseek(sim_disk_fd, internalBlock*block_size, SEEK_SET); 
                        if(i != required_indirect_blocks - 1)      
                            fwrite(buf, sizeof(char), block_size, sim_disk_fd);
                        else 
                            fwrite(buf, sizeof(char), strnlen(buf,block_size), sim_disk_fd);
                        inUse++;
                        buf += block_size;                          
                    }    
                    this->OpenFileDescriptors[fd].getInode()->setBlockInUse(inUse);
                }
                /*There is no free space on direct blocks*/
                else if(free_direct_chars == 0)
                {  
                    char t;
                    int index = 0;
                    int required_indirect_blocks = 0; //Save how many append blocks are requireds 

                    //There are needs of indirect block allocation
                    if(this->OpenFileDescriptors[fd].getInode()->getSingleInDirect() == -1)
                    {    
                        temp = freeBlock(BitVector,this->BitVectorSize);
                        this->OpenFileDescriptors[fd].getInode()->setSingleInDirect(temp); //Now the singelInDirect integer stors the management block number                        
                        //This part set the references to the data blocks, in the singel-indirect block.

                        (strlen(buf)%block_size != 0) ? required_indirect_blocks = (strlen(buf)/block_size) + 1 : required_indirect_blocks = strlen(buf)/block_size;

                        while(index < required_indirect_blocks)
                        {   
                            temp = freeBlock(BitVector,this->BitVectorSize);
                            this->OpenFileDescriptors[fd].getInode()->setBlockInUse(inUse + 1);
                            t = '\0';                        
                            decToBinary(temp,t);
                            fseek(sim_disk_fd, this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size + index, SEEK_SET);
                            fwrite(&t, sizeof(char), 1, sim_disk_fd);                                   
                            index++;
                        }
                        for (i=0; i < required_indirect_blocks ; i++) 
                        {
                            if(strcmp(buf, "") == 0)
                                break;
                            (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);

                            if(size_to_write > block_size-offset)
                                size_to_write = block_size-offset;

                            fseek(sim_disk_fd , (this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size) + i , SEEK_SET );
                            fread(&t , sizeof(char), 1, sim_disk_fd);
                            internalBlock = (int)t;

                            fseek(sim_disk_fd, internalBlock*block_size, SEEK_SET);       
                            fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);
                            buf += size_to_write;                          
                        }                     
                        /*********************************************************************************/
                    }
                    //Allocation allready done on last time this file written on
                    else if(this->OpenFileDescriptors[fd].getInode()->getSingleInDirect() != -1)
                    {
                        int offset_in_indirectblock = (fileSize - this->direct_enteris*block_size)/block_size;

                        //(offset == 0) ? offset_in_indirectblock = (fileSize - this->direct_enteris*block_size)/block_size : offset_in_indirectblock = ((fileSize - this->direct_enteris*block_size)/block_size) + 1;

                        //Fill in the internal fregmentation
                        if(offset != 0)
                        {
                            fseek(sim_disk_fd , (this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size) + offset_in_indirectblock , SEEK_SET );
                            fread(&t , sizeof(char), 1, sim_disk_fd);
                            internalBlock = (int)t;

                            (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);

                            if(size_to_write > block_size-offset)
                                size_to_write = block_size-offset;

                            fseek(sim_disk_fd, internalBlock*block_size + offset, SEEK_SET);
                            fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);
                            buf += size_to_write;   
                            offset_in_indirectblock++; 
                        }
                        //Fill the indirect blocks
                        for (i=offset_in_indirectblock ; i < block_size ; i++) 
                        {
                            if(strcmp(buf,"") == 0)
                                break;
                            (strlen(buf) > block_size) ? size_to_write = block_size : size_to_write = strlen(buf);

                            if(size_to_write > block_size-offset)
                                size_to_write = block_size-offset;
    
                            /*This section reads a block number and set on it "size_to_write" of data*/
                            temp = freeBlock(BitVector,this->BitVectorSize);
                            this->OpenFileDescriptors[fd].getInode()->setBlockInUse(inUse + 1);
                            t = '\0';                        
                            decToBinary(temp,t);                 
                            fseek(sim_disk_fd , (this->OpenFileDescriptors[fd].getInode()->getSingleInDirect()*block_size) + i, SEEK_SET );
                            fwrite(&t, sizeof(char), 1, sim_disk_fd);      
                            internalBlock = (int)t;

                            fseek(sim_disk_fd, internalBlock*block_size, SEEK_SET);       
                            fwrite(buf, sizeof(char), size_to_write, sim_disk_fd);
                            buf += size_to_write;                          
                        }          
                    }  
                }
            }
        }
    }
    // ------------------------------------------------------------------------ //
    /*This function delete an exist file*/
    int DelFile( string FileName ) {
        if(!this->is_formated)
            return -1;

        int j = 0;
        int bit_to_zero = 0;
        char t;
        int internal_block_index = 0;
        for(int i = 0 ; i < this->OpenFileDescriptors.size(); i++)
        {
            if(FileName.compare(this->OpenFileDescriptors[i].getFileName()) == 0)
            {
                /*Set zero at all allocate direct blocks*/
                for(j = 0 ; j < direct_enteris ; j++)
                {
                    if(this->OpenFileDescriptors[i].getInode()->getDirectBlocks(j) != -1)
                    {
                        bit_to_zero = this->OpenFileDescriptors[i].getInode()->getDirectBlocks(j);
                        BitVector[bit_to_zero] = 0;
                        this->OpenFileDescriptors[i].getInode()->setDirectBlocks(j,-1);
                    }
                }
                /*Set zero at all indirect blocks, if alloced*/
                if(this->OpenFileDescriptors[i].getInode()->getSingleInDirect() != -1)
                {
                    j = 0;
                    while(j < block_size)
                    {
                        fseek(sim_disk_fd , (this->OpenFileDescriptors[i].getInode()->getSingleInDirect()*block_size) + j , SEEK_SET ); 
                        fread(&t , sizeof(char), 1, sim_disk_fd); 
                        internal_block_index = (int)t;
                        if(internal_block_index > 0)
                            BitVector[internal_block_index] = 0;
                        t = '\0';
                        j++;
                    }                      
                }
                this->OpenFileDescriptors[i].setFileName("");
                this->OpenFileDescriptors[i].setInUse(false);
                delete this->OpenFileDescriptors[i].getInode();
                this->MainDir.erase(FileName);
                return i;
            }
        } 
        return -1;
    }
    // ------------------------------------------------------------------------ //
    /*This function get a fd number, and returns the content from this file stored in the disk. the data will write to the buffer*/
    int ReadFromFile(int fd, char *buf, int len ) { 

        for(int i = 0 ; i < DISK_SIZE ; i++)
            buf[i] = '\0';        

        if(!this->is_formated)
            return -1;

        if(fd > this->OpenFileDescriptors.size())
            return -1;

        if(!this->OpenFileDescriptors[fd].isInUse())
            return -1;

        int counter = 0;
        int char_counter = len;
        int index = 0;
        int file_size = this->OpenFileDescriptors[fd].getInode()->getFileSize();
        int blk_index = 0;

        int management_block = this->OpenFileDescriptors[fd].getInode()->getSingleInDirect();
        int internal_block_index = 0;
        int i = 0;
        int j = 0;
        char t, bufy;
        /*This for loop read the whole text*/
        for (index=0; index < file_size ; index++) 
        {
            /*Direct blocks*/
            if(index < block_size*this->direct_enteris)
            {
                //All required chars has copied
                if(char_counter == 0) 
                    break;

                if(index != 0 && index%block_size == 0)
                    blk_index++;    
            
                fseek ( sim_disk_fd , this->OpenFileDescriptors[fd].getInode()->getDirectBlocks(blk_index)*block_size + (index%block_size) , SEEK_SET );
                fread( &bufy , 1 , 1, sim_disk_fd );
                strncat(buf,&bufy,1);
                counter++;
                char_counter--;     
            }
            /*Indirect blocks*/
            else
            {
                while(i < block_size)
                {
                    t = '\0';
                    fseek(sim_disk_fd , management_block*block_size + i, SEEK_SET );
                    fread(&t , sizeof(char), 1, sim_disk_fd);
                    internal_block_index = (int)t;

                    j = 0;
                    while(j < block_size)
                    {  
                        if(char_counter == 0 || counter == file_size) 
                            return 0;
                        fseek ( sim_disk_fd , internal_block_index*block_size + j , SEEK_SET );
                        fread( &bufy , 1 , 1, sim_disk_fd );                        
                        strncat(buf,&bufy,1);
                        char_counter--;
                        counter++;
                        j++;
                    }
                    i++;
                }
            }
        }
    }
};
    
int main() {
    int blockSize = 0; 
	int direct_entries = 0;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read; 
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while(1) {
        cin >> cmd_;
        switch (cmd_)
        {
            case 0:   // exit
				delete fs;
				exit(0);
                break;

            case 1:  // list-file
                fs->listAll(); 
                break;
          
            case 2:    // format
                cin >> blockSize;
				cin >> direct_entries;
                fs->fsFormat(blockSize, direct_entries);
                break;
          
            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            
            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
             
            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd); 
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
           
            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile( _fd , str_to_write , strlen(str_to_write) );
                break;
          
            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read ;
                fs->ReadFromFile( _fd , str_to_read , size_to_read );
                cout << "ReadFromFile: " << str_to_read << endl;
                break;
           
            case 8:   // delete file 
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

} 
