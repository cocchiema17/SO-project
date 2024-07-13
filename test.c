#include "fake_mmu.h"
#define NUM_LOGICAL_ADDRESS 3

int main() {
    printf("Starting...\n");
    MMU* mmu = init_MMU(SEGMENTS_NUM, PAGES_NUM, "swapfile.txt");
    printSegmentsTable(mmu);
    printPagesTable(mmu);
    printRam(mmu);

    generateLogicalAddress(mmu);

    // accesso ad una lista di indirizzi logici
    LogicalAddress listLogicalAddress[NUM_LOGICAL_ADDRESS];
    listLogicalAddress[0].segment_id = 0;
    listLogicalAddress[0].page_number = 0;
    listLogicalAddress[0].offset = 0;

    listLogicalAddress[1].segment_id = 3;
    listLogicalAddress[1].page_number = 3;
    listLogicalAddress[1].offset = 15;

    listLogicalAddress[2].segment_id = 3;
    listLogicalAddress[2].page_number = 3;
    listLogicalAddress[2].offset = 0;

    for (int i = 0; i < NUM_LOGICAL_ADDRESS; i++) {
        printf("Accesso all' indirizzo logico: %x%x%x\n",
         listLogicalAddress[i].segment_id, listLogicalAddress[i].page_number, listLogicalAddress[i].offset);
        LinearAddress linear_address = getLinearAddress(mmu, listLogicalAddress[i]);
        printf("Indirizzo lineare: %x%x\n", linear_address.page_number, linear_address.offset);
        PhysicalAddress physical_address = getPhysicalAddress(mmu, linear_address);
        char* c = MMU_readByte(mmu, physical_address);
        printf("Indirizzo fisico: %x -> valore: %c\n", physical_address, *c);
    }

    cleanup_MMU(mmu);
    printf("...finish!\n");
    return EXIT_SUCCESS;
}