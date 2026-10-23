/*
 * CS 3733 Project: Virtual Memory Manager
 *
 * This program simulates virtual memory address translation.
 * It reads logical addresses from addresses.txt, translates them into
 * physical addresses, handles page faults using BACKING_STORE.bin,
 * and uses LRU page replacement when physical memory is full.
 */

#include <stdio.h>

#define PAGE_SIZE 256
#define NUM_PAGES 256
#define NUM_FRAMES 128

#define ADDRESS_FILE "addresses.txt"
#define BACKING_STORE_FILE "BACKING_STORE.bin"
#define OUTPUT_FILE "output.txt"
#define SNAPSHOT_500_FILE "snapshot_500.txt"
#define SNAPSHOT_631_FILE "snapshot_631.txt"

/*
 * Initializes the page table, valid bits, and LRU tracking array.
 * A page table value of -1 means the page is not currently mapped
 * to a physical frame.
 */
void initialize_memory_data(int page_table[], int valid[], int last_used[]) {
    for (int i = 0; i < NUM_PAGES; i++) {
        page_table[i] = -1;
        valid[i] = 0;
        last_used[i] = -1;
    }
}

/*
 * Writes the current page table to a snapshot file.
 * Each row shows the page number, whether it is valid,
 * and the frame number if that page is currently in memory.
 */
void write_page_table_snapshot(const char *filename,
                               int page_table[],
                               int valid[],
                               int request_number) {
    FILE *snapshot_file = fopen(filename, "w");

    if (snapshot_file == NULL) {
        printf("Error: Could not create %s\n", filename);
        return;
    }

    fprintf(snapshot_file, "Page Table Snapshot After Request %d\n", request_number);
    fprintf(snapshot_file, "Page Number\tValid\tFrame Number\n");
    fprintf(snapshot_file, "--------------------------------------\n");

    for (int page = 0; page < NUM_PAGES; page++) {
        if (valid[page] == 1) {
            fprintf(snapshot_file, "%d\t\t%d\t%d\n",
                    page, valid[page], page_table[page]);
        } else {
            fprintf(snapshot_file, "%d\t\t%d\t-\n",
                    page, valid[page]);
        }
    }

    fclose(snapshot_file);
}

/*
 * Finds the least recently used page currently in physical memory.
 * The page with the smallest last_used value is selected for replacement.
 */
int find_lru_page(int valid[], int last_used[]) {
    int lru_page = -1;
    int smallest_time = 1000000;

    for (int page = 0; page < NUM_PAGES; page++) {
        if (valid[page] == 1 && last_used[page] < smallest_time) {
            smallest_time = last_used[page];
            lru_page = page;
        }
    }

    return lru_page;
}

/*
 * Loads one page from the backing store into the selected physical frame.
 * The location in BACKING_STORE.bin is based on the page number.
 */
int load_page_from_backing_store(FILE *backing_store,
                                 signed char physical_memory[NUM_FRAMES][PAGE_SIZE],
                                 int page_number,
                                 int frame_number) {
    int seek_result;
    size_t bytes_read;

    seek_result = fseek(backing_store, page_number * PAGE_SIZE, SEEK_SET);

    if (seek_result != 0) {
        printf("Error: Could not seek to page %d in backing store.\n", page_number);
        return 0;
    }

    bytes_read = fread(physical_memory[frame_number],
                       sizeof(signed char),
                       PAGE_SIZE,
                       backing_store);

    if (bytes_read != PAGE_SIZE) {
        printf("Error: Could not read full page %d from backing store.\n", page_number);
        return 0;
    }

    return 1;
}

/*
 * Handles a page fault.
 *
 * If there is still a free frame, the page is loaded into that frame.
 * If physical memory is full, the least recently used page is removed
 * and its frame is reused for the new page.
 */
int handle_page_fault(FILE *backing_store,
                      signed char physical_memory[NUM_FRAMES][PAGE_SIZE],
                      int page_table[],
                      int valid[],
                      int last_used[],
                      int page_number,
                      int *next_free_frame) {
    int frame_number;

    if (*next_free_frame < NUM_FRAMES) {
        frame_number = *next_free_frame;
        (*next_free_frame)++;
    } else {
        int page_to_replace = find_lru_page(valid, last_used);

        frame_number = page_table[page_to_replace];

        valid[page_to_replace] = 0;
        page_table[page_to_replace] = -1;
        last_used[page_to_replace] = -1;
    }

    if (!load_page_from_backing_store(backing_store,
                                      physical_memory,
                                      page_number,
                                      frame_number)) {
        return -1;
    }

    page_table[page_number] = frame_number;
    valid[page_number] = 1;

    return frame_number;
}

/*
 * Prints one translated address line to both the output file
 * and the console. The format matches the project requirements.
 */
void write_translation(FILE *output_file,
                       int logical_address,
                       int physical_address,
                       signed char value) {
    fprintf(output_file,
            "Logical Address: %d Physical Address: %d Value: %d\n",
            logical_address, physical_address, value);

    printf("Logical Address: %d Physical Address: %d Value: %d\n",
           logical_address, physical_address, value);
}

int main() {
    FILE *address_file;
    FILE *backing_store;
    FILE *output_file;

    int page_table[NUM_PAGES];
    int valid[NUM_PAGES];
    int last_used[NUM_PAGES];

    signed char physical_memory[NUM_FRAMES][PAGE_SIZE];

    int logical_address;
    int page_number;
    int offset;
    int frame_number;
    int physical_address;
    signed char value;

    int next_free_frame = 0;
    int total_addresses = 0;
    int page_faults = 0;
    int time_counter = 0;

    initialize_memory_data(page_table, valid, last_used);

    address_file = fopen(ADDRESS_FILE, "r");
    if (address_file == NULL) {
        printf("Error: Could not open %s\n", ADDRESS_FILE);
        return 1;
    }

    backing_store = fopen(BACKING_STORE_FILE, "rb");
    if (backing_store == NULL) {
        printf("Error: Could not open %s\n", BACKING_STORE_FILE);
        fclose(address_file);
        return 1;
    }

    output_file = fopen(OUTPUT_FILE, "w");
    if (output_file == NULL) {
        printf("Error: Could not create %s\n", OUTPUT_FILE);
        fclose(address_file);
        fclose(backing_store);
        return 1;
    }

    /*
     * Main address-translation loop.
     * Each logical address is split into a page number and offset.
     */
    while (fscanf(address_file, "%d", &logical_address) == 1) {
        total_addresses++;

        page_number = logical_address / PAGE_SIZE;
        offset = logical_address % PAGE_SIZE;

        /*
         * If the page is not valid, it is not currently in physical memory.
         * This causes a page fault, so the page must be loaded from the
         * backing store.
         */
        if (valid[page_number] == 0) {
            page_faults++;

            frame_number = handle_page_fault(backing_store,
                                             physical_memory,
                                             page_table,
                                             valid,
                                             last_used,
                                             page_number,
                                             &next_free_frame);

            if (frame_number == -1) {
                printf("Error: Page fault handling failed.\n");
                fclose(address_file);
                fclose(backing_store);
                fclose(output_file);
                return 1;
            }
        } else {
            /*
             * If the page is valid, it is already in memory.
             * The frame number can be taken directly from the page table.
             */
            frame_number = page_table[page_number];
        }

        /*
         * Update LRU information after the page is used.
         * Larger values mean the page was used more recently.
         */
        last_used[page_number] = time_counter;
        time_counter++;

        physical_address = frame_number * PAGE_SIZE + offset;
        value = physical_memory[frame_number][offset];

        write_translation(output_file, logical_address, physical_address, value);

        /*
         * The project requires page table snapshots after the 500th
         * and 631st address requests.
         */
        if (total_addresses == 500) {
            write_page_table_snapshot(SNAPSHOT_500_FILE,
                                      page_table,
                                      valid,
                                      500);
        }

        if (total_addresses == 631) {
            write_page_table_snapshot(SNAPSHOT_631_FILE,
                                      page_table,
                                      valid,
                                      631);
        }
    }

    fprintf(output_file, "\nTotal addresses processed: %d\n", total_addresses);
    fprintf(output_file, "Page faults: %d\n", page_faults);
    fprintf(output_file, "Page fault rate: %.2f%%\n",
            ((double) page_faults / total_addresses) * 100);

    printf("\nTotal addresses processed: %d\n", total_addresses);
    printf("Page faults: %d\n", page_faults);
    printf("Page fault rate: %.2f%%\n",
           ((double) page_faults / total_addresses) * 100);

    fclose(address_file);
    fclose(backing_store);
    fclose(output_file);

    return 0;
}