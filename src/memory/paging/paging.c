#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

void paging_load_directory(uint32_t* current_directory);
static uint32_t* current_directory = 0;
struct paging_4GB_chunk* paging_new_4GB(uint8_t flags) {
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; ++i) {
        uint32_t* page_table_entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);    
        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; ++j) {
            page_table_entry[j] =   (offset + (j * PAGE_SIZE)) | flags;      
        }
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE);
        directory[i] = (uint32_t)page_table_entry | flags | PAGING_IS_WRITABLE;
    }

    struct paging_4GB_chunk* chunk_4GB = kzalloc(sizeof(struct paging_4GB_chunk));
    chunk_4GB->directory_entry = directory;
    return chunk_4GB;    
}

void paging_switch(uint32_t* directory) {
    paging_load_directory(directory);
    current_directory = directory;
}

uint32_t* paging_4GB_chunk_get_directory(struct paging_4GB_chunk* chunk) {
    return chunk->directory_entry;
}

bool is_paging_aligned(void* addr) {
    return ((uint32_t)addr % PAGE_SIZE) == 0;
}

int paging_get_indexes(void* virtual_addr, uint32_t* directory_index, uint32_t* table_index) {
    int ret = 0;
    if (!is_paging_aligned(virtual_addr)) {
        ret = -EINVARG;
        goto out;
    }
    *directory_index = ((uint32_t)virtual_addr / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE));
    *table_index = ((uint32_t)virtual_addr % (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE) / PAGE_SIZE);

out:
    return ret;
}

int paging_set(uint32_t* directory, void* virt, uint32_t val) {
    if(!is_paging_aligned(virt)) {
        return -EINVARG;
    }

    uint32_t directory_index = 0;
    uint32_t table_index = 0;
    int ret = paging_get_indexes(virt, &directory_index, &table_index);
    if (ret < 0) {
        return ret;
    }

    uint32_t page_directory_entry = directory[directory_index];
    uint32_t* page_table = (uint32_t*)(page_directory_entry & 0xfffff000); //ignoring the last 12 bits here.
    page_table[table_index] = val;
    return ret;
}