//#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// bits used for addressing
#define ADDRESS_NBITS 8 //20 

// number of bits used as segment descriptor
#define SEGMENT_NBITS 2 //4

// number of bits for a page selector
#define PAGE_NBITS 4    //14

// number of bytes in address space
#define MAX_MEMORY (1<<ADDRESS_NBITS) // 20 bits = 2^20 B = 1 MB

#define SEGMENT_FLAGS_NBITS 3
#define PAGE_FLAGS_NBITS 5

// total number of segments
#define SEGMENTS_NUM (1<<SEGMENT_NBITS)

#define LOGICAL_ADDRESS_NBITS (ADDRESS_NBITS+SEGMENT_NBITS) // 20+4=24 bits => virtual memory space = 2^24 B = 16 MB

// number of pages
#define PAGES_NUM  (1<<PAGE_NBITS)

#define FRAME_NBITS (ADDRESS_NBITS-PAGE_NBITS)
		    

// size of a memory page
#define PAGE_SIZE (1<<FRAME_NBITS)

typedef enum {
  Valid=0x1,
  Read=0x2,
  Write=0x4,
  Unswappable=0x8,
  Reference = 0x10 
} PageFlags;

typedef struct SegmentDescriptor{
  uint32_t base: PAGE_NBITS;  
  uint32_t limit: PAGE_NBITS;
  uint32_t flags: SEGMENT_FLAGS_NBITS;
} SegmentDescriptor;

typedef struct LogicalAddress{
  uint32_t segment_id: SEGMENT_NBITS;
  uint32_t page_number: PAGE_NBITS;
  uint32_t offset: FRAME_NBITS;
} LogicalAddress;


typedef struct LinearAddress{
  uint32_t page_number: PAGE_NBITS;
  uint32_t offset: FRAME_NBITS;
} LinearAddress;

typedef struct PageEntry {
  uint32_t frame_number: PAGE_NBITS;
  PageFlags flags: PAGE_FLAGS_NBITS;
} PageEntry;

typedef struct MMU {
  SegmentDescriptor* segments;
  uint32_t num_segments; // number of good segments
  PageEntry *pages;
  uint32_t num_pages;
  char* ram;
  FILE* swap_file;
  int pointer;  // Pointer for the second chance algorithm
} MMU;

// applies segmentation to an address and returns linear address
LinearAddress getLinearAddress(MMU* mmu, LogicalAddress logical_address);

typedef uint32_t PhysicalAddress;

// applies pagination to an address and returns the physical address
PhysicalAddress getPhysicalAddress(MMU* mmu, LinearAddress linear_address);

MMU* init_MMU(uint32_t num_segments, uint32_t num_pages, const char* swap_file);

void printRam(MMU* mmu);

void generateLogicalAddress(MMU* mmu);

char* MMU_readByte(MMU* mmu, int pos);

void MMU_writeByte(MMU* mmu, int pos, char c);

void MMU_exception(MMU* mmu, int pos);

void cleanup_MMU(MMU* mmu);
