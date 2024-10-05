#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGES 256 // 28 entries in the page table
#define PAGE_MASK 0x00FF // 8-bit page number
#define PAGE_SIZE 256 // 28 bytes
#define OFFSET 8 // 8-bit page offset
#define OFFSET_MASK 0x00FF // 8-bit page offset
#define FRAME_SIZE 256 // 2^8 bytes
#define FRAMES 256 // 256 frames
#define BUFFER_SIZE 10 // Buffer size for reading logical addresses from file


int main (int argc, char** argv) {
    FILE* addressesFile = fopen(argv[1], "r");
    FILE* backingStore = fopen("BACKING_STORE.bin", "rb");
    // FILE* outputFile = fopen("outputPart1.txt", "w");

    int* pageTable = (int*)malloc(sizeof(int) * PAGES);
    int** tlb = (int**)malloc(sizeof(int*) * 16);
    for (int i = 0; i < 16; i++) {
        tlb[i] = (int*)malloc(sizeof(int) * 2);
    }

    int totalAddresses = 0, pageFault = 0, tlbHits = 0;

    char* buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE); 
    signed char** physcialMemory = (signed char**)malloc(sizeof(signed char*) * FRAMES);
    for (int i = 0; i < FRAMES; i++) {
        physcialMemory[i] = (signed char*)malloc(sizeof(signed char) * FRAME_SIZE);
    }
    unsigned char freePage = 0; // Next free page in physcialMemory

    int fifo = 0; // First-in, first-out counter for page replacement

    for (int i = 0; i < PAGES; i++) {
        pageTable[i] = -1; // Initialize page table with -1 because it is empty
    }

    for (int i = 0; i < 16; i++) {
        tlb[i][0] = -1; // Initialize TLB with -1 because it is empty
        tlb[i][1] = -1;
    }

    while (fgets(buffer, BUFFER_SIZE, addressesFile) != NULL) {
        totalAddresses++; // Increment total addresses
        int logicalAddress = atoi(buffer); // Convert address to integer
        int pageNumber = (logicalAddress >> OFFSET) & PAGE_MASK; // Get page number
        int pageOffset = logicalAddress & OFFSET_MASK; // Get page offset

        int frameNumber = -1;
        int tlbIndex = -1;

        // Check TLB for page number
        for (int i = 0; i < 16; i++) {
            if (tlb[i][0] == pageNumber) {
                frameNumber = tlb[i][1];
                tlbIndex = i;
                tlbHits++;
                break;
            }
        }

        // TLB miss
        if (frameNumber == -1) {
            // Check page table for page number
            frameNumber = pageTable[pageNumber];

            // Page fault
            if (frameNumber == -1) {
                pageFault++;
                // Read 256 bytes from BACKING_STORE.bin into physcialMemory
                fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET);
                fread(physcialMemory[freePage], sizeof(signed char), FRAME_SIZE, backingStore);
                frameNumber = freePage;
                pageTable[pageNumber] = frameNumber;
                freePage = (freePage + 1) % FRAMES;
            }

            // Update TLB
            tlb[fifo][0] = pageNumber;
            tlb[fifo][1] = frameNumber;
            fifo = (fifo + 1) % 16;
        }

        int physicalAddress = (frameNumber << OFFSET) | pageOffset;
        signed char value = physcialMemory[frameNumber][pageOffset];
        // fprintf(outputFile, "Virtual address: %d Physical address: %d Value: %d\n", logicalAddress, physicalAddress, value);
        printf("Virtual address: %d Physical address: %d Value: %d\n", logicalAddress, physicalAddress, value);
    }

    free(buffer);
    for (int i = 0; i < FRAMES; i++) {
        free(physcialMemory[i]);
    }
    free(physcialMemory);
    free(pageTable);
    for (int i = 0; i < 16; i++) {
        free(tlb[i]);
    }
    free(tlb);

    // fprintf(outputFile, "Number of Translated Addresses = %d\n", totalAddresses);
    // fprintf(outputFile, "Page Faults = %d\nPage Fault Rate = %.3f\n", pageFault, (float)pageFault / totalAddresses);
    // fprintf(outputFile, "TLB Hits = %d\nTLB Hit Rate = %.3f\n", tlbHits, (float)tlbHits / totalAddresses);

    printf("Number of Translated Addresses = %d\n", totalAddresses);
    printf("Page Faults = %d\nPage Fault Rate = %.3f\n", pageFault, (float)pageFault / totalAddresses);
    printf("TLB Hits = %d\nTLB Hit Rate = %.3f\n", tlbHits, (float)tlbHits / totalAddresses);


    fclose(addressesFile);
    fclose(backingStore);
    // fclose(outputFile);

    return 0;
}