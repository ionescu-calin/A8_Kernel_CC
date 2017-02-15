#ifndef __FILE_H
#define __FILE_H

#include <stddef.h>
#include <stdint.h>

//info block is used to tell if a block is occupied or not
//4096 blocks are used to index the entire 2^16 no of blocks
//metadata occupies half the space
//files take up the rest 1/2 of the space
#define info_block_meta 		0
#define info_block_file 		2048
#define info_block_end  		4096
#define info_metadata_start     (info_block_file)
#define metadata_start 			4096
#define files_start_address     (info_block_file*16)
#define error_place     		65535
#define file_parts     			7
#define file_name_size			8
#define max_files_dir			  20

//typedef enum {FILE, DIR} filetype_t;

//This represents the file information (meta data)
typedef struct file_t  {
	char name[file_name_size];
	uint16_t size;
	uint16_t dist_address;      //pointer to the block that points to the file contents
	uint16_t permissions;				//RW/Read/Write
	uint16_t type;					  	//File or Dir
}file_t;								      //exactly 16B so it can fit into a block


typedef struct file_distributor  {
	uint16_t file_part[file_parts];				//pointer to the contents of a file
	uint16_t next_dist;							//pointer to the next file distributor if all the file_part[] is filled
}file_distributor;								//exactly 16B so it can fit into a block


//finds a free block startding from the start_block address and returns its address
uint16_t find_empty_block(uint16_t start_block);

//stores the metadata of a file to the disk
int store_metadata(file_t file_metadata, uint16_t address);

//stores the file distributor of a file to the disk
void store_file_distributor(file_distributor dist, uint16_t address);

//creates new meta data for a file (basicaly creates the file/dir) and returns its address
uint16_t create_metadata(char* name, uint16_t type, uint16_t perm);

//creates file distributor and returns its address
uint16_t create_file_distributor();

//returns the metadata contained by the block at an address
file_t read_file_meta(uint16_t address);

//returns the distributor contained by the block at an address
file_distributor read_file_distributor(uint16_t address);

//searches for the metadata of a file/dir and returns its address or error_place if not found
uint16_t find_meta(char* name, uint16_t type);

//reads the contents of a file
void read_file_content (uint8_t* x);

//creates a new file part that is linked to the indirect linker (distributor) and returns its address
uint16_t create_file_part();

//writes x to a file at a certain file pointer,  given the file's meta_address
//returns -1 for fail or the new fp if it succeeds
int write_to_file(uint16_t meta_address, uint16_t fp, char* x, uint16_t n);

//deletes file/dir and everything they contain by removing the links
//between all the files that it contains
//it does not remove the link from the parent of the directory, this must
//be handled separately (use delete_from_dir which calls this function)
void delete_file(uint16_t address);

//reads information from the file given its address and file pointer
//information is returned in x
//the function returns the new file pointer of the file
int read_from_file(uint16_t meta_address, uint16_t fp, uint16_t n, char* x);

//Looks through the entire meta data to see if root exists
//Creates root if it doesn't, else it returns its address
//Takes a long time if root does not exist
uint16_t create_root();

//creates a file given a directory addres
uint16_t create_file_dir_addr(uint16_t dir_addr, char* name, uint16_t type, uint16_t perm);

//finds all files within a folder and writes them to STDOUT
void find_all_files_by_addr(uint16_t file_address);

//returns the address of a file if it is found in the file system
//else it returns error_place
uint16_t find_file(char* path, uint16_t type);

//deletes a file given its addres and its parrent addres by removing the link
//from the partent to it
void delete_from_dir(uint16_t fdr_addr, uint16_t dell_addr);


#endif
