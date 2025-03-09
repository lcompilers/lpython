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
        INTRINSIC_SUBROUTINE_NAME_CASE(RandomInit)
        INTRINSIC_SUBROUTINE_NAME_CASE(RandomSeed)
        INTRINSIC_SUBROUTINE_NAME_CASE(GetCommand)
        INTRINSIC_SUBROUTINE_NAME_CASE(GetCommandArgument)
        INTRINSIC_SUBROUTINE_NAME_CASE(GetEnvironmentVariable)
        INTRINSIC_SUBROUTINE_NAME_CASE(ExecuteCommandLine)
        INTRINSIC_SUBROUTINE_NAME_CASE(CpuTime)
        INTRINSIC_SUBROUTINE_NAME_CASE(Srand)
        INTRINSIC_SUBROUTINE_NAME_CASE(SystemClock)
        INTRINSIC_SUBROUTINE_NAME_CASE(DateAndTime)
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
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomInit),
            {&RandomInit::instantiate_RandomInit, &RandomInit::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomSeed),
            {&RandomSeed::instantiate_RandomSeed, &RandomSeed::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::Srand),
            {&Srand::instantiate_Srand, &Srand::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::GetCommand),
            {&GetCommand::instantiate_GetCommand, &GetCommand::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::GetCommandArgument),
            {&GetCommandArgument::instantiate_GetCommandArgument, &GetCommandArgument::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::SystemClock),
            {&SystemClock::instantiate_SystemClock, &SystemClock::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::DateAndTime),
            {&DateAndTime::instantiate_DateAndTime, &DateAndTime::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::GetEnvironmentVariable),
            {&GetEnvironmentVariable::instantiate_GetEnvironmentVariable, &GetEnvironmentVariable::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::ExecuteCommandLine),
            {&ExecuteCommandLine::instantiate_ExecuteCommandLine, &ExecuteCommandLine::verify_args}},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::CpuTime),
            {&CpuTime::instantiate_CpuTime, &CpuTime::verify_args}},
    };

    static const std::map<int64_t, std::string>& intrinsic_subroutine_id_to_name = {
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomNumber),
            "random_number"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomInit),
            "random_init"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::RandomSeed),
            "random_seed"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::Srand),
            "srand"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::GetCommand),
            "get_command"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::GetCommandArgument),
            "get_command_argument"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::SystemClock),
            "system_clock"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::DateAndTime),
            "date_and_time"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::GetEnvironmentVariable),
            "get_environment_variable"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::ExecuteCommandLine),
            "execute_command_line"},
        {static_cast<int64_t>(IntrinsicImpureSubroutines::CpuTime),
            "cpu_time"},
    };


    static const std::map<std::string,
        create_intrinsic_subroutine>& intrinsic_subroutine_by_name_db = {
                {"random_number", &RandomNumber::create_RandomNumber},
                {"random_init", &RandomInit::create_RandomInit},
                {"random_seed", &RandomSeed::create_RandomSeed},
                {"srand", &Srand::create_Srand},
                {"get_command", &GetCommand::create_GetCommand},
                {"get_command_argument", &GetCommandArgument::create_GetCommandArgument},
                {"system_clock", &SystemClock::create_SystemClock},
                {"get_environment_variable", &GetEnvironmentVariable::create_GetEnvironmentVariable},
                {"execute_command_line", &ExecuteCommandLine::create_ExecuteCommandLine},
                {"cpu_time", &CpuTime::create_CpuTime},
                {"date_and_time", &DateAndTime::create_DateAndTime},
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

    inline std::string get_intrinsic_subroutine_name(int64_t id) {
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
