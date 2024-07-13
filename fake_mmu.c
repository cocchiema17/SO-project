#include "fake_mmu.h"

LinearAddress getLinearAddress(MMU* mmu, LogicalAddress logical_address){
  //2. check if the segment is in the segment table
  assert (logical_address.segment_id < mmu->num_segments && "segment out of bounds" );
  
  //2. select the descriptor from the mmu table
  SegmentDescriptor descriptor=mmu->segments[logical_address.segment_id];

  //3. see if the segment is valid
  assert((descriptor.flags) & Valid && "invalid_segment");
  assert((logical_address.page_number <descriptor.limit) && "out_of_segment_limit");

  LinearAddress linear_address;
  linear_address.page_number=descriptor.base+logical_address.page_number;
  linear_address.offset=logical_address.offset;

  //5. return the address
  return linear_address;
}


PhysicalAddress getPhysicalAddress(MMU* mmu, LinearAddress linear_address) {
  //1. get the page number
  PageEntry page_entry=mmu->pages[linear_address.page_number];
  //assert( page_entry.flags & Valid && "invalid page");
  if (page_entry.flags & Valid) {
    mmu->pages[linear_address.page_number].flags |= Read | Reference;
    uint32_t frame_number=page_entry.frame_number;
    //5. combine the entry of the page table with the offset, and get the physical address
    return (frame_number<<FRAME_NBITS)|linear_address.offset;
  }
  else {
    printf("Page fault\n");
    MMU_exception(mmu, page_entry.frame_number);
  }
}

MMU* init_MMU(uint32_t num_segments, uint32_t num_pages, const char* swap_file){
  MMU* mmu = (MMU*)malloc(sizeof(MMU));
  assert(mmu != NULL && "Error mmu malloc");
  mmu->segments = (SegmentDescriptor*)malloc(sizeof(SegmentDescriptor) * num_segments);
  assert(mmu->segments != NULL && "Error segments malloc");
  mmu->num_segments = num_segments;
  mmu->pages = (PageEntry*)malloc(sizeof(PageEntry) * num_pages);
  assert(mmu->pages != NULL && "Error pages malloc");
  mmu->num_pages = num_pages;
  mmu->ram = (char*)malloc(MAX_MEMORY);
  assert(mmu->ram != NULL && "Error ram malloc");
  mmu->used_memory = 0;

  // inizializzazione delle pagine
  for(int i = 0; i < num_pages; i++) {
    mmu->pages[i].frame_number = PAGES_NUM - i - 1; // ordine inverso
    mmu->pages[i].flags = Valid;
  }

  // inizializzazione dei segmenti
  for (uint32_t i = 0; i < num_segments; i++) {
        if (i == 0)
            mmu->segments[i].base = 0;
        else
            mmu->segments[i].base = mmu->segments[i-1].base + mmu->segments[i-1].limit;
        mmu->segments[i].limit = PAGES_NUM / SEGMENTS_NUM;
        mmu->segments[i].flags = Valid;
  }

  // Store the page table at the beginning of the RAM
  memcpy(mmu->ram, mmu->pages, sizeof(PageEntry) * num_pages);
  mmu->used_memory = sizeof(PageEntry) * num_pages;
  mmu->pages = (PageEntry*) mmu->ram;

  mmu->swap_file = fopen(swap_file, "w+");
  if (!mmu->swap_file) {
    printf("Error opening swap file");
    exit(EXIT_FAILURE);
  }

   mmu->pointer = 0;  // Initialize pointer for the second chance algorithm

   return mmu;
}

void printSegmentsTable(MMU* mmu) {
  SegmentDescriptor* pointer = mmu->segments;
  for (int i = 0; i < mmu->num_segments; i++) {
    printf("Segment %d: base = %d, limit = %d, flags = %d\n",
      i, pointer->base, pointer->limit, pointer->flags);
    pointer++;
  }
}

void printPagesTable(MMU* mmu) {
  PageEntry* pointer = mmu->pages;
  for (int i = 0; i < mmu->num_pages; i++) {
    printf("Page %d: frame_number = %d, flags = %d\n",
      i, pointer->frame_number, pointer->flags);
    pointer++;
  }
}

void printRam(MMU* mmu) {
  char* pointer = mmu->ram;
  for (int i = 0; i < MAX_MEMORY; i++) {
    printf("Ram [%d = %x]: %x\n", i, i, *pointer);
    pointer++;
  }
}

int isRamFull(MMU* mmu) {
  return (mmu->used_memory >= MAX_MEMORY);
}

void generateLogicalAddress(MMU* mmu) {

  printf("Generazione degli indirizzi logici...");
  for (int s = 0; s < mmu->num_segments; s++) {
    for (int p = 0; p < mmu->segments[s].limit; p++){
      for (int o = 0; o < PAGE_SIZE; o++) {
        LogicalAddress logical_address = {
          .segment_id = s,
          .page_number = p,
          .offset = o
        };
        char c = 'A';
        //printf("%x %x %x\n", s, p, o);

        // inserire sul file la coppia <indirizzo logico, dato>
        fprintf(mmu->swap_file, "%x%x%x %c\n",
         logical_address.segment_id, logical_address.page_number, logical_address.offset, c);
      }
    } 
  }
  printf("completata!\n");
}

char* MMU_readByte(MMU* mmu, int pos) {
  assert(pos >= 0 && pos < MAX_MEMORY && "Invalid physical address");
  char* pointer = mmu->ram;
  pointer += pos;  
  return pointer;
}

void MMU_writeByte(MMU* mmu, int pos, char c) {
  assert(pos >= 0 && pos < MAX_MEMORY && "Invalid physical address");
  char* pointer = mmu->ram;
  pointer += pos;
  *pointer = c;
  mmu->used_memory++;
}

void MMU_exception(MMU* mmu, int pos) {
  exit(EXIT_FAILURE);
}

void cleanup_MMU(MMU* mmu) {
    fclose(mmu->swap_file);
    free(mmu->segments);
    free(mmu->ram);
    free(mmu);
}
