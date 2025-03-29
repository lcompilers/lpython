#ifndef LFORTRAN_PASS_INTRINSIC_SUBROUTINE_REGISTRY_H
#define LFORTRAN_PASS_INTRINSIC_SUBROUTINE_REGISTRY_H

// #include <libasr/pass/intrinsic_function_registry_util.h>
#include <libasr/pass/intrinsic_subroutines.h>

#include <cmath>
#include <string>
#include <tuple>

namespace LCompilers {

namespace ASRUtils {

#define INTRINSIC_SUBROUTINE_NAME_CASE(X)                                                  \
    case (static_cast<int64_t>(ASRUtils::IntrinsicImpureSubroutines::X)) : {   \
        return #X;                                                              \
    }

inline std::string get_intrinsic_subroutine_name(int x) {
    switch (x) {
        INTRINSIC_SUBROUTINE_NAME_CASE(RandomNumber)
        default : {
            throw LCompilersException("pickle: intrinsic_id not implemented");
        }
    }
}

/************************* Intrinsic Impure Subroutine **************************/

namespace IntrinsicImpureSubroutineRegistry {

    static const std::map<int64_t,
        std::tuple<impl_subroutine,
                   verify_subroutine>>& intrinsic_subroutine_by_id_db = {
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomNumber),
            {&RandomNumber::instantiate_RandomNumber, &RandomNumber::verify_args}},
    };

    static const std::map<int64_t, std::string>& intrinsic_subroutine_id_to_name = {
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomNumber),
            "random_number"},
    };


    static const std::map<std::string,
        create_intrinsic_subroutine>& intrinsic_subroutine_by_name_db = {
                {"random_number", &RandomNumber::create_RandomNumber},
    };

    static inline bool is_intrinsic_subroutine(const std::string& name) {
        return intrinsic_subroutine_by_name_db.find(name) != intrinsic_subroutine_by_name_db.end();
    }

    static inline bool is_intrinsic_subroutine(int64_t id) {
        return intrinsic_subroutine_by_id_db.find(id) != intrinsic_subroutine_by_id_db.end();
    }

    static inline create_intrinsic_subroutine get_create_subroutine(const std::string& name) {
        return  intrinsic_subroutine_by_name_db.at(name);
    }

    static inline verify_subroutine get_verify_subroutine(int64_t id) {
        return std::get<1>(intrinsic_subroutine_by_id_db.at(id));
    }

    static inline impl_subroutine get_instantiate_subroutine(int64_t id) {
        if( intrinsic_subroutine_by_id_db.find(id) == intrinsic_subroutine_by_id_db.end() ) {
            return nullptr;
        }
        return std::get<0>(intrinsic_subroutine_by_id_db.at(id));
    }

    static inline std::string get_intrinsic_subroutine_name(int64_t id) {
        if( intrinsic_subroutine_id_to_name.find(id) == intrinsic_subroutine_id_to_name.end() ) {
            throw LCompilersException("IntrinsicSubroutine with ID " + std::to_string(id) +
                                      " has no name registered for it");
        }
        return intrinsic_subroutine_id_to_name.at(id);
    }

} // namespace IntrinsicImpureSubroutineRegistry

} // namespace ASRUtils

} // namespace LCompilers

#endif // LFORTRAN_PASS_INTRINSIC_SUBROUTINE_REGISTRY_H
