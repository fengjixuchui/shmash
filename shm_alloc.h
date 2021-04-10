#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#define SHM_NAME "/shm_heap"
#define HEAPSIZE 65536

typedef struct shm_chunk
{
    int free;
    size_t size;
    struct shm_chunk *next;
    struct shm_chunk *prev;
} shm_chunk;

shm_chunk *chunk_start;
int shmfd;

void *shm_alloc(size_t size)
{
    shm_chunk *tmp = chunk_start;
    while (1)
    {
        if (tmp->free && size <= tmp->size)
        {
            shm_chunk tmp_t = *tmp;
            tmp->free = 1;
            tmp->size = size;
            tmp->next = (shm_chunk *)((uintptr_t) tmp->next - (uintptr_t) size);

            tmp->next->free = 0;
            tmp->next->size = tmp_t.size - size;
            tmp->next->next = tmp_t.next;
            tmp->next->prev = tmp;

            return ((shm_chunk *) tmp) + 1;
        }

        tmp = tmp->next;
        if (!tmp)
        {
            return NULL;
        }
    }
}

void shm_free(void *ptr)
{
    shm_chunk *to_free = ((shm_chunk *) ptr) - 1;
    int nto_free = to_free->size;

    shm_chunk *tmp = to_free->prev;
    while (tmp)
    {
        nto_free += tmp->size;
        if (tmp->prev->free)
        {
            tmp = tmp->prev;
        }
    }
    shm_chunk *start = tmp;
    if (!start)
    {
        start = to_free;
    }

    tmp = to_free->next;
    while (tmp)
    {
        nto_free += tmp->size;
        if (tmp->next->free)
        {
            tmp = tmp->next;
        }
    }

    start->free = 1;
    start->size = nto_free;
    start->next = tmp;
    memset((start + 1), 0, nto_free);
}

void shm_init()
{
    int flags = MAP_SHARED;
    shmfd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shmfd == -1)
    {
        shmfd = shm_open(SHM_NAME, O_RDWR, 0666);
        flags = flags | MAP_FIXED;
    }
    chunk_start = mmap(0, HEAPSIZE, PROT_READ | PROT_WRITE, flags, shmfd, 0);
}

void shm_detach()
{
    munmap((void *) chunk_start, HEAPSIZE);
}

void shm_close()
{
    close(shmfd);
}
