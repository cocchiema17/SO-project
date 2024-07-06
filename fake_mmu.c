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


uint32_t getPhysicalAddress(MMU* mmu, LinearAddress linear_address) {
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

  mmu->swap_file = fopen(swap_file, );
  if (!mmu->swap_file) {
    printf("Error opening swap file");
    exit(EXIT_FAILURE);
  }

}
