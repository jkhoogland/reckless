#include "reckless/file_writer.hpp"

#include <system_error>

#include <sys/stat.h>   // open()
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

reckless::file_writer::file_writer(char const* path) :
    fd_(-1)
{
    // FIXME use correct mode flags here.
    auto full_access =
        S_IRUSR | S_IWUSR | S_IXUSR |
        S_IRGRP | S_IWGRP | S_IXGRP |
        S_IROTH | S_IWOTH | S_IXOTH;
    fd_ = open(path, O_WRONLY | O_CREAT, full_access);
    if(fd_ == -1)
        throw std::system_error(errno, std::system_category());
    lseek(fd_, 0, SEEK_END);
}

reckless::file_writer::~file_writer()
{
    if(fd_ != -1)
        close(fd_);
}

auto reckless::file_writer::write(void const* pbuffer, std::size_t count) -> Result
{
    char const* p = static_cast<char const*>(pbuffer);
    while(count != 0) {
        ssize_t written = ::write(fd_, p, count);
        if(written == -1) {
            if(errno != EINTR)
                break;
        } else {
            p += written;
            count -= written;
        }
    }
    if(count == 0)
        return SUCCESS;

    // TODO handle broken pipe signal?
    switch(errno) {
    case EFBIG:
    case EIO:
    case EPIPE:
    case ERANGE:
    case ECONNRESET:
    case EINVAL:
    case ENXIO:
    case EACCES:
    case ENETDOWN:
    case ENETUNREACH:
        // TODO handle this error by not writing to the buffer any more.
        return ERROR_GIVE_UP;
    case ENOSPC:
        return ERROR_TRY_LATER;
    default:
        // TODO throw proper error
        throw std::runtime_error("cannot write to file descriptor");
    }
}
