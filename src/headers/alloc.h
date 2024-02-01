
#ifndef _H_ALLOC
#define _H_ALLOC

typedef struct Allocator Allocator;
typedef struct AllocatorBlock AllocatorBlock;

struct Allocator {
    AllocatorBlock *first_block_private;
    AllocatorBlock *last_block_private;
    AllocatorBlock *first_block;
    AllocatorBlock *last_block;
};
struct AllocatorBlock {
    AllocatorBlock *next;
    size_t size;
    size_t space_left;
    void *start_adr;
    void *current_adr;
};

Allocator *alc_make();
void alc_wipe(Allocator *alc);
void alc_delete(Allocator *alc);
AllocatorBlock *alc_block_make(size_t size);
void *al(Allocator *alc, size_t size);
AllocatorBlock *al_private(Allocator *alc, size_t size);
void free_block(AllocatorBlock *block);
char *dups(Allocator *alc, char *str);

#endif
