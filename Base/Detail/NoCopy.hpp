//
// Created by taganyer on 24-2-21.
//

#ifndef BASE_NOCOPY_HPP
#define BASE_NOCOPY_HPP

#ifdef BASE_NOCOPY_HPP

namespace Base {

    class NoCopy {
    public:
        NoCopy(const NoCopy &) = delete;

        NoCopy& operator=(const NoCopy &) = delete;

    protected:
        NoCopy() = default;

        ~NoCopy() = default;

    };

}

#endif

#endif //BASE_NOCOPY_HPP
