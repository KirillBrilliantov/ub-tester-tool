#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/MultiplexConsumer.h"
#include "clang/Tooling/Tooling.h"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include "ASTFrontendInjector.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...\n");

namespace ub_tester {
class UBTesterAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance& Compiler, llvm::StringRef InFile) {

    ASTFrontendInjector::getInstance().addFile(
        Compiler.getASTContext().getSourceManager());

    // std::unique_ptr<ASTConsumer> consumer1 =
    //    std::make_unique<SomeConsumer>(Compiler.getASTContext());
    // Create your consumers here

    std::vector<std::unique_ptr<ASTConsumer>> consumers;
    // consumers.emplace_back(std::move(consumer1));
    // Add your consumers to vector here

    return std::make_unique<clang::MultiplexConsumer>(std::move(consumers));
  }
};
} // namespace ub_tester

int main(int argc, const char** argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(
      OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
  return Tool.run(newFrontendActionFactory<ub_tester::UBTesterAction>().get());
}
