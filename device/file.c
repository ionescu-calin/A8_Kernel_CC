#include "file.h"
#include "disk.h"
#include <string.h>

uint16_t root_addr = 4096;
void write_no2(uint32_t n) {
     uint32_t x = n;if(n == 0)  PL011_putc( UART0, ('0') );
     for(uint32_t i = 1;i<=1;i++) {
          n = x;
          while(n!=0) {
            PL011_putc( UART0, ('0'+n%10) );
            n = n/10;
          }
      PL011_putc( UART0, ('\n') );
      }
}

void write_str2(char* x) {
  while(*x!='\0')
    PL011_putc( UART0, *x++ );
}

uint16_t find_empty_block(uint16_t start_block) {
	//so it starts from the current block as it is inc at the beginning of the while

	uint16_t block = start_block - 1;
	uint8_t content[16];
	uint16_t content_index = 0;
	uint16_t found = 0;
	while(!found) {
		block++;
    content_index = 0;
		disk_rd(block, content, 16);
		while(content_index < 16 && !found) {
			if(content[content_index] == 0)
				found = 1;
			else content_index++;
		}
	}

	if(!found || block == -1)
		return error_place;
	else {
		if(start_block>=info_block_file && start_block < info_block_end)
			return (block*16+content_index);
		else {
			uint16_t address = info_block_end + block*16+content_index;
			return address;
		}
	}
}

//makes an update in the info space. Type 0 == delete, type 1 == create
void update_info_space(uint16_t address, uint16_t type) {
	uint8_t x[16];
  if(address < files_start_address)
	 address = address - info_block_end;
	disk_rd(address/16, x, 16);
	x[address%16] = type;
  disk_wr(address/16, x, 16);
}

int store_metadata(file_t file_metadata, uint16_t address) {
  	uint8_t* x = (uint8_t*) &file_metadata;
	disk_wr(address, x, 16);
}

void store_file_distributor(file_distributor dist, uint16_t address) {
  	uint8_t* x = (uint8_t*) &dist;
	disk_wr(address, x, 16);
}

uint16_t create_metadata(char* name, uint16_t type, uint16_t perm) {
	file_t metadata;
	//metadata.name = name;
    strcpy(metadata.name, name);
	metadata.type = type;
	metadata.dist_address = error_place;
    metadata.size = 0;
    metadata.permissions = perm; 
	uint16_t address = find_empty_block(info_block_meta);
	if(address == error_place || address > files_start_address)
		return error_place;
	else {
		store_metadata(metadata, address);
    update_info_space(address, 1);
		return address;
	}
}

//Creates file_distributor, saves it to the disk and returns it's block address
uint16_t create_file_distributor() {
	file_distributor dist;
	for(int i = 0; i<file_parts; i++)
		dist.file_part[i] = error_place;
	uint16_t address = find_empty_block(info_block_file);
  dist.next_dist = error_place;
	store_file_distributor(dist, address);
	update_info_space(address, 1);
	return address;
}

//Finds an empty space for a new file content part and returns its address
//+ marks the block as used
uint16_t create_file_part() {
	uint16_t location = find_empty_block(info_block_file);
	update_info_space(location, 1);
	return location;
}

file_t read_file_meta(uint16_t address) {
	uint8_t x[16];
	file_t meta;
	disk_rd(address, x, 16);
	meta = *((file_t*)x);
	return meta;
}

file_distributor read_file_distributor(uint16_t address) {
	uint8_t x[16];
	file_distributor file;
	disk_rd(address, x, 16);
	file = *((file_distributor*)x);
	return file;
}

//Returns the meta data which contains a pointer to the file/dir contents
uint16_t find_meta(char* name, uint16_t type) {
	for(uint16_t i = metadata_start; i<files_start_address; i++) {
		file_t meta = read_file_meta(i);
		if(strcmp(meta.name, name) == 0 && meta.type == type) {
			return i;
		}
	}
	return error_place;
}

// return -1 if it fails (no space to allocate a required block)
//els           e the new fp is returned (also represents no of bytes written)
// implemented so -1 will be returned if fp is larger than the file size
int write_to_file(uint16_t meta_address, uint16_t fp, char* x, uint16_t n) {
  file_t meta;
  meta = read_file_meta(meta_address);

  if(meta.type == 1)
    return -1;

	if(meta.dist_address == error_place) {
		meta.dist_address = create_file_distributor();
		store_metadata(meta, meta_address);
	}

	uint16_t dist_address = meta.dist_address;
    uint16_t fpp = fp; // file pointer part

	if(fp>meta.size)
		return -1;
	else {
  	  file_distributor dist = read_file_distributor(dist_address);
      fpp /=16;
      //Move to the correct distributor and content according to the fp
  		while(fpp>=file_parts) {
        if(dist.next_dist == error_place) {
          dist.next_dist  = create_file_distributor();
          if(dist.next_dist == error_place)
            return -1;
          store_file_distributor(dist, dist_address);
          dist_address = dist.next_dist;
          dist =  read_file_distributor(dist_address);
        }
        else {
          dist_address  = dist.next_dist;
          dist = read_file_distributor(dist.next_dist);
        }

  			fpp-=file_parts;
  		}
  	  uint16_t part_index = (fp/16)%7; //which of the part_number (7) it should write to
      uint16_t byte_index = fp%16;     //from where to start replacing within a block

      //Start writing to the file in the right file part
      while(n!=0) {
        if(part_index >= file_parts) {
           part_index = 0;
           if(dist.next_dist == error_place) {
             dist.next_dist  = create_file_distributor();
             if(dist.next_dist == error_place)
               return -1;
             store_file_distributor(dist, dist_address);
             dist_address = dist.next_dist;
             dist =  read_file_distributor(dist_address);
           }
           else {
             dist_address  = dist.next_dist;
             dist = read_file_distributor(dist.next_dist);
           }
        }
		if(dist.file_part[part_index] == error_place) {
			dist.file_part[part_index] = create_file_part();
      		if(dist.file_part[part_index] == error_place) //No more blocks
        		return -1;
			store_file_distributor(dist, dist_address);
		}

        if(n + byte_index > 16) {
        	if(byte_index!=0) {
        	  uint8_t content[16];
        	  disk_rd(dist.file_part[part_index], content, 16);
        	  for(int i = byte_index; i < 16; i++) {
        	  	content[i] = x[i - byte_index];
        	  }
        	  disk_wr(dist.file_part[part_index], content, 16);
            x+=(16-byte_index);
            n-=(16-byte_index);
            fp+=(16-byte_index);
            byte_index = 0;
        	}
        	else{
          	disk_wr(dist.file_part[part_index], x, n);
            x+=16;
            fp+=16;
            n-=16;
      	  }
        }
        else {
        	if(byte_index!=0) {
        	  uint8_t content[16];
        	  disk_rd(dist.file_part[part_index], content, 16);
        	  for(int i = byte_index; i <byte_index + n; i++) {
        	  	content[i] = x[i - byte_index];
        	  }
        	  disk_wr(dist.file_part[part_index], content, 16);
            byte_index = 0;
        	}
        	else{
          	disk_wr(dist.file_part[part_index], x, n);
      	    }
            x+=n;
            fp+=n;
            n-=n;
        }
        part_index++;
      }
      if(meta.size < fp) {
      	meta.size = fp;
      	store_metadata(meta, meta_address);
      }
  		return (fp);
	}
}

//returns -1 if fp is trying to access something which is not part of the file
//else the new fp is returned
int read_from_file(uint16_t meta_address, uint16_t fp, uint16_t n, char* x) {
  file_t meta;
  meta = read_file_meta(meta_address);
  if(meta.size < n)
    n = meta.size;
  //if(fp > meta.size)
    //return -1;

  file_distributor dist = read_file_distributor(meta.dist_address);

  uint16_t fpp = fp/16;
  while(fpp >= file_parts) {
    dist = read_file_distributor(dist.next_dist);
    fpp-=file_parts;
  }

  uint16_t fp_loc = (fp/16)%7;    //at which part
  uint8_t content[17];
  uint16_t byte_index = fp%16;
  while(n != 0) {
    if(fp_loc >= file_parts) {
      fp_loc = 0;
      dist = read_file_distributor(dist.next_dist);
    }
    if( n + byte_index > 16 ) {
      disk_rd(dist.file_part[fp_loc], content, 16);
      content[16] = '\0';
      strcpy(x, content + byte_index);
      n-= 16-byte_index;
      x+= 16-byte_index;
      fp+=16-byte_index;
      byte_index = 0;
      fp_loc++;
    }
    else {
      disk_rd(dist.file_part[fp_loc], content, 16);
      content[n + byte_index] = '\0';
      strcpy(x, content + byte_index);
      x+= n;
      fp+=n;
      n-= n;
    }
  }
  return fp;
}

void delete_file(uint16_t address) {
  //uint16_t address = find_meta(name, 0);
  file_t file = read_file_meta(address);
  update_info_space(address, 0);

  address = file.dist_address;
  while(address != error_place) {
    file_distributor dist = read_file_distributor(file.dist_address);
    update_info_space(address, 0);
    address = dist.next_dist;
    for(int i = 0; i< file_parts; i++) {
      if(dist.file_part[i] != error_place)
        update_info_space(dist.file_part[i], 0);
      }
  }
}

uint16_t create_root() {
  uint16_t address = error_place; //= find_meta("root", 1);
  if(address == error_place)
    address = create_metadata("root", 1, 0);
  return address;
}

//create file given directory address and returns its address
uint16_t create_file_dir_addr(uint16_t dir_addr, char* name, uint16_t type, uint16_t perm) {
  file_t file = read_file_meta(dir_addr);
  uint16_t dist_addr     = file.dist_address, dist_index;
  uint16_t dist_addr_cpy = dist_addr;
  file_distributor dist  = read_file_distributor(file.dist_address);
  //If there is no distributor
  if(dist_addr == error_place) {
    dist_addr = create_file_distributor();
    file.dist_address = dist_addr;
    store_metadata (file, dir_addr);
    dist = read_file_distributor(dist_addr);
  }
  //If distributor already exists, look for an empty space to add file
  while(dist_addr != error_place) {
    dist_addr_cpy = dist_addr;
    dist_index = 0;
    while(dist_index < file_parts) {
      if(dist.file_part[dist_index] == error_place) {
        dist.file_part[dist_index] = create_metadata(name, type, perm);
        store_file_distributor(dist, dist_addr);
        return dist.file_part[dist_index];
      }
      dist_index ++;
    }
    dist_addr = dist.next_dist;
    dist = read_file_distributor(dist_addr);
  }
  dist_addr = dist_addr_cpy;
  dist = read_file_distributor(dist_addr);
  //If a new distributor has to be created
  dist.next_dist = create_file_distributor();
  store_file_distributor(dist, dist_addr);
  dist_addr = dist.next_dist;
  dist = read_file_distributor(dist_addr);
  dist.file_part[0] = create_metadata(name, type, perm);
  store_file_distributor(dist, dist_addr);
  return dist.file_part[0];
}


//returns all the file (names) that are contained by a directory given its address
void find_all_files_by_addr(uint16_t file_address) {
  char names[100][100];
  file_t file            = read_file_meta(file_address);
  file_t file2;
  uint16_t dist_addr     = file.dist_address;
  file_distributor dist  = read_file_distributor(file.dist_address);
  //char names[max_files_dir][file_name_size];
  uint16_t names_index = 0;
  while(dist_addr != error_place) {
    for(int i = 0; i < file_parts; i++)
      if(dist.file_part[i] != error_place){
          file2 = read_file_meta(dist.file_part[i]);
          strcpy(names[names_index], file2.name);
          write_str2(file2.name);
          write_str2("  ");
          names_index++;
      }
    dist_addr = dist.next_dist;
    dist  = read_file_distributor(dist.next_dist);
  }
}

uint16_t find_file(char* path, uint16_t type) {
  char  *path_tokens     = strtok(path,"/");
  uint16_t meta_addr     = root_addr; //or file_addr
  file_t file            = read_file_meta(meta_addr);
  uint16_t dist_addr     = file.dist_address;
  file_distributor dist  = read_file_distributor(file.dist_address);
  uint16_t found, dist_index;

  if(!path_tokens)
    return root_addr;

  while(path_tokens != NULL) {
    found = 0;
    while(dist_addr != error_place && !found) {
      dist_index = 0;
      while((dist_index < file_parts) && !found) {
        if(dist.file_part[dist_index] != error_place) {
          meta_addr = dist.file_part[dist_index];
          file = read_file_meta(dist.file_part[dist_index]);
          if(strcmp(file.name,path_tokens) == 0)
            found = 1;
        }
        dist_index++;
      }
      if(!found) {
        dist_addr = dist.next_dist;
        dist =  read_file_distributor(dist_addr);
      }
    }
    if(!found){
      return error_place;
    }
    else {
      if(file.type == 1) {
        dist_addr = file.dist_address;
        dist =  read_file_distributor(dist_addr);
      }
    }
    path_tokens = strtok(NULL, "/");
  }

  if(path_tokens != NULL)
    return error_place;
  else return meta_addr;
  return error_place;
}

void delete_from_dir(uint16_t fdr_addr, uint16_t dell_addr) {
  file_t file = read_file_meta(fdr_addr);
  uint16_t dist_addr = file.dist_address;
  file_distributor dist = read_file_distributor(dist_addr);
  int found = 0, i;
  while(!found) {
    i = 0;
    while(dist.file_part[i] != dell_addr && i < file_parts)
      i++;
    if(dist.file_part[i] == dell_addr && i < file_parts){
      found = 1;
      dist.file_part[i] = error_place;
      store_file_distributor(dist, dist_addr);
    }
    else {
      dist_addr = dist.next_dist;
      dist = read_file_distributor(dist_addr);
    }
  }
}
/*
create dir();
when you try to create a file, you have to specify a folder or root will be selected
the created metadata is linked to a file
*/
