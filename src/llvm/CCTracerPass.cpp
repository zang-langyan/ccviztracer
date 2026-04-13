#include "llvm/Passes/PassBuilder.h"
// #include "llvm/Passes/PassPlugin.h"/* llvm17 */
#include "llvm/Plugins/PassPlugin.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Demangle/Demangle.h"

namespace cctracer {

class CCTracerPass : public llvm::PassInfoMixin<CCTracerPass> {
private:
    bool shouldInstrument(llvm::Function& F) {
        if (F.isIntrinsic()) return false;
        if (F.isDeclaration()) return false;
        std::string func_name = llvm::demangle(F.getName().str());
        if (func_name == "__cctracer_function_entry" || func_name == "__cctracer_function_exit") {
            return false;
        }
        return true;
    }
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &) {
        if (!shouldInstrument(F)) {
            return llvm::PreservedAnalyses::all();
        }

        llvm::Module& M = *F.getParent();
        llvm::LLVMContext& Ctx = M.getContext();

        // void __cctracer_function_entry(const char* func_name, const char* file_name, int line, int column)
        llvm::FunctionType* EnterType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(Ctx),
            {
                llvm::PointerType::getUnqual(Ctx), // func_name
                llvm::PointerType::getUnqual(Ctx), // file_name
                llvm::Type::getInt32Ty(Ctx), // line
                llvm::Type::getInt32Ty(Ctx)  // column
            },
            false
        );
        llvm::FunctionCallee EnterFunc = M.getOrInsertFunction("__cctracer_function_entry", EnterType);

        // void __cctracer_function_exit(const char* func_name, const char* file_name, int line, int column)
        llvm::FunctionType* ExitType = llvm::FunctionType::get(
            llvm::Type::getVoidTy(Ctx),
            {
                llvm::PointerType::getUnqual(Ctx), // func_name
                llvm::PointerType::getUnqual(Ctx), // file_name
                llvm::Type::getInt32Ty(Ctx), // line
                llvm::Type::getInt32Ty(Ctx)  // column
            },
            false
        );
        llvm::FunctionCallee ExitFunc = M.getOrInsertFunction("__cctracer_function_exit", ExitType);

        /* Extract function information */
        std::string func_name = llvm::demangle(F.getName().str());
        std::string file_name = "";
        int start_line = 0;
        int start_col = 0;
        int end_line = 0;
        int end_col = 0;
        if (llvm::DISubprogram *SP = F.getSubprogram()) {
            start_line = SP->getLine();
            file_name = SP->getFilename().str();
            // first instruction debug location
            for (llvm::BasicBlock &BB : F) {
                for (llvm::Instruction &I : BB) {
                    if (llvm::DILocation *Loc = I.getDebugLoc()) {
                        if (Loc->getLine() < start_line) {
                            start_line = Loc->getLine();
                            file_name = Loc->getFilename().str();
                            start_col = Loc->getColumn();
                        }
                        if (Loc->getLine() > end_line) {
                            end_line = Loc->getLine();
                            end_col = Loc->getColumn();
                        }
                    }
                }
            }
        }

        /* Instrument function entry */
        {
            llvm::BasicBlock& EntryBB = F.getEntryBlock();
            llvm::Instruction* FirstInst = &*EntryBB.getFirstInsertionPt();
            if (!FirstInst) {
                llvm::errs() << "Warning: Function " << llvm::demangle(F.getName().str()) << " has no instructions, skipping instrumentation.\n";
                return llvm::PreservedAnalyses::all();
            }
            llvm::IRBuilder<> Builder(&EntryBB, EntryBB.begin());
            llvm::Value *FuncNamePtr = Builder.CreateGlobalString(func_name);
            llvm::Value *FileNamePtr = Builder.CreateGlobalString(file_name);
            Builder.CreateCall(EnterFunc, {FuncNamePtr, FileNamePtr, Builder.getInt32(start_line), Builder.getInt32(start_col)});
        }
        /* Instrument return instructions */
        {
            for (llvm::BasicBlock &BB : F) {
                for (llvm::Instruction &I : BB) {
                    if (llvm::isa<llvm::ReturnInst>(&I)) {
                        llvm::IRBuilder<> Builder(&I);
                        llvm::Value *FuncNamePtr = Builder.CreateGlobalString(func_name);
                        llvm::Value *FileNamePtr = Builder.CreateGlobalString(file_name);
                        Builder.CreateCall(ExitFunc, {FuncNamePtr, FileNamePtr, Builder.getInt32(end_line), Builder.getInt32(end_col)});
                    }
                }
            }
        }

        
        // llvm::errs() << "Instrumenting function: " << func_name << "\n";

        return llvm::PreservedAnalyses::none();
    }

    static bool isRequired() { return true; }
};

} // namespace cctracer

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "CCTracerPass", "v0.1",
        [](llvm::PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](llvm::ModulePassManager &MPM, llvm::OptimizationLevel Level) {
                    llvm::FunctionPassManager FPM;
                    FPM.addPass(cctracer::CCTracerPass());
                    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
            // PB.registerPipelineParsingCallback(
            //     [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
            //        llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) -> bool {
            //         if (Name == "cctracer_pass") {
            //             FPM.addPass(CCTracerPass{});
            //             return true;
            //         }
            //         return false;
            //     }
            // );
        }
    };
}
