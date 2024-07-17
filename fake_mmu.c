#include "fake_mmu.h"

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
  mmu->pageFault = 0;
  mmu->busyFramesInRam = 0;

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
  mmu->busyFramesInRam += unswappable_pages;

  // Store the page table at the beginning of the RAM
  memcpy(mmu->ram, mmu->pages, sizeof(PageEntry) * num_pages);
  mmu->pages = (PageEntry*) mmu->ram; 

  mmu->swap_file = fopen(swap_file, "r+");
  if (!mmu->swap_file) {
    printf("Error opening swap file");
    exit(EXIT_FAILURE);
  }
  
  return mmu;
}

// to be removed
// void printPagesTable(MMU* mmu) {
//   PageEntry* pointer = mmu->pages;
//   for (int i = 0; i < mmu->num_pages; i++) {
//     printf("Page %d: frame_number = %d, flags = %d\n",
//       i, pointer->frame_number, pointer->flags);
//     pointer++;
//   }
// }

// void printPagesTableInRam(MMU* mmu) {
//   PageEntry* pointer = mmu->pagesInRam;
//   int frameInRam =  MAX_MEMORY / PAGE_SIZE;
//   for (int i = 0; i < frameInRam; i++) {
//     printf("Page %d: frame_number = %d, flags = %d\n",
//       i, pointer->frame_number, pointer->flags);
//     pointer++;
//   }
// }

// to be removed
// void printRam(MMU* mmu) {
//   char* pointer = mmu->ram;
//   for (int i = 0; i < MAX_MEMORY; i++) {
//     printf("Ram [%d = %x]: %x = %c\n", i, i, *pointer, *pointer);
//     pointer++;
//   }
// }

int isRamFull(MMU* mmu) {
  return (mmu->busyFramesInRam >= MAX_MEMORY / PAGE_SIZE);
}

char* MMU_readByte(MMU* mmu, LinearAddress pos) {
  assert(pos.page_number >= 0 && pos.page_number < PAGES_NUM && "Invalid linear address");
  printf("Reading at linear address: 0x%x%x\n", pos.page_number, pos.offset);
  PageEntry page_entry = mmu->pages[pos.page_number];
  if (!(page_entry.flags & Valid)) {
    MMU_exception(mmu, pos);
    page_entry = mmu->pages[pos.page_number];
  }

  page_entry.flags |= Read | Reference;
  mmu->pages[pos.page_number] = page_entry;

  uint32_t frame_number = page_entry.frame_number;
  PhysicalAddress physicalAddress = (frame_number << FRAME_NBITS) | pos.offset;
  char* pointer = mmu->ram;
  pointer += physicalAddress;
  return pointer;
}

void MMU_writeByte(MMU* mmu, LinearAddress pos, char c) {
  assert(pos.page_number >= 0 && pos.page_number < PAGES_NUM && "Invalid linear address");
  printf("Writing %c at linear address: 0x%x%x\n", c, pos.page_number, pos.offset);
  PageEntry page_entry = mmu->pages[pos.page_number];
  if (!(page_entry.flags & Valid)) {
    MMU_exception(mmu, pos);
    page_entry = mmu->pages[pos.page_number];
  }
  page_entry.flags |= Write | Reference;
  mmu->pages[pos.page_number] = page_entry;

  uint32_t frame_number = page_entry.frame_number;
  PhysicalAddress physicalAddress = (frame_number << FRAME_NBITS) | pos.offset;
  char* pointer = mmu->ram;
  pointer += physicalAddress;
  *pointer = c;
}

void MMU_exception(MMU* mmu, LinearAddress pos) {
  printf("Page fault at page number 0x%x\n", pos.page_number);
  mmu->pageFault++;
  // The page table in RAM cannot be accessed either for reading or writing
  assert(!(mmu->pages[pos.page_number].flags & Unswappable) && "Page table in RAM cannot be accessed for reading or writing");
  if(isRamFull(mmu)) {
    printf("Ram is full\n");
    int frame_to_swap = 0; 
    int frameInRam =  MAX_MEMORY / PAGE_SIZE;
    // second chance algorithm
    while (mmu->pagesInRam[frame_to_swap].flags & Unswappable || (mmu->pagesInRam[frame_to_swap].flags & Reference)) {
      mmu->pagesInRam[frame_to_swap].flags ^= Reference;
      frame_to_swap = (frame_to_swap + 1) % frameInRam;
    }

    // Swap out the selected frame
    swap_out(mmu, mmu->pagesInRam[frame_to_swap].frame_number, frame_to_swap);

    // Update the page table entry
    mmu->pagesInRam[frame_to_swap].frame_number = frame_to_swap;
    mmu->pagesInRam[frame_to_swap].flags = Valid | Reference;
    mmu->pages[pos.page_number] = mmu->pagesInRam[frame_to_swap];
    mmu->pages[frame_to_swap].frame_number = 0;
    mmu->pages[frame_to_swap].flags = 0;

    // Swap in the required page
    swap_in(mmu, pos.page_number, frame_to_swap);

    frame_to_swap = (frame_to_swap + 1) % frameInRam;
  }
  else {
    printf("Ram is not full\n");
    swap_in(mmu, pos.page_number, 0);
    // Update the page table entry
    mmu->pagesInRam[mmu->busyFramesInRam - 1].frame_number = pos.page_number;
    mmu->pagesInRam[mmu->busyFramesInRam - 1].flags = Valid | Reference;
    mmu->pages[pos.page_number] = mmu->pagesInRam[mmu->busyFramesInRam - 1];
  }
}

void swap_out(MMU* mmu, int page_number, int frame_to_swap) {
  printf("Swapping out page %d to swap file\n", page_number);
  fseek(mmu->swap_file, page_number * PAGE_SIZE, SEEK_SET);
  fwrite(&mmu->ram[frame_to_swap * PAGE_SIZE], 1, PAGE_SIZE, mmu->swap_file);
  fflush(mmu->swap_file);
  printf("FrameRam [0x%x] => File [0x%x]\n", frame_to_swap, page_number);
  memset(&mmu->ram[frame_to_swap * PAGE_SIZE], 0, PAGE_SIZE); // Clear the RAM space after swapping out
}

void swap_in(MMU* mmu, int page_number, int frame_to_swap) {
  printf("Swapping in page %d from swap file\n", page_number);
  fseek(mmu->swap_file, page_number * PAGE_SIZE, SEEK_SET);
  if (!isRamFull(mmu)) {
    fread(&mmu->ram[mmu->busyFramesInRam * PAGE_SIZE], 1, PAGE_SIZE, mmu->swap_file);
    printf("FrameRam [0x%x] <= File [0x%x]\n", mmu->busyFramesInRam, page_number);
    mmu->busyFramesInRam++;
  }
  else {
    fread(&mmu->ram[frame_to_swap * PAGE_SIZE], 1, PAGE_SIZE, mmu->swap_file);
    printf("FrameRam [0x%x] <= File [0x%x]\n", frame_to_swap, page_number);
  }
  
}

void cleanup_MMU(MMU* mmu) {
  fclose(mmu->swap_file);
  free(mmu->ram);
  free(mmu->pagesInRam);
  free(mmu);
}
