//===- KaleidoscopeJIT.h - A simple JIT for Kaleidoscope --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Contains a simple JIT definition for use in the kaleidoscope tutorials.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EXECUTIONENGINE_ORC_KALEIDOSCOPEJIT_H
#define LLVM_EXECUTIONENGINE_ORC_KALEIDOSCOPEJIT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include <memory>

#if LLVM_VERSION_MAJOR >= 13
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#endif

#if LLVM_VERSION_MAJOR >= 16
#    define RM_OPTIONAL_TYPE std::optional
#else
#    define RM_OPTIONAL_TYPE llvm::Optional
#endif

namespace llvm {
namespace orc {

class KaleidoscopeJIT {
private:
  std::unique_ptr<ExecutionSession> ES;
  RTDyldObjectLinkingLayer ObjectLayer;
  IRCompileLayer CompileLayer;

  DataLayout DL;
  MangleAndInterner Mangle;
  JITDylib &JITDL;

public:
  KaleidoscopeJIT(std::unique_ptr<ExecutionSession> ES, JITTargetMachineBuilder JTMB, DataLayout DL)
      :
        ES(std::move(ES)),
        ObjectLayer(*this->ES,
                    []() { return std::make_unique<SectionMemoryManager>(); }),
        CompileLayer(*this->ES, ObjectLayer, std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
        DL(std::move(DL)), Mangle(*this->ES, this->DL),
        JITDL(
#if LLVM_VERSION_MAJOR >= 11
            cantFail
#endif
          (this->ES->createJITDylib("Main"))) {
    JITDL.addGenerator(
        cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(
            DL.getGlobalPrefix())));
    if (JTMB.getTargetTriple().isOSBinFormatCOFF()) {
      ObjectLayer.setOverrideObjectFlagsWithResponsibilityFlags(true);
      ObjectLayer.setAutoClaimResponsibilityForObjectSymbols(true);
    }
  }

  static Expected<std::unique_ptr<KaleidoscopeJIT>> Create() {
#if LLVM_VERSION_MAJOR >= 13
    auto EPC = SelfExecutorProcessControl::Create();
    if (!EPC)
      return EPC.takeError();

    auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

    JITTargetMachineBuilder JTMB(
        ES->getExecutorProcessControl().getTargetTriple());
#else
    auto ES = std::make_unique<ExecutionSession>();

    auto JTMB_P = JITTargetMachineBuilder::detectHost();
    if (!JTMB_P)
      return JTMB_P.takeError();

    auto JTMB = *JTMB_P;
#endif

    auto DL = JTMB.getDefaultDataLayoutForTarget();
    if (!DL)
      return DL.takeError();

    return std::make_unique<KaleidoscopeJIT>(std::move(ES), std::move(JTMB),
                                             std::move(*DL));
  }

  const DataLayout &getDataLayout() const { return DL; }

  Error addModule(std::unique_ptr<Module> M, std::unique_ptr<LLVMContext> &Ctx) {
    auto res =  CompileLayer.add(JITDL,
                            ThreadSafeModule(std::move(M), std::move(Ctx)));
    Ctx = std::make_unique<LLVMContext>();
    return res;
  }

#if LLVM_VERSION_MAJOR < 17
  Expected<JITEvaluatedSymbol> lookup(StringRef Name) {
#else
  Expected<ExecutorSymbolDef> lookup(StringRef Name) {
#endif
    return ES->lookup({&JITDL}, Mangle(Name.str()));
  }

};

} // end namespace orc
} // end namespace llvm

#endif // LLVM_EXECUTIONENGINE_ORC_KALEIDOSCOPEJIT_H
