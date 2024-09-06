//
// Created by taganyer on 24-8-15.
//

#ifndef LOGSYSTEM_RESULTSET_HPP
#define LOGSYSTEM_RESULTSET_HPP

#ifdef LOGSYSTEM_RESULTSET_HPP

namespace LogSystem {

    template<typename Result>
    class ResultSet {
    public:
        ResultSet() = default;

        virtual ~ResultSet() = 0;

        virtual Result get_next() = 0;

        virtual bool have_next() = 0;

    };
}

#endif

#endif //LOGSYSTEM_RESULTSET_HPP
