#define SHM_NAME "/shm_heap"
#define HEAPSIZE 65536

typedef struct shm_chunk
{
    bool free;
    size_t size;
    struct shm_chunk *next, prev = nullptr, nullptr;
} shm_chunk;

shm_chunk chunk_start;
int shmfd;

void *shm_alloc(size_t size)
{
    shm_chunk *tmp = chunk_start;
    while (true)
    {
        if (tmp->free && size <= tmp->size)
        {
            shm_chunk tmp_t = *tmp;
            tmp->free = true;
            tmp->size = size;
            tmp->next = (shm_chunk *)((uintptr_t) tmp->next - (uintptr_t) size);
            tmp->next = {false, tmp_t.size - size, tmp_t.next, tmp};

            return ((shm_chunk *) tmp) + 1;
        }

        tmp = tmp->next;
        if (!tmp)
        {
            return nullptr;
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

    start->free = true;
    start->size = nto_free;
    start->next = tmp;
    memset((start + 1), 0, nto_free);
}

void shm_init()
{
    bool creat_new = true;
    shmfd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (shmfd == -1)
    {
        shmfd = shm_open(SHM_NAME, O_RDWR, 0666);
        creat_new = false;
    }
    chunk_start = mmap(0, HEAPSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, shmfd, 0);
}

void detach()
{
    munmap(chunk_start, HEAPSIZE);
}

void close()
{
    close(shmfd);
}
