#include <system_error>

#include <errno.h>

#include "shmem.hpp"

static size_t
fdsize(int fd)
{
    struct stat st;
    if (fstat(fd, &st) < 0) {
        throw std::system_error(errno, std::system_category());
    }

    return static_cast<size_t>(st.st_size);
}

static std::system_error
errno_exception()
{
    return std::system_error(errno, std::system_category());
}

void
ShmemBuffer::semWait(sem_t *sem)
{
    if (sem_wait(sem) < 0) {
        throw errno_exception();
    }
}

int
ShmemBuffer::semGetValue(sem_t *sem)
{
    int val;
    if (sem_getvalue(sem, &val) < 0) {
        throw errno_exception();
    }
    return val;
}

ShmemBuffer::ShmemBuffer(int fd, size_t size, bool write,
                         int flags, off_t offset)
    :m_size{size}
{
    if (write) {
        if (ftruncate(fd, mappingSize()) < 0) {
            throw errno_exception();
        }
    }

    m_buf = mmap(nullptr, mappingSize(), PROT_READ | PROT_WRITE,
                 flags, fd, offset);
    if (m_buf == MAP_FAILED) {
        throw errno_exception();
    }

    if (write) {
        sem_init(writeSemaphore(), 1, 1);
        sem_init(readSemaphore(), 1, 0);
    }
}

ShmemBuffer::ShmemBuffer(int fd, bool write)
    :ShmemBuffer{fd, fdsize(fd) - 2*sizeof(sem_t), write}
{}

ShmemBuffer::~ShmemBuffer()
{
    if (m_buf != nullptr) {
        if (m_doCleanup) {
            sem_destroy(writeSemaphore());
            sem_destroy(readSemaphore());
        }

        munmap(m_buf, mappingSize());
    }
}

void
ShmemBuffer::swap(ShmemBuffer &p)
{
    std::swap(m_buf, p.m_buf);
    std::swap(m_size, p.m_size);
    std::swap(m_doCleanup, m_doCleanup);
}

ShmemBuffer::ShmemBuffer(ShmemBuffer &&p)
{
    *this = std::move(p);
}

ShmemBuffer &
ShmemBuffer::operator=(ShmemBuffer &&p)
{
    swap(p);
    ShmemBuffer b;
    p.swap(b);
    return *this;
}

bool
ShmemBuffer::checkEof(sem_t *sem)
{
    bool eof = semGetValue(sem) > 0;
    if (eof) {
        m_doCleanup = true;
    }
    return !eof;
}

bool
ShmemBuffer::waitWrite()
{
    semWait(writeSemaphore());
    return checkEof(readSemaphore());
}

void
ShmemBuffer::sendWriteEof()
{
    m_doCleanup = false;
    sem_post(writeSemaphore());
    sem_post(readSemaphore());
}

void
ShmemBuffer::sendReadEof()
{
    m_doCleanup = false;
    sem_post(readSemaphore());
    sem_post(writeSemaphore());
}

void
ShmemBuffer::sendDoneWriting()
{
    sem_post(readSemaphore());
}

bool
ShmemBuffer::waitRead()
{
    semWait(readSemaphore());
    return checkEof(writeSemaphore());
}

void
ShmemBuffer::sendDoneReading()
{
    sem_post(writeSemaphore());
    sync();
}

size_t
ShmemBuffer::mappingSize() const
{
    return m_size + 2 * sizeof(sem_t);
}

sem_t *
ShmemBuffer::writeSemaphore()
{
    return static_cast<sem_t *>(m_buf);
}

sem_t *
ShmemBuffer::readSemaphore()
{
    return static_cast<sem_t *>(m_buf) + 1;
}

uint8_t *
ShmemBuffer::data()
{
    return static_cast<uint8_t *>(m_buf) + 2*sizeof(sem_t);
}

const uint8_t *
ShmemBuffer::data() const
{
    return static_cast<uint8_t *>(m_buf) + 2*sizeof(sem_t);
}
