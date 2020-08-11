#include <iostream>
#include <memory>

#include <lfortran/asr.h>
#include <lfortran/containers.h>
#include <lfortran/codegen/asr_to_cpp.h>
#include <lfortran/exception.h>
#include <lfortran/asr_utils.h>


namespace LFortran {

class ASRToCPPVisitor : public ASR::BaseVisitor<ASRToCPPVisitor>
{
public:
    std::string src;

    void visit_TranslationUnit(const ASR::TranslationUnit_t &x) {
        // All loose statements must be converted to a function, so the items
        // must be empty:
        LFORTRAN_ASSERT(x.n_items == 0);
        for (auto &item : x.m_global_scope->scope) {
            visit_asr(*item.second);
        }
    }

    void visit_Program(const ASR::Program_t &x) {
        src =
R"(int main()
{
    return 0;
})";
    }

};

std::string asr_to_cpp(ASR::asr_t &asr)
{
    ASRToCPPVisitor v;
    LFORTRAN_ASSERT(asr.type == ASR::asrType::unit);
    v.visit_asr(asr);
    return v.src;
}

} // namespace LFortran
