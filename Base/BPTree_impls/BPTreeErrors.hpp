//
// Created by taganyer on 24-9-21.
//

#ifndef BPTREEERRORS_HPP
#define BPTREEERRORS_HPP

#ifdef BPTREEERRORS_HPP

#include "Base/Exception.hpp"

namespace Base {

    class BPTreeError : public Exception {
    public:
        BPTreeError() : Exception("BPTreeError") {};

        BPTreeError(std::string s) : Exception(std::move(s)) {};
    };

    class BPTreeRuntimeError : public BPTreeError {
    public:
        BPTreeRuntimeError() : BPTreeError("BPTreeRuntimeError") {};

        BPTreeRuntimeError(std::string s) : BPTreeError(std::move(s)) {};
    };

    class BPTreeFileError : public BPTreeError {
    public:
        BPTreeFileError() : BPTreeError("BPTreeFileError") {};

        BPTreeFileError(std::string s) : BPTreeError(std::move(s)) {};
    };

}

#endif

#endif //BPTREEERRORS_HPP
