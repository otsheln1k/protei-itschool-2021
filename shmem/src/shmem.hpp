#ifndef SHMEM_HPP
#define SHMEM_HPP

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class ShmemBuffer {
public:
    ShmemBuffer() =default;

    ShmemBuffer(int fd, size_t size, bool write = true,
                int flags = MAP_SHARED, off_t offset = 0);

    explicit ShmemBuffer(int fd, bool write = false);

    ShmemBuffer(const ShmemBuffer &) =delete;
    ShmemBuffer &operator=(const ShmemBuffer &) =delete;

    ~ShmemBuffer();

    void swap(ShmemBuffer &p);

    ShmemBuffer(ShmemBuffer &&p);
    ShmemBuffer &operator=(ShmemBuffer &&p);

    // Wait until buffer can be written to.
    bool waitWrite();

    // Mark buffer as closed from write end.
    void sendWriteEof();

    // Mark buffer as closed from write end.
    void sendReadEof();

    // Release buffer with written data.
    void sendDoneWriting();

    // Wait until buffer is written to. Returns false if eof or true if
    // buffer should contain data.
    bool waitRead();

    // Release buffer so that it can be written to.
    void sendDoneReading();

    uint8_t *data();

    const uint8_t *data() const;

    size_t size() const { return m_size; }

    using iterator = uint8_t *;

    iterator begin() { return data(); }
    iterator end() { return data() + m_size; }

    using const_iterator = const uint8_t *;

    const_iterator begin() const { return data(); }
    const_iterator end() const { return data() + m_size; }

private:
    size_t mappingSize() const;

    sem_t *writeSemaphore();

    sem_t *readSemaphore();

    static void semWait(sem_t *sem);
    static int semGetValue(sem_t *sem);

    bool checkEof(sem_t *sem);

    void *m_buf = nullptr;
    size_t m_size = 0;
    bool m_doCleanup = false;

};

#endif
