//-----------------------------------------------------------------------------
// F34x_MSD_File_System.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// File contains the basic functions for file system commands.
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000035
// Target:         C8051F34x
// Tool chain:     Keil
// Command Line:   See Readme.txt
// Project Name:   F34x_USB_MSD
//
// Release 1.1
//    -All changes by PKC
//    -09 JUN 2006
//    -No changes; incremented revision number to match project revision
//
// Release 1.0
//    -Initial Release
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "F34x_MSD_Definitions.h"
#include "F34x_MSD_File_System.h"
#include "F34x_MSD_Sect_Serv.h"
#include "F34x_MSD_Util.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

extern bootrecord_small xdata MBR;
void Print_File(FILE *file);

//-----------------------------------------------------------------------------
// file_name_match
//-----------------------------------------------------------------------------
//
// Return Value : 1 if they match
// Parameters   : filename - pointer to file name
//     			  direntryname - pointer to enterd directory name
//
// Compares the file name and directory name
//-----------------------------------------------------------------------------

static BYTE file_name_match(char* filename,char* direntryname)
{
  xdata BYTE i,j = filename[0];
  for(i=0;i<8;i++) {
    if(direntryname[i] == ' ' && (filename[i] == '\0' || filename[i] == '.')) 
	{
	  if(!(j == '.' && filename[i] == '.')) 
	      break;
	}
    if(tolower(direntryname[i])!=filename[i]) 
      return 0;
  }
  j = i+1;
  for(i = 8; i < 11; i++) {
    if( filename[j] == '\0' && direntryname[i] != ' ') 
      return 0;
    if( direntryname[i] == ' ' && (filename[j] == '\0' || filename[j] == '.' 
        || filename[j-1] == '\0'))
      break;
    if(tolower(direntryname[i]) != filename[j]) 
      return 0;
    j++;
  }
  return 1;
}


//-------------------------------------------------------------------------------
// Functions only for F340 device
//-------------------------------------------------------------------------------


#ifdef __F340_VER__

xdata unsigned char Path_Name[200];
xdata unsigned long Current_Dir_Block;
//xdata char current_dir_name[40];
static find_info xdata findinfo; // Shared find_info for fopen() and fdelete() 
static unsigned fat_chain_alloc(unsigned from,unsigned nr) ;
static unsigned long fat_chain(unsigned long from,unsigned nr) ;
static void fat_chain_free(unsigned from);

//-----------------------------------------------------------------------------
// write_current_dir
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function printout current directory name
//-----------------------------------------------------------------------------
void write_current_dir()
{
	printf("%s",Path_Name);
}


//-----------------------------------------------------------------------------
// GetClusterOfParentDirectory
//-----------------------------------------------------------------------------
//
// Return Value : cluster number
// Parameters   : None
//
// Function returns cluster number which begins current directory
//-----------------------------------------------------------------------------
static unsigned GetClusterOfParentDirectory()
{
	if(Current_Dir_Block == Sect_Root_Dir())  return 0;
	return (Current_Dir_Block - Sect_File_Data()) / MBR.sectors_per_cluster;
}

//-----------------------------------------------------------------------------
// Get_Cluster_From_Sector
//-----------------------------------------------------------------------------
//
// Return Value : cluster number
// Parameters   : sector - sector which belongs to returned cluster
//
// Function returns cluster number which contains sector
//-----------------------------------------------------------------------------
static unsigned Get_Cluster_From_Sector(unsigned long sector)
{
	if(sector < (Sect_File_Data() + 2*MBR.sectors_per_cluster)) return 0;
	return ((sector - Sect_File_Data()) / MBR.sectors_per_cluster);
}

//-----------------------------------------------------------------------------
// Get_First_Sector
//-----------------------------------------------------------------------------
//
// Return Value : sector address
// Parameters   : cluster - cluster number
//
// Function returns first sector which belongs to the cluster
//-----------------------------------------------------------------------------
static unsigned long Get_First_Sector(unsigned cluster)
{
	if(cluster >= 2) return Sect_File_Data() + cluster*MBR.sectors_per_cluster;
	else return Sect_Root_Dir();
}

//-----------------------------------------------------------------------------
// Get_First_Block_Of_Next_Cluster
//-----------------------------------------------------------------------------
//
// Return Value : first block of next cluster in chain or 0xFFFFFFFF if cluster 
//                is last in chain
//
// Parameters   : cluster - searching cluster 
//
// Function returns number of first sector in next cluster in cluster chain
//-----------------------------------------------------------------------------
static unsigned long Get_First_Block_Of_Next_Cluster(unsigned cluster)
{
	xdata unsigned long ret = fat_chain(cluster,MBR.sectors_per_cluster);
	if(ret != 0xFFFFFFFF)
		return ret + Sect_File_Data();
	return ret;
}

//-----------------------------------------------------------------------------
// Get_Next_Cluster
//-----------------------------------------------------------------------------
//
// Return Value : cluster number
// Parameters   : next cluster in chain
//
// Function returns number of next cluster in chain
//-----------------------------------------------------------------------------
static unsigned Get_Next_Cluster(unsigned cluster)
{
	unsigned* xdata fat_table=Scratch;

    Sect_Read(Sect_Fat1() + cluster/(Sect_Block_Size()/2));

    return ntohs(fat_table[cluster%(Sect_Block_Size()/2)]);
}

//-----------------------------------------------------------------------------
// Get_File_Name
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : file_name - name of file [out] (must be allocated outside
//                            this function)
//				      direntry_name - name used in direntry [in]
//
// Function gets file name from direntry
//-----------------------------------------------------------------------------
static void Get_File_Name(char* direntry_name,char* file_name)
{
	unsigned i,j = 0,k = 0;
	for(i=0;i<11;i++)
	{
		if(direntry_name[i] == ' ')
		{
			j = 1;
			continue;
		}
		if(j)
		{
			file_name[k++] = '.';
			j = 0;
		}
		file_name[k++] = tolower(direntry_name[i]);
	}
	file_name[k] = 0;
}

//-----------------------------------------------------------------------------
// Get_First_Block_Directory_Cluster
//-----------------------------------------------------------------------------
//
// Return Value : first sector of cluster which belongs to current directory 
//                fat chain
// Parameters   : sector - sector to check				
//
// Function returns first sector of cluster which contains sector if this
// cluster belongs to FAT chain of current directory
//-----------------------------------------------------------------------------
static unsigned long Get_First_Block_Directory_Cluster(unsigned long sector)
{
	xdata unsigned cluster = Get_Cluster_From_Sector(sector);
	xdata unsigned next_dir_cluster = GetClusterOfParentDirectory();
	while(next_dir_cluster != cluster)
	{
		next_dir_cluster = Get_Next_Cluster(next_dir_cluster);
		if(next_dir_cluster >= 0xfff8) return next_dir_cluster;
	}
	return Get_First_Sector(cluster);
}


//-----------------------------------------------------------------------------
// Clear_Cluster
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : cluster - cluster number				
//
// Function is used to clear all sectors of cluster
//-----------------------------------------------------------------------------
static void Clear_Cluster(unsigned cluster)
{
	xdata unsigned long sector = Get_First_Sector(cluster);
	xdata unsigned i;
	memset(Scratch,0,512);
	for(i=0;i<MBR.sectors_per_cluster;i++)
	{
		Sect_Write(sector+i);
	}

}

//-----------------------------------------------------------------------------
// FillDirEntry
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : direntry - dir entry
//				  dir_name - file/directory name							
//
// Function fills dir entry with file name
//-----------------------------------------------------------------------------
static void FillDirEntry(dir_entry* direntry,char* dir_name)
{
  xdata BYTE i;
  for( i = 0; i < 10; i++) 
    direntry->sfn.reserved[i] = 0;
  
  direntry->sfn.time.i = findinfo.direntry->sfn.date.i = 0;
  direntry->sfn.filesize = 0;

  // Fill in the filename
  for( i = 0; i < 11; i++ ) 
    direntry->sfn.name[i] = ' ';
 
  for( i = 0; i < 11; i++ ) {
    if(!dir_name[i])
	     break;
    direntry->sfn.name[i] = toupper(dir_name[i]);
  }
}

//-----------------------------------------------------------------------------
// FileSys_Init
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None							
//
// Function initializes some data used to navigate over directories
//-----------------------------------------------------------------------------
void FileSys_Init()
{
	Current_Dir_Block = Sect_Root_Dir();
	strcpy(Path_Name,"\\");
}

//-----------------------------------------------------------------------------
// chngdir
//-----------------------------------------------------------------------------
//
// Return Value : 0 - if such directory not exists in current directory 1 
//                    otherwise
// Parameters   : dirname - directory name to create							
//
// Function tries to find and opens directory in current directory
//-----------------------------------------------------------------------------
BYTE chngdir(char* dirname)
{
  findfirst(&findinfo, 0);
  while(!file_name_match(dirname,findinfo.direntry->sfn.name)) {
    if(!findnext(&findinfo)) {
      return 0;
    }
  }
  if(findinfo.direntry->sfn.attrib & ATTRIB_SUBDIR) 
  {
  	if(findinfo.direntry->sfn.starting_cluster == 0x00)
		Current_Dir_Block = Sect_Root_Dir();
	else
	  	Current_Dir_Block = Sect_File_Data() + 
			(htons(findinfo.direntry->sfn.starting_cluster) * MBR.sectors_per_cluster);
	if(!strcmp(dirname,"."))	return 1;
	if(!strcmp(dirname,".."))
	{
		xdata unsigned char* next,*pos = strstr(Path_Name,"\\");

		while((next = strstr(pos,"\\")) != (NULL))
			pos = next+1;
		
		if(pos!=(Path_Name+1))
		pos--;
		
			*pos = '\0';
	}
	else
	{
		xdata unsigned len = strlen(Path_Name);
		if(Path_Name[len-1] != '\\')
		{	
			strcpy(&Path_Name[len],"\\");
			len++;
		}
		strcpy(&Path_Name[len],dirname);
	}
	return 1;	
  }
  return 0;
}

//-----------------------------------------------------------------------------
// mkdir
//-----------------------------------------------------------------------------
//
// Return Value : 0 if function succeeds other value if error occurs
// Parameters   : dir_name - directory name							
//
// Function creates directory 
//-----------------------------------------------------------------------------
BYTE mkdir(char* dir_name)
{
   
   xdata unsigned long dir_sectors;
   xdata dir_entry* entry;
   xdata unsigned start_cluster;
   unsigned max_len = strlen(dir_name);

   
  if((dir_name == NULL) || (max_len == 0) || (max_len > 8))
  {
	return DIRNAME_LENGTH_ERROR;
  }
  start_cluster = fat_chain_alloc(0,1);
  // try to find directory with such name
  findfirst(&findinfo, 0);
  while(findnext(&findinfo)) {
    if(file_name_match(dir_name,findinfo.direntry->sfn.name)) {
      return DIRECTORY_EXISTS;
    }
  }	
  if(!findfirst(&findinfo,1)) return NO_PLACE_FOR_DIRECTORY;;
  
   // Fill in the direntry
  FillDirEntry(findinfo.direntry,dir_name);

  findinfo.direntry->sfn.starting_cluster = htons(start_cluster);
  // Don't forget to set the attrib:
  findinfo.direntry->sfn.attrib = ATTRIB_SUBDIR;

	// Write the new data to MMC
  Sect_Write(findinfo.block);

  // Clear dir_entry of directory and create dot and dotdot directory inside
  Clear_Cluster(start_cluster);
  dir_sectors = Sect_File_Data() + (start_cluster * MBR.sectors_per_cluster);
  
  entry = (dir_entry*)Scratch;
  FillDirEntry(entry,".");

  entry->sfn.starting_cluster = htons(start_cluster);
  entry->sfn.attrib = ATTRIB_SUBDIR;

  entry = (dir_entry*)&Scratch[32];

  FillDirEntry(entry,"..");	

  entry->sfn.starting_cluster = htons(GetClusterOfParentDirectory());
  entry->sfn.attrib = ATTRIB_SUBDIR;


  Sect_Write(dir_sectors);
  return 0;
}

//-----------------------------------------------------------------------------
// rmdir
//-----------------------------------------------------------------------------
//
// Return Value : 1 if function suceeds other value if error occurs
// Parameters   : dir_name - directory name							
//
// Function removes directory 
//-----------------------------------------------------------------------------
BYTE rmdir(char* dir_name)
{
  unsigned dir_deep = 0;
  PREV_SEARCH prev_dir_block[40];
  char  first_part_of_dir[20];
  char  dir_tmp_name[20];
  char* tmp;
  
  // error if someone tries to removw root directory
  if(!strcmp(dir_name,"\\")) return 0;

  if((tmp = strstr(dir_name,"\\")) == NULL)
  {
  	strcpy(first_part_of_dir,dir_name);
  }
  else
  {
  	if(tmp == dir_name)
	{
		tmp = strstr(&dir_name[1],"\\");
		if(tmp != NULL)
			*tmp = 0;
		strcpy(first_part_of_dir,&dir_name[1]);
		if(tmp != NULL)
			*tmp = '\\';
	}
	else
	{
		*tmp = 0;
		strcpy(first_part_of_dir,dir_name);
		*tmp = '\\';
	}
  }

  if(!chngdir(dir_name)) return 0; 	
  if(!findfirst(&findinfo,0)) return 0;
  while(1)
  {
  	 if(findinfo.direntry->sfn.name[0]!=(char)0xE5)
	 {
	  	 if(!(findinfo.direntry->sfn.attrib & ATTRIB_LABEL))
		 {
			 if(findinfo.direntry->sfn.attrib & (ATTRIB_SUBDIR)) 
			 {
			 	if(!file_name_match(".",findinfo.direntry->sfn.name) && 
               !file_name_match("..",findinfo.direntry->sfn.name))
				{
				prev_dir_block[dir_deep].block = findinfo.block;
				prev_dir_block[dir_deep].offset = findinfo.offset;
				Get_File_Name(findinfo.direntry->sfn.name,dir_tmp_name);
				chngdir(dir_tmp_name);
				findfirst(&findinfo,0);
				dir_deep++;
				}
			 }
			 else
			 {
			  	  // Mark the direntry as "deleted" before freeing the fat chain.
				  // At this point, the findinfo is still valid in the 'Scratch' 
              // buffer.
				  // fat_chain_free() would overwrite the Scratch buffer.
				  findinfo.direntry->sfn.name[0]=0xE5; // Mark as "deleted"
			  	  Sect_Write(findinfo.block);
				  fat_chain_free(ntohs(findinfo.direntry->sfn.starting_cluster));	
				  Sect_Read(findinfo.block);		
			  }
		  }
	  }
	  if(!findnext(&findinfo)) 
	  {
	  	if(dir_deep)
		{
			dir_deep--;
			chngdir("..");
			findinfo.block = prev_dir_block[dir_deep].block;
			findinfo.offset = prev_dir_block[dir_deep].offset ;
			Sect_Read(findinfo.block);
			findinfo.direntry=(dir_entry*)(Scratch+findinfo.offset);
 		    findinfo.direntry->sfn.name[0]=0xE5; // Mark as "deleted"
		  	Sect_Write(findinfo.block);
			fat_chain_free(ntohs(findinfo.direntry->sfn.starting_cluster));	
			Sect_Read(findinfo.block);
			findinfo.direntry=(dir_entry*)(Scratch+findinfo.offset);
		} else
		{
			chngdir("..");
			break;
		} 
	  }
  }
  findfirst(&findinfo,0);
  while(!file_name_match(first_part_of_dir,findinfo.direntry->sfn.name)) {
    if(!findnext(&findinfo)) {
      return 0;
    }
  }
  findinfo.direntry->sfn.name[0]=0xE5; // Mark as "deleted"
  Sect_Write(findinfo.block);
  fat_chain_free(ntohs(findinfo.direntry->sfn.starting_cluster)); 
  return 1;
}


//-----------------------------------------------------------------------------
// fcreate
//-----------------------------------------------------------------------------
//
// Return Value : If ok returns TRUE
// Parameters   : find_info - pointer to info about file
//				  filename  - pointer to file name
//
// This function creates file
//-----------------------------------------------------------------------------

static BYTE fcreate(find_info* findinfo,char* filename)
{
  xdata BYTE i,j;
  // Find the first empty directory entry
  if(!findfirst(findinfo,1)) return 0;

  // Fill in the direntry
  for( i = 0; i < 10; i++) 
    findinfo->direntry->sfn.reserved[i] = 0;
  findinfo->direntry->sfn.time.i = findinfo->direntry->sfn.date.i = 0;
  findinfo->direntry->sfn.starting_cluster =  
  findinfo->direntry->sfn.filesize = 0;

  // Fill in the filename
  for( i = 0; i < 11; i++ ) 
    findinfo->direntry->sfn.name[i] = ' ';
  for( j = 0; j < 20; j++ ) {
    if(!filename[j] || filename[j] == '.') 
      break;
    if( j < 8 ) 
      findinfo->direntry->sfn.name[j] = toupper(filename[j]);
  }
  if( filename[j] == '.' ) {
    for( i = 0; i < 3; i++ ) {
      if(!filename[j+i+1] || filename[j+i+1]=='.')
        break;
      findinfo->direntry->sfn.name[8+i] = toupper(filename[j+i+1]);
    }
  }
  //for(i=0;i<11;i++) findinfo->direntry->sfn.name[i]=
  //toupper(findinfo->direntry->sfn.name[i]);

  // Don't forget to set the attrib:
  findinfo->direntry->sfn.attrib = ATTRIB_ARCHIVE;
	// Write the new data to MMC
  Sect_Write(findinfo->block);

	return 1;
}



//-----------------------------------------------------------------------------
// fopen
//-----------------------------------------------------------------------------
//
// Return Value : TRUE if file is open
// Parameters   : f - pointer to file structure info
//  			  filename - pointer to file name
//				  mode - pointer to opened file mode (read, write etc.)
//
// This function opens file
//-----------------------------------------------------------------------------

int fopen(FILE* f,char* filename,char* mode) 
{
	f->isopen = 0;

	if( mode[0] == 'w' ) { 
    fdelete(filename); 
  } // This is the most memory-efficient solution, not the most 
    // time-efficient solution.

  findfirst(&findinfo, 0);
  while(!file_name_match(filename,findinfo.direntry->sfn.name)) {
    if(!findnext(&findinfo)) {
      if(mode[0] == 'r') {
        return 0; // File not found.
      }
      if( mode[0] == 'w' || mode[0] == 'a' ) {
        if(!fcreate(&findinfo, filename)) {
          return 0; // File cannot be created.
        } else {
          break;
        }
      }
    }
  }

  f->sector_direntry=findinfo.block;
  f->offset_direntry=findinfo.offset;
  f->cluster_start=f->sector_current=
  ntohs(findinfo.direntry->sfn.starting_cluster);//*MBR.sectors_per_cluster;
  f->attrib=findinfo.direntry->sfn.attrib;
  f->size=ntohl(findinfo.direntry->sfn.filesize);

  if(mode[0]=='a') f->pos=f->size; else f->pos=0;
//	Print_File(f);
  return 	f->isopen=1;
}

//-----------------------------------------------------------------------------
// feof
//-----------------------------------------------------------------------------
//
// Return Value : End of file value
// Parameters   : f - pointer to file info structure
//
// This function printout size of file
//-----------------------------------------------------------------------------

int feof(FILE* f) 
{
  if(!f->isopen) return 1;
  return f->pos >= f->size;
}

//-----------------------------------------------------------------------------
// fat_chain
//-----------------------------------------------------------------------------
//
// Return Value : global number of sector 
// Parameters   : from - starting number of sector
//				  nr - relative number of sector
//
// Find the 'nr'-th sector in the fat chain starting at 'from'
//-----------------------------------------------------------------------------

static unsigned long fat_chain(unsigned long from,unsigned nr) 
{
  unsigned* xdata fat_table=Scratch;
  unsigned xdata sect,sect_prev=0;
  unsigned xdata cluster = nr/MBR.sectors_per_cluster;

  while(cluster) {
    sect = Sect_Fat1() + from/(Sect_Block_Size()/2);
    if(sect!=sect_prev) {
      Sect_Read(sect_prev=sect);
    }

    from=ntohs(fat_table[from%(Sect_Block_Size()/2)]);

    if(!(from>=2 && from<=0xFFEF)) {
      return 0xFFFFFFFF;
    }

    cluster--;
  }
  from *= MBR.sectors_per_cluster;
  from += nr%MBR.sectors_per_cluster;
  return from;
}

//-----------------------------------------------------------------------------
// fat_chain_free
//-----------------------------------------------------------------------------
//
// Return Value :  
// Parameters   : from - starting number of sector
//
// Function frees an entire fat chain, starting at 'from' until the end of the chain
//-----------------------------------------------------------------------------

static void fat_chain_free(unsigned from) 
{
  unsigned* xdata fat_table=Scratch;
  unsigned xdata sect,sect_prev=0;
  unsigned xdata index;

  if(from<2) return;

  sect = Sect_Fat1() + from/(Sect_Block_Size()/2);

  while(1) {
    if(sect!=sect_prev) {
      Sect_Read(sect_prev=sect);
    }

    index = from%(Sect_Block_Size()/2);
	
    from=ntohs(fat_table[index]);
		
    fat_table[index]=0x0000; // Free it

    if(!(from>=2 && from<=0xFFEF)) {
      Sect_Write_Multi_Fat(sect_prev);
      break;
    }

    sect = Sect_Fat1() + from/(Sect_Block_Size()/2);
    if(sect!=sect_prev) {
      Sect_Write_Multi_Fat(sect_prev);
    }
  }
}


//-----------------------------------------------------------------------------
// fat_chain_alloc
//-----------------------------------------------------------------------------
//
// Return Value : numer of allocated sector 
// Parameters   : from - starting number of sector
//				  nr - relative number of sector
//
// Allocate 'nr' of extra FAT blocks at the end of the chain that starts at 'from'
//-----------------------------------------------------------------------------

static unsigned fat_chain_alloc(unsigned from,unsigned nr) 
{
  unsigned* xdata fat_table=Scratch;
  unsigned xdata sect,sect_prev=0;
  unsigned xdata index;
  unsigned xdata alloced=0xFFFF;

	// Find free FAT entries, allocate them, and link them together.
  for(sect=Sect_Fat1();nr && sect<Sect_Fat2();sect++) {
    Sect_Read(sect);
		// (Skip first two FAT entries when looking for free blocks)
    for(index=((sect==Sect_Fat1())?2:0);index<Sect_Block_Size()/2;index++) {
      if(fat_table[index]==0x0000) { 		// It's free
        fat_table[index]=ntohs(alloced);// Allocate it (refer to previously 
                                        //   alloc'ed FAT entry).
        alloced = 						// Remember which FAT entry was alloc'ed
          (sect-Sect_Fat1()) * (Sect_Block_Size()/2) + index;
        if(!--nr) break;
      }
    }
    if(alloced!=0xFFFF) Sect_Write_Multi_Fat(sect);	// Write all FAT copies
  }

  // When we get here, 'alloced' contains the first FAT block in the alloc'ed chain
  // Find the end of the current FAT chain.
  // Make the end of the current FAT chain refer to the newly allocated FAT chain
  while(from>=2 && from<=0xFFEF && alloced!=0xFFFF) {
    sect = Sect_Fat1() + from/(Sect_Block_Size()/2);
    if(sect!=sect_prev) {
      Sect_Read(sect_prev=sect);
    }
    index = from%(Sect_Block_Size()/2);
    from=ntohs(fat_table[index]);

    if(from>=0xFFF8) {
      fat_table[index]=ntohs(alloced);
      Sect_Write_Multi_Fat(sect);
    }
  }

  return alloced;
}

//-----------------------------------------------------------------------------
// fread
//-----------------------------------------------------------------------------
//
// Return Value : amount of read bytes 
// Parameters   : f - pointer to file structure
//				  buffer - pinter to buffer
//				  count - number of bytes to read
//
// This function reads the file
//-----------------------------------------------------------------------------

unsigned fread(FILE* f,BYTE* buffer,unsigned count) 
{
  unsigned xdata cnt,total_cnt=0;
  if(!f->isopen || !count) return 0;

  // If you use the Scratch buffer as fread buffer, then
  // we cannot possibly support transfers consisting of
  // multiple Sect_Read() operations. The second Sect_Read
  // would overwrite the stored bytes from the first one.
  if(buffer>=Scratch && buffer<Scratch+Sect_Block_Size())
    count=min(count,Sect_Block_Size()-f->pos%Sect_Block_Size());

  while(count && !feof(f)) {
    f->sector_current = fat_chain(f->cluster_start,f->pos/Sect_Block_Size());

    Sect_Read(Sect_File_Data() + f->sector_current);

    cnt=min(Sect_Block_Size()-f->pos%Sect_Block_Size(),count);
    cnt=min(cnt,f->size-f->pos);

    memmove(buffer,Scratch+f->pos%Sect_Block_Size(),cnt); // MUST be overlap-safe 
                                                          // copy operation!
    total_cnt+=cnt;
    f->pos+=cnt;
    count-=cnt;
    buffer+=cnt;
  }

  return total_cnt;
}

//-----------------------------------------------------------------------------
// fwrite
//-----------------------------------------------------------------------------
//
// Return Value : amount of written bytes 
// Parameters   : f - pointer to file structure
//				  buffer - pinter to buffer
//				  count - number of bytes to write
//
// This function writes the file
//-----------------------------------------------------------------------------

unsigned fwrite(FILE* f,BYTE* buffer,unsigned count) 
{
  xdata unsigned cnt,total_cnt=0,xtra,alloced;
  xdata dir_entry* entry;
  if(!f->isopen || !count) return 0;


  // First, extend the file so it can hold all the data:
  if(f->pos+count>f->size) {
    // If the new EOF ends up in the next FAT block, an extra block must be allocated.
    // The number of blocks needed to store X bytes = 1+(X-1)/512 if X!=0, or =0 if X==0.
    // We will need to store 'pos+count' bytes after the write operation
    // This means we need 1+(pos+count-1)/512 blocks after the write operation
    // We currently have 'size' bytes in the file, or 1+(size-1)/512 blocks (or 0 if size==0).
    // So, we need to allocate (1+(pos+count-1)/512) - (1+(size-1)/512) extra blocks
    xtra=(1+((f->pos+count-1)/Sect_Block_Size())/MBR.sectors_per_cluster); 
    if ( f->size )
      xtra -= ( 1 +(( f->size - 1 ) / Sect_Block_Size() ) / MBR.sectors_per_cluster );
		

    if(xtra) {
      if(0xFFFF==(alloced=fat_chain_alloc(f->sector_current/MBR.sectors_per_cluster,xtra))) 
        return 0;
    }

		// Modify the direntry for this file:
    Sect_Read(f->sector_direntry);
    entry = (dir_entry*)(Scratch+f->offset_direntry);
    if((entry->sfn.filesize==0) && (entry->sfn.starting_cluster<2 || 
                                entry->sfn.starting_cluster>=0xFFF0)) {
      entry->sfn.starting_cluster=ntohs(f->cluster_start=alloced);
    } 
    entry->sfn.filesize=ntohl(f->size=f->pos+count);
    f->attrib=(entry->sfn.attrib|=ATTRIB_ARCHIVE);
    Sect_Write(f->sector_direntry);
  }

  // Now we are sure the fwrite() operation can be performed
  // in the existing file data blocks. Either because the file 
  // was big enough to start with, or because we have just
  // allocated extra blocks for the new data.
  while(count && !feof(f)) {
    f->sector_current = fat_chain(f->cluster_start,f->pos/Sect_Block_Size());
		

    Sect_Read(Sect_File_Data() + f->sector_current);
  //	Print_File(f);
  //	print_scratch();

    cnt=min(Sect_Block_Size()-f->pos%Sect_Block_Size(),count);
    cnt=min(cnt,f->size-f->pos);

    memmove(Scratch+f->pos%Sect_Block_Size(),buffer,cnt);

  //	print_scratch();
    Sect_Write(Sect_File_Data() + f->sector_current);

    total_cnt+=cnt;
    f->pos+=cnt;
    count-=cnt;
    buffer+=cnt;
  }

  return total_cnt;
}

//-----------------------------------------------------------------------------
// fclose
//-----------------------------------------------------------------------------
//
// Return Value : 
// Parameters   : f - pointer to file structure

//
// This function closes the file
//-----------------------------------------------------------------------------

void fclose(FILE* f) 
{
  f->isopen=0;
}

//-----------------------------------------------------------------------------
// fdelete
//-----------------------------------------------------------------------------
//
// Return Value :TRUE if everything is ok
// Parameters   : name - pointer to filename
//
//
// This function deletes the file
//-----------------------------------------------------------------------------

int fdelete(char* name) 
{
  findfirst(&findinfo,0);
  while(!file_name_match(name,findinfo.direntry->sfn.name)) {
    if(!findnext(&findinfo)) {
      return 0;
    }
  }

  // Do not delete subdirectories or labels:
  if(findinfo.direntry->sfn.attrib & (ATTRIB_SUBDIR|ATTRIB_LABEL)) return 0;

  // Mark the direntry as "deleted" before freeing the fat chain.
  // At this point, the findinfo is still valid in the 'Scratch' buffer.
  // fat_chain_free() would overwrite the Scratch buffer.
  findinfo.direntry->sfn.name[0]=0xE5; // Mark as "deleted"
  Sect_Write(findinfo.block);
  fat_chain_free(ntohs(findinfo.direntry->sfn.starting_cluster));

  return 1;
}

//-------------------directory functions---------------------
//--------- findfirst, findnext directory functions ---------

static BYTE findvalid(find_info* findinfo) 
{
  xdata char n0 = findinfo->direntry->sfn.name[0];
  if(findinfo->findempty) {
    return (n0==(char)0xE5) || (n0=='\0');
  }
  return (n0!=(char)0xE5) && (n0>' ') && (findinfo->direntry->sfn.attrib!=0x0F);
}


BYTE findfirst(find_info* findinfo,BYTE empty) 
{
  Sect_Read(findinfo->block = Current_Dir_Block);

  findinfo->findempty=empty;

  findinfo->direntry=(dir_entry*)(Scratch+(findinfo->offset=0));
  if(findvalid(findinfo))
    return 1;
  return findnext(findinfo);
}


BYTE findnext(find_info* findinfo) 
{
  xdata BYTE bRoot = (Current_Dir_Block == Sect_Root_Dir());
  

  do {
    if((findinfo->offset+=sizeof(dir_entry))>=Sect_Block_Size()) {
      xdata unsigned long dir_next_cluster_block = 
                     Get_First_Block_Directory_Cluster(findinfo->block);
      if(bRoot &&  (findinfo->block>=Sect_Root_Dir_Last()))
      {
      //	printf("NOT FOUND\r\n");
        return 0;
      }
	  else if((!bRoot) && (dir_next_cluster_block != 0xffffffff) && 
	  		(findinfo->block>=(dir_next_cluster_block + MBR.sectors_per_cluster-1)))
	  {
	  	// read next cluster occupied by directory	
	  	xdata unsigned long next_next_block = Get_First_Block_Of_Next_Cluster(
										Get_Cluster_From_Sector(dir_next_cluster_block));
			
		if(next_next_block == (0xFFFFFFFF))
		{
			if(!findinfo->findempty)
				return 0;
			else
			{
				xdata new_cluster = 
                  fat_chain_alloc(Get_Cluster_From_Sector(dir_next_cluster_block),1);
				if(new_cluster == 0xFFFF)
					return 0;
				next_next_block = Get_First_Sector(new_cluster);
				Clear_Cluster(new_cluster);
			}	
		}

		dir_next_cluster_block = next_next_block;
		findinfo->offset=0;
		Sect_Read(findinfo->block = dir_next_cluster_block);
	  }
      else {
        findinfo->offset=0;
        Sect_Read(++findinfo->block);

      }
    }
		
		
    findinfo->direntry=(dir_entry*)(Scratch+findinfo->offset);
  } while(!findvalid(findinfo));

  return 1;
}

#else

//---------------------------------------------------------
/* Copy of functions for devices different then F340*/
//---------------------------------------------------------

static BYTE fcreate(find_info* findinfo,char* filename) 
{
  BYTE i,j;
  // Find the first empty directory entry
  if(!findfirst(findinfo,1)) return 0;

  // Fill in the direntry
  for( i = 0; i < 10; i++) 
    findinfo->direntry->sfn.reserved[i] = 0;
  findinfo->direntry->sfn.time.i = findinfo->direntry->sfn.date.i = 0;
  findinfo->direntry->sfn.starting_cluster = findinfo->direntry->sfn.filesize = 0;

  // Fill in the filename
  for( i = 0; i < 11; i++ ) 
    findinfo->direntry->sfn.name[i] = ' ';
  for( j = 0; j < 20; j++ ) {
    if(!filename[j] || filename[j] == '.') 
      break;
    if( j < 8 ) 
      findinfo->direntry->sfn.name[j] = toupper(filename[j]);
  }
  if( filename[j] == '.' ) {
    for( i = 0; i < 3; i++ ) {
      if(!filename[j+i+1] || filename[j+i+1]=='.') 
        break;
      findinfo->direntry->sfn.name[8+i] = toupper(filename[j+i+1]);
    }
  }
  //for(i=0;i<11;i++) findinfo->direntry->sfn.name[i]=
  //toupper(findinfo->direntry->sfn.name[i]);

  // Don't forget to set the attrib:
  findinfo->direntry->sfn.attrib = ATTRIB_ARCHIVE;

	// Write the new data to MMC
  Sect_Write(findinfo->block);

	return 1;
}


static find_info xdata findinfo; // Shared find_info for fopen() and fdelete()

int fopen(FILE* f,char* filename,char* mode) 
{
	f->isopen = 0;

	if( mode[0] == 'w' ) { 
    fdelete(filename); 
  } // This is the most memory-efficient solution, not the most time-efficient solution.

  findfirst(&findinfo, 0);
  while(!file_name_match(filename,findinfo.direntry->sfn.name)) {
    if(!findnext(&findinfo)) {
      if(mode[0] == 'r') {
        return 0; // File not found.
      }
      if( mode[0] == 'w' || mode[0] == 'a' ) {
        if(!fcreate(&findinfo, filename)) {
          return 0; // File cannot be created.
        } else {
          break;
        }
      }
    }
  }

  f->sector_direntry=findinfo.block;
  f->offset_direntry=findinfo.offset;
  f->cluster_start=f->sector_current=
  ntohs(findinfo.direntry->sfn.starting_cluster);//*MBR.sectors_per_cluster;
  f->attrib=findinfo.direntry->sfn.attrib;
  f->size=ntohl(findinfo.direntry->sfn.filesize);

  if(mode[0]=='a') f->pos=f->size; else f->pos=0;
//	Print_File(f);
  return 	f->isopen=1;
}

/*
int fexists(char* filename) {
	FILE xdata f;
	if(fopen(&f,filename,"r")) {
		fclose(&f);
		return 1;
	}
	return 0;
}
*/

/*
int fseek(FILE* f,long offset,int origin) {
	if(!f->isopen) return 0;
	switch(origin) {
		case SEEK_SET:default:
			if(offset>=0)
				f->pos=offset;
			else
				return 0;
			break;
		case SEEK_END:
			f->pos=f->size+offset;
			break;
		case SEEK_CUR:
			f->pos+=offset;
			break;
	}

	return 1;
}
*/

/*
DWORD ftell(FILE* f) {
	if(!f->isopen) return 0;
	return f->pos;
}
*/

int feof(FILE* f) 
{
  if(!f->isopen) return 1;
  return f->pos >= f->size;
}

// Find the 'nr'-th sector in the fat chain starting at 'from'
static unsigned long fat_chain(unsigned long from,unsigned nr) 
{
  unsigned* xdata fat_table=Scratch;
  unsigned xdata sect,sect_prev=0;
  unsigned xdata cluster = nr/MBR.sectors_per_cluster;

  while(cluster) {
    sect = Sect_Fat1() + from/(Sect_Block_Size()/2);
    if(sect!=sect_prev) {
      Sect_Read(sect_prev=sect);
    }

    from=ntohs(fat_table[from%(Sect_Block_Size()/2)]);

    if(!(from>=2 && from<=0xFFEF)) {
      return 0xFFFFu;
    }

    cluster--;
  }
  from *= MBR.sectors_per_cluster;
  from += nr%MBR.sectors_per_cluster;
  return from;
}

// Free an entire fat chain, starting at 'from' until the end of the chain
static void fat_chain_free(unsigned from) 
{
  unsigned* xdata fat_table=Scratch;
  unsigned xdata sect,sect_prev=0;
  unsigned xdata index;

  if(from<2) return;

  sect = Sect_Fat1() + from/(Sect_Block_Size()/2);

  while(1) {
    if(sect!=sect_prev) {
      Sect_Read(sect_prev=sect);
    }

    index = from%(Sect_Block_Size()/2);
	
    from=ntohs(fat_table[index]);
		
    fat_table[index]=0x0000; // Free it

    if(!(from>=2 && from<=0xFFEF)) {
      Sect_Write_Multi_Fat(sect_prev);
      break;
    }

    sect = Sect_Fat1() + from/(Sect_Block_Size()/2);
    if(sect!=sect_prev) {
      Sect_Write_Multi_Fat(sect_prev);
    }
  }
}


// Allocate 'nr' extra FAT blocks at the end of the chain that starts at 'from':
static unsigned fat_chain_alloc(unsigned from,unsigned nr) 
{
  unsigned* xdata fat_table=Scratch;
  unsigned xdata sect,sect_prev=0;
  unsigned xdata index;
  unsigned xdata alloced=0xFFFF;

	// Find free FAT entries, allocate them, and link them together.
  for(sect=Sect_Fat1();nr && sect<Sect_Fat2();sect++) {
    Sect_Read(sect);
		// (Skip first two FAT entries when looking for free blocks)
    for(index=((sect==Sect_Fat1())?2:0);index<Sect_Block_Size()/2;index++) {
      if(fat_table[index]==0x0000) { 		// It's free
        fat_table[index]=ntohs(alloced);// Allocate it (refer to previously alloc'ed FAT entry).
        alloced = 						// Remember which FAT entry was alloc'ed
          (sect-Sect_Fat1()) * (Sect_Block_Size()/2) + index;
        if(!--nr) break;
      }
    }
    if(alloced!=0xFFFF) Sect_Write_Multi_Fat(sect);	// Write all FAT copies
  }

  // When we get here, 'alloced' contains the first FAT block in the alloc'ed chain
  // Find the end of the current FAT chain.
  // Make the end of the current FAT chain refer to the newly allocated FAT chain
  while(from>=2 && from<=0xFFEF && alloced!=0xFFFF) {
    sect = Sect_Fat1() + from/(Sect_Block_Size()/2);
    if(sect!=sect_prev) {
      Sect_Read(sect_prev=sect);
    }
    index = from%(Sect_Block_Size()/2);
    from=ntohs(fat_table[index]);

    if(from>=0xFFF8) {
      fat_table[index]=ntohs(alloced);
      Sect_Write_Multi_Fat(sect);
    }
  }

  return alloced;
}

unsigned fread(FILE* f,BYTE* buffer,unsigned count) 
{
  unsigned xdata cnt,total_cnt=0;
  if(!f->isopen || !count) return 0;

  // If you use the Scratch buffer as fread buffer, then
  // we cannot possibly support transfers consisting of
  // multiple Sect_Read() operations. The second Sect_Read
  // would overwrite the stored bytes from the first one.
  if(buffer>=Scratch && buffer<Scratch+Sect_Block_Size())
    count=min(count,Sect_Block_Size()-f->pos%Sect_Block_Size());

  while(count && !feof(f)) {
    f->sector_current = fat_chain(f->cluster_start,f->pos/Sect_Block_Size());

    Sect_Read(Sect_File_Data() + f->sector_current);

    cnt=min(Sect_Block_Size()-f->pos%Sect_Block_Size(),count);
    cnt=min(cnt,f->size-f->pos);

    memmove(buffer,Scratch+f->pos%Sect_Block_Size(),cnt); // MUST be overlap-safe copy operation!
    total_cnt+=cnt;
    f->pos+=cnt;
    count-=cnt;
    buffer+=cnt;
  }

  return total_cnt;
}

unsigned fwrite(FILE* f,BYTE* buffer,unsigned count) 
{
  unsigned cnt,total_cnt=0,xtra,alloced;
  dir_entry* entry;
  if(!f->isopen || !count) return 0;


  // First, extend the file so it can hold all the data:
  if(f->pos+count>f->size) {
    // If the new EOF ends up in the next FAT block, an extra block must be 
    // allocated.
    // The number of blocks needed to store X bytes = 1+(X-1)/512 if X!=0, 
    // or =0 if X==0.
    // We will need to store 'pos+count' bytes after the write operation
    // This means we need 1+(pos+count-1)/512 blocks after the write operation
    // We currently have 'size' bytes in the file, or 1+(size-1)/512 blocks 
    // (or 0 if size==0).
    // So, we need to allocate (1+(pos+count-1)/512) - (1+(size-1)/512) 
    // extra blocks
    xtra=(1+((f->pos+count-1)/Sect_Block_Size())/MBR.sectors_per_cluster); 
    if ( f->size )
      xtra -= 
      ( 1 +(( f->size - 1 ) / Sect_Block_Size() ) / MBR.sectors_per_cluster );
		

    if(xtra) {
      if(0xFFFF==
      (alloced=fat_chain_alloc(f->sector_current/MBR.sectors_per_cluster,xtra)))
        return 0;
    }

		// Modify the direntry for this file:
    Sect_Read(f->sector_direntry);
    entry = (dir_entry*)(Scratch+f->offset_direntry);
    if((entry->sfn.filesize==0) && (entry->sfn.starting_cluster<2 || 
    entry->sfn.starting_cluster>=0xFFF0)) {
      entry->sfn.starting_cluster=ntohs(f->cluster_start=alloced);
    } 
    entry->sfn.filesize=ntohl(f->size=f->pos+count);
    f->attrib=(entry->sfn.attrib|=ATTRIB_ARCHIVE);
    Sect_Write(f->sector_direntry);
  }

  // Now we are sure the fwrite() operation can be performed
  // in the existing file data blocks. Either because the file 
  // was big enough to start with, or because we have just
  // allocated extra blocks for the new data.
  while(count && !feof(f)) {
    f->sector_current = fat_chain(f->cluster_start,f->pos/Sect_Block_Size());
		

    Sect_Read(Sect_File_Data() + f->sector_current);
  //	Print_File(f);
  //	print_scratch();

    cnt=min(Sect_Block_Size()-f->pos%Sect_Block_Size(),count);
    cnt=min(cnt,f->size-f->pos);

    memmove(Scratch+f->pos%Sect_Block_Size(),buffer,cnt);

  //	print_scratch();
    Sect_Write(Sect_File_Data() + f->sector_current);

    total_cnt+=cnt;
    f->pos+=cnt;
    count-=cnt;
    buffer+=cnt;
  }

  return total_cnt;
}

void fclose(FILE* f) 
{
  f->isopen=0;
}

int fdelete(char* name) 
{
  findfirst(&findinfo,0);
  while(!file_name_match(name,findinfo.direntry->sfn.name)) {
    if(!findnext(&findinfo)) {
      return 0;
    }
  }

  // Do not delete subdirectories or labels:
  if(findinfo.direntry->sfn.attrib & (ATTRIB_SUBDIR|ATTRIB_LABEL)) return 0;

  // Mark the direntry as "deleted" before freeing the fat chain.
  // At this point, the findinfo is still valid in the 'Scratch' buffer.
  // fat_chain_free() would overwrite the Scratch buffer.
  findinfo.direntry->sfn.name[0]=0xE5; // Mark as "deleted"
  Sect_Write(findinfo.block);
  fat_chain_free(ntohs(findinfo.direntry->sfn.starting_cluster));

  return 1;
}

//--------- findfirst, findnext directory functions ---------

static BYTE findvalid(find_info* findinfo) 
{
  char n0 = findinfo->direntry->sfn.name[0];
  if(findinfo->findempty) {
    return (n0==(char)0xE5) || (n0=='\0');
  }
  return (n0!=(char)0xE5) && (n0>' ') && (findinfo->direntry->sfn.attrib!=0x0F);
}

BYTE findfirst(find_info* findinfo,BYTE empty) 
{
  Sect_Read(findinfo->block = Sect_Root_Dir());

  findinfo->findempty=empty;

  findinfo->direntry=(dir_entry*)(Scratch+(findinfo->offset=0));
  if(findvalid(findinfo))
    return 1;
  return findnext(findinfo);
}
void print_info(find_info* info);

BYTE findnext(find_info* findinfo) 
{
  do {
    if((findinfo->offset+=sizeof(dir_entry))>=Sect_Block_Size()) {
      if(findinfo->block>=Sect_Root_Dir_Last())
      {
      //	printf("NOT FOUND\r\n");
        return 0;
      }
      else {
        findinfo->offset=0;
        Sect_Read(++findinfo->block);

      }
    }
		
		
    findinfo->direntry=(dir_entry*)(Scratch+findinfo->offset);
  } while(!findvalid(findinfo));

  return 1;
}

#endif