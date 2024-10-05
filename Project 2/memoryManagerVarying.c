#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define PAGE_SIZE 256
#define PAGES 256
#define PAGE_MASK 0x00FF
#define OFFSET_MASK 0x00FF
#define OFFSET 8
#define FRAME_SIZE 256
#define BUFFER_SIZE 10

typedef struct {
    int pageNumber;
    int frameNumber;
} GeneralTableStruct;

int main(int argc, char** argv) {
    FILE* addressesFile = fopen(argv[1], "r");
    FILE* backingStore = fopen("BACKING_STORE.bin", "rb");
    // FILE* outputFile = fopen("outputPart2.txt", "w");

    int frames = atoi(argv[2]), currentFrame = 0, frameCounter = 0, tlbHits = 0, pageFaults = 0, totalAddresses = 0, fifo = 0;
    int resetIndex; // Reset page table index for LRU algorithm
    int updateIndex = 0;  // holds the index of the page table temp1 to be updated

    GeneralTableStruct* pageTable = (GeneralTableStruct*)malloc(sizeof(GeneralTableStruct) * PAGES);
    GeneralTableStruct* tlb = (GeneralTableStruct*)malloc(sizeof(GeneralTableStruct) * 16);

    char* buffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
    signed char** physicalMemory = (signed char**)malloc(sizeof(signed char*) * frames);
    for (int i = 0; i < frames; i++) {
        physicalMemory[i] = (signed char*)malloc(sizeof(signed char) * FRAME_SIZE);
    }

    // Initialize page table and TLB
    for (int i = 0; i < PAGES; i++) { // Initialize page table with -1 because it is empty
        pageTable[i].pageNumber = -1;
        pageTable[i].frameNumber = -1;
    }

    for (int i = 0; i < 16; i++){ // Initialize TLB with -1 because it is empty
        tlb[i].pageNumber = -1;
        tlb[i].frameNumber = -1;
    }

    while (fgets(buffer, BUFFER_SIZE, addressesFile) != NULL) {
        resetIndex = -1; // Reset page table index
        bool found = false; // Reset found flag

        int logicalAddress = atoi(buffer);                      // Convert address to integer
        int pageNumber = (logicalAddress >> OFFSET) & PAGE_MASK; // Get page number
        int pageOffset = logicalAddress & OFFSET_MASK;           // Get page offset

        for (int i = 0; i < 16; i++) {
            if (tlb[i].pageNumber == pageNumber) {  // TLB hit
                currentFrame = tlb[i].frameNumber; // Get frame number from TLB
                tlbHits++;
                found = true;

                for (int j = 0; j < PAGES; j++) { // Update page table with most recent frame number using LRU algorithm
                    if (pageTable[j].pageNumber == pageNumber) {
                        updateIndex = j;
                        break;
                    }
                }

                currentFrame = (frameCounter + 1) % frames; // Get next frame number
                GeneralTableStruct temp1 = pageTable[updateIndex]; // Get page table temp1

                for (int j = updateIndex; j > 0; j--) { // Update page table with most recent frame number using LRU algorithm implementated as a queue
                    pageTable[j] = pageTable[j - 1];
                }

                pageTable[0] = temp1; // Update page table with most recent frame number using LRU algorithm
                currentFrame = pageTable[0].frameNumber; // Get current frame number

                break;
            }
        }

        if (!found) { // TLB miss
            for (int i = 0; i < PAGES; i++) {
                if (pageTable[i].pageNumber == pageNumber && pageTable[i].frameNumber >= 0) { // Page table hit
                    resetIndex = i;
                    break;
                }
            }

            if (resetIndex != -1) { // Page table hit
                GeneralTableStruct temp1 = pageTable[resetIndex]; // Get page table temp1

                for (int i = resetIndex; i > 0; i--) {
                    GeneralTableStruct temp2 = pageTable[i - 1]; // Swap page table elements
                    pageTable[i] = temp2;
                }

                pageTable[0] = temp1; // Update page table with most recent frame number using LRU algorithm
                currentFrame = pageTable[0].frameNumber; // Get current frame number
            }
            else if (resetIndex == -1) { // Page table miss
                pageFaults++;

                int newFrame = pageTable[frames - 1].frameNumber; // Get new frame number
                for (int i = frames - 1; i > 0; i--) {  // Update page table with most recent frame number using LRU algorithm
                    GeneralTableStruct temp2 = pageTable[i - 1]; // Get page table temp1
                    pageTable[i] = temp2;                 // Update page table
                }

                pageTable[0].pageNumber = pageNumber; // Update page table with new page number
                if (frameCounter > frames - 1) { // Update page table with new frame number
                    pageTable[0].frameNumber = newFrame;     // Update page table with new frame number
                    currentFrame = pageTable[0].frameNumber; // Get current frame number
                }
                else {
                    currentFrame = frameCounter % frames;  // Get current frame number
                    pageTable[0].frameNumber = currentFrame; // Update page table with new frame number
                }
                
                fseek(backingStore, pageNumber * PAGE_SIZE, SEEK_SET); // Read 256 bytes from BACKING_STORE.bin into memory
                fread(physicalMemory[currentFrame], sizeof(signed char), FRAME_SIZE, backingStore); // Read 256 bytes from BACKING_STORE.bin into memory
                frameCounter++;
            }

            tlb[fifo].pageNumber = pageNumber; 
            tlb[fifo++].frameNumber = currentFrame;
            fifo %= 16;
        }

        int physicalAddress = (currentFrame << OFFSET) | pageOffset;
        signed char value = physicalMemory[currentFrame][pageOffset];

        // fprintf(outputFile, "Virtual address: %d Physical address: %d Value: %d\n", logicalAddress, physicalAddress, value);
        printf("Virtual address: %d Physical address: %d Value: %d\n", logicalAddress, physicalAddress, value);
        totalAddresses++; // Increment total addresses
    }

    free(buffer);
    free(pageTable);
    free(tlb);
    for (int i = 0; i < frames; i++) {
        free(physicalMemory[i]);
    }
    free(physicalMemory);

    // Print statistics
    // fprintf(outputFile, "Number of Translated Addresses = %d\n", totalAddresses);
    // fprintf(outputFile, "Page Faults = %d\nPage Fault Rate = %.3f\n", pageFaults, (float)pageFaults / totalAddresses);
    // fprintf(outputFile, "TLB Hits = %d\nTLB Hit Rate = %.3f\n", tlbHits, (float)tlbHits / totalAddresses);

    printf("Number of Translated Addresses = %d\n", totalAddresses);
    printf("Page Faults = %d\nPage Fault Rate = %.3f\n", pageFaults, (float)pageFaults / totalAddresses);
    printf("TLB Hits = %d\nTLB Hit Rate = %.3f\n", tlbHits, (float)tlbHits / totalAddresses);


    fclose(addressesFile);
    fclose(backingStore);
    // fclose(outputFile);

    return 0;
}