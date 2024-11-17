//
// Created by taganyer on 24-10-29.
//

#ifndef LOGSYSTEM_LINKLOGERROR_HPP
#define LOGSYSTEM_LINKLOGERROR_HPP

#ifdef LOGSYSTEM_LINKLOGERROR_HPP

#include <utility>
#include "Base/Exception.hpp"
#include "LogSystem/linkLog/Identification.hpp"

namespace LogSystem {

    enum LinkErrorType : uint8 {
        Success,
        ExtraFollow,
        WrongType,
        ExtraDecision,
        ConflictingNode,
        FailureNotice,
        NotRegister,
        AlreadyCreated,
        CreateTimeOut,
        EndTimeOut
    };

    inline const char* getLinkErrorTypeName(LinkErrorType errorType) {
        constexpr const char* errorTypeNames[] = {
            "Success",
            "ExtraFollow",
            "WrongType",
            "ExtraDecision",
            "ConflictingNode",
            "FailureNotice",
            "NotRegister",
            "AlreadyCreated",
            "CreateTimeout",
            "EndTimeout"
        };
        return errorTypeNames[errorType];
    };

    class LinkLogError : public Base::Exception {
    public:
        LinkLogError(const LinkServiceID &service, const LinkNodeID &node,
                     LinkErrorType errorType_, std::string message) :
            Exception(std::move(message)),
            id(service, node), errorType(errorType_) {};

        Identification id {};

        LinkErrorType errorType;
    };

    class LinkLogCreateError : public LinkLogError {
    public:
        LinkLogCreateError(const LinkServiceID &service, const LinkNodeID &node,
                           LinkErrorType errorType_) :
            LinkLogError(service, node, errorType_,
                         "LinkLogCreateError: service["
                         + std::string(service.data(), sizeof(LinkServiceID))
                         + "] node["
                         + std::string(node.data(), sizeof(LinkNodeID))
                         + "] reason : "
                         + getLinkErrorTypeName(errorType_)) {};

    };

    class LinkLogRegisterError : public LinkLogError {
    public:
        LinkLogRegisterError(const LinkServiceID &service, const LinkNodeID &node,
                             LinkErrorType errorType_):
            LinkLogError(service, node, errorType_,
                         "LinkLogRegisterError: service["
                         + std::string(service.data(), sizeof(LinkServiceID))
                         + "] node["
                         + std::string(node.data(), sizeof(LinkNodeID))
                         + "] reason : "
                         + getLinkErrorTypeName(errorType_)) {};
    };

}

#endif

#endif //LOGSYSTEM_LINKLOGERROR_HPP
