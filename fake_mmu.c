#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "bits_macros.h"
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
  assert( page_entry.flags & Valid && "invalid page");
  uint32_t frame_number=page_entry.frame_number;
  //5. combine the entry of the page table with the offset, and get the physical address
  return (frame_number<<FRAME_NBITS)|linear_address.offset;
}

MMU* init_mmu(uint32_t num_segments, uint32_t num_pages, const char* swap_file){
  MMU* mmu = (MMU*)malloc(sizeof(MMU));
  mmu->segments = (SegmentDescriptor*)malloc(sizeof(SegmentDescriptor) * num_segments);
  mmu->num_segments = num_segments;
  mmu->pages = (PageEntry*)malloc(sizeof(PageEntry) * num_pages);
  mmu->num_pages = num_pages;
  mmu->ram = (char*)malloc(MAX_MEMORY);

  // inizializzazione delle pagine
  for(int i = 0; i < num_pages; i++) {
    mmu->pages[i]->flags = 0;
  }

  // Store the page table at the beginning of the RAM
  memcpy(mmu->ram, mmu->pages, sizeof(PageEntry) * num_pages);

  mmu->swap_file = fopen(swap_file, "wb+");
  if (!mmu->swap_file) {
    printf("Error opening swap file");
    exit(EXIT_FAILURE);
  }

   mmu->pointer = 0;  // Initialize pointer for the second chance algorithm
}

void MMU_writeByte(MMU* mmu, int pos, char c) {
    LogicalAddress logical_address = {
        .segment_id = (pos >> (PAGE_NBITS + FRAME_NBITS)) & ((1 << SEGMENT_NBITS) - 1),
        .page_number = (pos >> FRAME_NBITS) & ((1 << PAGE_NBITS) - 1),
        .offset = pos & ((1 << FRAME_NBITS) - 1)
    };
    LinearAddress linear_address = getLinearAddress(mmu, logical_address);
    PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);

    mmu->ram[physical_address] = c;
    mmu->pages[linear_address.page_number].flags |= Write | Reference;
}

char MMU_readByte(MMU* mmu, int pos) {
    LogicalAddress logical_address = {
        .segment_id = (pos >> (PAGE_NBITS + FRAME_NBITS)) & ((1 << SEGMENT_NBITS) - 1),
        .page_number = (pos >> FRAME_NBITS) & ((1 << PAGE_NBITS) - 1),
        .offset = pos & ((1 << FRAME_NBITS) - 1)
    };
    LinearAddress linear_address = getLinearAddress(mmu, logical_address);
    PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);

    mmu->pages[linear_address.page_number].flags |= Read | Reference;
    return mmu->ram[physical_address];
}


void cleanup_mmu(MMU* mmu) {
    fclose(mmu->swap_file);
    free(mmu->segments);
    free(mmu->pages);
    free(mmu->ram);
    free(mmu);
}
