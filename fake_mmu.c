#include "fake_mmu.h"

LinearAddress getLinearAddress(MMU* mmu, LogicalAddress logical_address){
  printf("Logical address: %x%x%x ", 
    logical_address.segment_id, logical_address.page_number, logical_address.offset);
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
  printf("=> Linear address: %x%x\n", linear_address.page_number, linear_address.offset);
  return linear_address;
}


PhysicalAddress getPhysicalAddress(MMU* mmu, LinearAddress linear_address) {
  printf("Linear address: %x%x ", linear_address.page_number, linear_address.offset);
  PageEntry page_entry = mmu->pages[linear_address.page_number];

  if (!(page_entry.flags & Valid)) {
    MMU_exception(mmu, linear_address.page_number);
    page_entry = mmu->pages[linear_address.page_number];
  }

  page_entry.flags |= Read | Reference;
  mmu->pages[linear_address.page_number] = page_entry;

  uint32_t frame_number = page_entry.frame_number;
  PhysicalAddress physicalAddress = (frame_number << FRAME_NBITS) | linear_address.offset;
  printf("=> Physical address: %x = %d\n", physicalAddress, physicalAddress);
  return physicalAddress;
}

MMU* init_MMU(uint32_t num_segments, uint32_t num_pages, const char* swap_file) {
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
  mmu->pageFault = 0;

  // inizializzazione delle pagine
  int unswappable_pages = num_pages / sizeof(PageEntry);
  for(int i = 0; i < num_pages; i++) {
    //mmu->pages[i].frame_number = PAGES_NUM - i - 1; // ordine inverso
    mmu->pages[i].frame_number = i;
    if (i < unswappable_pages)
      mmu->pages[i].flags |= Valid | Unswappable;
    else
      mmu->pages[i].flags = 0;
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

  mmu->swap_file = fopen(swap_file, "wb+");
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
    printf("Ram [%d = %x]: %x = %c\n", i, i, *pointer, *pointer);
    pointer++;
  }
}

int isRamFull(MMU* mmu) {
  return (mmu->used_memory >= MAX_MEMORY);
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

void MMU_exception(MMU* mmu, int page_number) {
  printf("Page fault at page number %d\n", page_number);
  mmu->pageFault++;
  if(isRamFull(mmu)) {
    printf("Ram is full\n");
    int frame_to_swap = mmu->pointer;
    // second chance algorithm
    while (mmu->pages[frame_to_swap].flags & Unswappable || (mmu->pages[frame_to_swap].flags & Reference)) {
      mmu->pages[frame_to_swap].flags ^= Reference;
      frame_to_swap = (frame_to_swap + 1) % PAGES_NUM;
    }

    printf("Frame victim: %d\n", frame_to_swap);

    // Swap out the selected frame
    swap_out(mmu, mmu->pages[frame_to_swap].frame_number);

    // Update the page table entry
    mmu->pages[page_number].frame_number = mmu->pages[frame_to_swap].frame_number;
    mmu->pages[page_number].flags = Valid | Write | Reference;

    // Swap in the required page
    swap_in(mmu, mmu->pages[page_number].frame_number);

    mmu->pointer = (frame_to_swap + 1) % PAGES_NUM;
  }
  else {
    printf("Ram is not full\n");
    swap_in(mmu, mmu->pages[page_number].frame_number);
    mmu->pages[page_number].flags = Valid | Write | Reference;
  }
}

void swap_out(MMU* mmu, int frame_number) {
  printf("Swapping out frame %d to swap file\n", frame_number);
  fseek(mmu->swap_file, frame_number * PAGE_SIZE, SEEK_SET);
  fwrite(&mmu->ram[frame_number * PAGE_SIZE], 1, PAGE_SIZE, mmu->swap_file);
  fflush(mmu->swap_file);
  memset(&mmu->ram[frame_number * PAGE_SIZE], 0, PAGE_SIZE); // Clear the RAM space after swapping out
}

void swap_in(MMU* mmu, int frame_number) {
  printf("Swapping in frame %d from swap file\n", frame_number);
  fseek(mmu->swap_file, frame_number * PAGE_SIZE, SEEK_SET);
  fread(&mmu->ram[frame_number * PAGE_SIZE], 1, PAGE_SIZE, mmu->swap_file);
}

void cleanup_MMU(MMU* mmu) {
    fclose(mmu->swap_file);
    free(mmu->segments);
    free(mmu->ram);
    free(mmu);
}
