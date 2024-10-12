#include<stdio.h>
#include<string.h>
#include<sys/mman.h>

# define HEAP_SIZE 65536
# define BLOCK_SIZE 8

typedef struct Block {
    size_t size; // size of the chunk (data)
    int free;    // check if the block is free
    void *start; // start of chunk
    struct Block *next; // pointer to the next block
} Block;

Block *allocated_head = NULL; // pointer to the head of the allocated blocks
Block *freed_head = NULL; // pointer to the head of the freed blocks
void *heap1 = NULL;

void init_heap() {
    void *mem = mmap(NULL, HEAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    heap1 = mem;
}

int allocated_size  = 0;

void *arena_alloc(size_t size) {
    if (size == 0) {
        printf("Size must be positive\n");
        return NULL;
    }

    size_t padded_size = BLOCK_SIZE - (size % BLOCK_SIZE) + size; // aligning to 8 bytes

    if (allocated_size + padded_size + sizeof(Block) > HEAP_SIZE) {
        printf("Insufficient storage on heap1\n");
        return NULL;
    }

    // Check if there is a freed block that we can reuse
    Block *prev = NULL;
    Block *current = freed_head;
    while (current != NULL) {
        if (current->size >= padded_size) {
            current->free = 0; // Mark as allocated

            // Remove from freed list
            if (prev == NULL) {
                freed_head = current->next;
            } else {
                prev->next = current->next;
            }

            // Add to allocated list
            current->next = allocated_head;
            allocated_head = current;

            return current->start;
        }
        prev = current;
        current = current->next;
    }

    // No suitable freed block found, allocate new memory
    void *allocated_mem = heap1 + allocated_size; // Pointer to the start of the block + header
    Block *newBlock = (Block*) allocated_mem;

    newBlock->size = padded_size; // size of the payload
    newBlock->free = 0;
    newBlock->start = (void *) allocated_mem + sizeof(Block);

    // Add new block to allocated list
    newBlock->next = allocated_head;
    allocated_head = newBlock;

    allocated_size += padded_size + sizeof(Block);

    return newBlock->start; // returning the pointer to the start of the payload
}

// Function to free memory
void arena_destroy(void *ptr) {
    if (ptr == NULL) {
        printf("NULL pointer cannot be freed\n");
        return;
    }

    // Find the block in allocated list
    Block *prev = NULL;
    Block *current = allocated_head;
    while (current != NULL && current->start != ptr) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        return;
    }

    // Mark block as free
    current->free = 1;

    // Remove from allocated list
    if (prev == NULL) {
        allocated_head = current->next;
    } else {
        prev->next = current->next;
    }

    // Add block to freed list
    current->next = freed_head;
    freed_head = current;
    allocated_size -= (current->size + sizeof(Block));
}

// Function to reallocate memory
void *arena_reset(void *ptr, size_t size) {
    if (ptr == NULL) {
        return arena_alloc(size);
    }

    if (size == 0) {
        arena_destroy(ptr);
        return NULL;
    }

    Block *block = (Block*)ptr - 1;
    size_t current_size = block->size;

    if (size <= current_size) {
        return ptr; // if new size is smaller or equal, we don't need to reallocate
    }

    // Allocate new memory and copy old data
    void *new_ptr = arena_alloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    // Copy old data to the new memory
    char *old_data = (char*)ptr;
    char *new_data = (char*)new_ptr;
    for (size_t i = 0; i < current_size; i++) {
        new_data[i] = old_data[i];
    }
    arena_destroy(ptr);
    return new_ptr;
}

void inspect(){
    printf("-----------------------------------\n");
    printf("Current Memory in allocated: \n");
    Block *current = allocated_head;
    while (current != NULL) {
        printf("\t%p %ld\n", current->start, current->size);
        current = current->next;
    }

    printf("Memory in freed: \n");
    current = freed_head;
    while (current != NULL) {
        printf("\t%p %ld\n", current->start, current->size);
        current = current->next;
    }

    printf("Total Size: %ld\n", allocated_size);
    printf("-----------------------------------\n");
}

void main() {
    init_heap();
    void *ptr1 = arena_alloc(39);
    inspect();
    strcpy(ptr1,"adi");
    printf("\n");

    void *ptr2 = arena_alloc(20);
    inspect();
    printf("\n");
    strcpy(ptr2,"adithya");

    printf("\n");

    arena_destroy(ptr1);
    inspect();
    printf("\n");

    ptr2 = arena_reset(ptr2, 70);
    inspect();
}
