//
// Created by taganyer on 3/12/24.
//

#ifndef NET_FILEDESCRIPTOR_HPP
#define NET_FILEDESCRIPTOR_HPP

#include "tinyBackend/Base/Detail/NoCopy.hpp"

namespace Net {

    class FileDescriptor : Base::NoCopy {
    public:
        explicit FileDescriptor(int fd) : _fd(fd) {};

        FileDescriptor(FileDescriptor&& other) noexcept: _fd(other._fd) {};

        virtual ~FileDescriptor();

        operator bool() const { return _fd > 0; };

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] bool valid() const { return _fd > 0; };

    protected:
        int _fd = -1;

    };

}


#endif //NET_FILEDESCRIPTOR_HPP
