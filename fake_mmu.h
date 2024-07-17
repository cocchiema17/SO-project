#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

// bits used for addressing
#define ADDRESS_NBITS 20 

// number of bits for a page selector
#define PAGE_NBITS 12

// number of bytes in address space
#define MAX_MEMORY (1<<ADDRESS_NBITS) // 20 bits = 2^20 B = 1 MB

#define PAGE_FLAGS_NBITS 5

// number of pages
#define PAGES_NUM  (1<<PAGE_NBITS)

#define FRAME_NBITS 12 //(ADDRESS_NBITS-PAGE_NBITS)


// size of a memory page
#define PAGE_SIZE (1<<FRAME_NBITS)

typedef enum {
  Valid=0x1,
  Read=0x2,
  Write=0x4,
  Unswappable=0x8,
  Reference = 0x10 
} PageFlags;

typedef struct LinearAddress{
  uint32_t page_number: PAGE_NBITS;
  uint32_t offset: FRAME_NBITS;
} LinearAddress;

typedef struct PageEntry {
  uint32_t frame_number: PAGE_NBITS;
  PageFlags flags: PAGE_FLAGS_NBITS;
} PageEntry;

typedef struct MMU {
  PageEntry *pages;
  uint32_t num_pages;
  char* ram;
  int busyFramesInRam;  // number of frames in ram that are currently in use
  PageEntry* pagesInRam;  // Pages stored in RAM, on which the second chance algorithm will be performed
  FILE* swap_file;
  int pageFault;
} MMU;

typedef uint32_t PhysicalAddress;

MMU* init_MMU(uint32_t num_pages, const char* swap_file);

void printPagesTable(MMU* mmu);

void printPagesTableInRam(MMU* mmu);

void printRam(MMU* mmu);

int isRamFull(MMU* mmu);

char* MMU_readByte(MMU* mmu, LinearAddress pos);

void MMU_writeByte(MMU* mmu, LinearAddress pos, char c);

void MMU_exception(MMU* mmu, LinearAddress pos);

void swap_out(MMU* mmu, int frame_number, int frame_to_swap);

void swap_in(MMU* mmu, int frame_number, int frame_to_swap);

void cleanup_MMU(MMU* mmu);
