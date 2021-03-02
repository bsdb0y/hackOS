#include "heap.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

static int heap_validate_table(void* ptr, void* end, struct heap_table* heap) {
    int res = 0;
    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE;
    if (heap->total != total_blocks) {
        res = -EINVARG;
        goto out;
    }

out:
    return res;
}

static bool heap_check_alignment(void* ptr) {
    return (((unsigned int)ptr % PEACHOS_HEAP_BLOCK_SIZE) == 0);
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* htable) {
    int res = 0;

    if (!heap_check_alignment(ptr) || !heap_check_alignment(end)) {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(heap));
    heap->saddr = ptr;
    heap->table = htable;

    res = heap_validate_table(ptr, end, htable);
    if (res < 0) {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * htable->total;
    memset(htable->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);
    
out:
    return res;
}

static uint32_t find_enough_heap_addr(size_t val) {
    size_t aligned = val % PEACHOS_HEAP_BLOCK_SIZE;
    if (!aligned) {
        return val;
    }

    val -= aligned;
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val; 
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry) {
    return entry & 0x0f; // last four bits
}

int heap_get_start_block(struct heap* heap, size_t blocks_aligned_addr) {
    struct heap_table* table = heap->table;
    int block_current = 0;
    int block_start = -1;

    for (size_t i = 0; i < table->total; ++i) {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE) {
            block_current = 0;
            block_start = -1;
            continue;
        }

        if (block_start == -1) {
            block_start = i;
        }
        block_current++;
        if (block_current == blocks_aligned_addr) {
            break;
        }

        if (block_start == -1) {
            return -ENOMEM;
        }
    }
    return block_start; 
}

int heap_address_to_block(struct heap* heap, void* addr) {
    return (int)(addr - heap->saddr) / PEACHOS_HEAP_BLOCK_SIZE;
}

void* heap_block_to_address(struct heap* heap, int s_block) {
    return heap->saddr + (s_block * PEACHOS_HEAP_BLOCK_SIZE);
}

void heap_mark_blocks_free(struct heap* heap, int s_block) {
    struct heap_table* table = heap->table;
    for (int block = s_block; block < (int)table->total; ++block) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[block];
        table->entries[block] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if(!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

void heap_mark_blocks_taken(struct heap* heap, int start_block, size_t blocks_for_aligned_addr) {
    size_t end_block = (start_block + blocks_for_aligned_addr) - 1;
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if (blocks_for_aligned_addr > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }
    for (int i = start_block; i <= end_block; ++i) {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i != (end_block - 1)) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void* heap_allocate_blocks(struct heap* heap, size_t blocks_for_aligned_addr) {
    void* address = NULL;

    int start_block = heap_get_start_block(heap, blocks_for_aligned_addr);
    if (start_block < 0) {
        goto out;
    }
    address = heap_block_to_address(heap, start_block);
    // Marks the blocks as taken
    heap_mark_blocks_taken(heap, start_block, blocks_for_aligned_addr);
out:
    return address;
}

void* heap_malloc(struct heap *heap, size_t size) {
    size_t max_aligned_addr = find_enough_heap_addr(size);
    size_t blockes_as_per_max_aligned_addr = max_aligned_addr / PEACHOS_HEAP_BLOCK_SIZE;
    return heap_allocate_blocks(heap, blockes_as_per_max_aligned_addr);
}

void heap_free(struct heap* heap, void* ptr) {
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}