#include "fake_mmu.h"

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

MMU* init_MMU(uint32_t num_pages, const char* swap_file) {
  MMU* mmu = (MMU*)malloc(sizeof(MMU));
  assert(mmu != NULL && "Error mmu malloc");
  mmu->pages = (PageEntry*)malloc(sizeof(PageEntry) * num_pages);
  assert(mmu->pages != NULL && "Error pages malloc");
  mmu->num_pages = num_pages;
  mmu->ram = (char*)malloc(MAX_MEMORY);
  assert(mmu->ram != NULL && "Error ram malloc");
  // frameInRam = MAX_MEMORY / PAGE_SIZE = 1 MB / 4 KB = 256 frames
  int frameInRam =  MAX_MEMORY / PAGE_SIZE;
  printf("Frames in ram = %d\n", frameInRam);
  mmu->pagesInRam = (PageEntry*)malloc(sizeof(PageEntry) * frameInRam);
  assert(mmu->pagesInRam != NULL && "Error pagesInRam malloc");
  mmu->used_memory = 0;
  mmu->pageFault = 0;

  // initialization of pages
  // size of Page table = sizeof(PageEntry) * num_pages = 4 B * 4096 = 16384 B = 16 KB => the first occupied frames are size of Page table / PAGE_SIZE = 16 KB / 4 KB = 4 pages
  int unswappable_pages = sizeof(PageEntry) * num_pages / PAGE_SIZE;
  printf("Unswappable pages = %d\n", unswappable_pages);
  for(int i = 0; i < num_pages; i++) {
    if (i < unswappable_pages) {
      mmu->pages[i].frame_number = i;
      mmu->pages[i].flags = Unswappable;
      // initialization of pagesInRam
      mmu->pagesInRam[i] = mmu->pages[i];
    }
    else
      mmu->pages[i].flags = 0;
  }

  // Store the page table at the beginning of the RAM
  memcpy(mmu->ram, mmu->pages, sizeof(PageEntry) * num_pages);
  mmu->used_memory = sizeof(PageEntry) * num_pages;
  mmu->pages = (PageEntry*) mmu->ram; 

  mmu->swap_file = fopen(swap_file, "r+");
  if (!mmu->swap_file) {
    printf("Error opening swap file");
    exit(EXIT_FAILURE);
  }

  mmu->pointer = 0;
  
  return mmu;
}

void printPagesTable(MMU* mmu) {
  PageEntry* pointer = mmu->pages;
  for (int i = 0; i < mmu->num_pages; i++) {
    printf("Page %d: frame_number = %d, flags = %d\n",
      i, pointer->frame_number, pointer->flags);
    pointer++;
  }
}

void printPagesTableInRam(MMU* mmu) {
  PageEntry* pointer = mmu->pagesInRam;
  int frameInRam =  MAX_MEMORY / PAGE_SIZE;
  for (int i = 0; i < frameInRam; i++) {
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
  return (mmu->used_memory == MAX_MEMORY);
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

    // Swap out the selected frame
    swap_out(mmu, mmu->pages[frame_to_swap].frame_number);

    // Update the page table entry
    mmu->pages[page_number].frame_number = mmu->pages[frame_to_swap].frame_number;
    mmu->pages[page_number].flags = Valid | Reference;

    // Swap in the required page
    swap_in(mmu, mmu->pages[page_number].frame_number);

    mmu->pointer = (frame_to_swap + 1) % PAGES_NUM;
  }
  else {
    printf("Ram is not full\n");
    swap_in(mmu, mmu->pages[page_number].frame_number);
    mmu->pages[page_number].flags = Valid | Reference;
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
  mmu->used_memory += PAGE_SIZE;
}

void cleanup_MMU(MMU* mmu) {
  fclose(mmu->swap_file);
  free(mmu->ram);
  free(mmu->pagesInRam);
  free(mmu);
}
