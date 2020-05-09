#include "pointers/PointerHandler.h"
#include "UBUtility.h"
#include "code-injector/ASTFrontendInjector.h"

#include "clang/Basic/SourceManager.h"

#include <unordered_map>

// TODO add support of more allocation funcs

using namespace clang;

namespace ub_tester {

PointerHandler::PointerInfo_t::PointerInfo_t(bool shouldVisitNodes)
    : Init_{std::nullopt}, PointeeType_{std::nullopt}, shouldVisitNodes_{
                                                           shouldVisitNodes} {}

bool PointerHandler::shouldVisitNodes() {
  return !Pointers_.empty() && Pointers_.back().shouldVisitNodes_;
}

void PointerHandler::reset() {
  if (!Pointers_.empty())
    Pointers_.pop_back();
}

PointerHandler::PointerHandler(ASTContext* Context) : Context_{Context} {}

namespace {

using SizeCalculator = std::string (*)(CallExpr*, const std::string&,
                                       ASTContext* Context);

std::string MallocSize(CallExpr* CE, const std::string& PointeeType,
                       ASTContext* Context) {
  std::stringstream Res;
  Res << getExprAsString(CE->getArg(0), Context) << "/"
      << "sizeof(" << PointeeType << ")";
  return Res.str();
}

std::unordered_map<std::string, SizeCalculator> SizeCalculators = {
    {"malloc", &MallocSize}};

std::optional<SizeCalculator>
getSizeCalculationFunc(const std::string& FunctionName) {
  if (SizeCalculators.find(FunctionName) != SizeCalculators.end())
    return SizeCalculators[FunctionName];
  return {};
}

} // namespace

bool PointerHandler::VisitCallExpr(CallExpr* CE) {
  if (shouldVisitNodes() && CE->getDirectCallee()) {
    auto Calculator = getSizeCalculationFunc(
        CE->getDirectCallee()->getNameInfo().getAsString());
    if (Calculator) {
      Pointers_.back().Size_ << Calculator.value()(
          CE, Pointers_.back().PointeeType_.value(), Context_);
    }
  }
  return true;
}

std::pair<std::string, std::string> PointerHandler::getCtorFormats() {
  std::string SourceFormat = Pointers_.back().Init_.has_value() ? "#@" : "";
  std::stringstream OutputFormat;
  OutputFormat << "(" << (Pointers_.back().Init_.has_value() ? "@" : "")
               << (Pointers_.back().Size_.str().size() > 0 ? ", " : "")
               << (Pointers_.back().Size_.str().size() > 0
                       ? Pointers_.back().Size_.str()
                       : "")
               << ")";
  return {SourceFormat, OutputFormat.str()};
}

void PointerHandler::executeSubstitutionOfPointerCtor(VarDecl* VDecl) {
  SourceLocation Loc = getAfterNameLoc(VDecl, Context_);
  auto Formats = getCtorFormats();
  ASTFrontendInjector::getInstance().substitute(
      Context_, Loc, Formats.first, Formats.second, Pointers_.back().Init_);
}

bool PointerHandler::TraverseDecl(Decl* D) {
  VarDecl* VDecl = nullptr;
  if (D && (VDecl = dyn_cast<VarDecl>(D))) {
    if (!Context_->getSourceManager().isWrittenInMainFile(VDecl->getBeginLoc()))
      return true;
    Pointers_.emplace_back(VDecl->getType().getTypePtr()->isPointerType());
    if (shouldVisitNodes()) {
      Pointers_.back().PointeeType_ =
          dyn_cast<PointerType>(VDecl->getType().getTypePtr())
              ->getPointeeType()
              .getAsString();
    }
  }
  RecursiveASTVisitor<PointerHandler>::TraverseDecl(D);
  if (shouldVisitNodes()) {
    if (VDecl->hasInit()) {
      Pointers_.back().Init_ = getExprAsString(VDecl->getInit(), Context_);
    }
    executeSubstitutionOfPointerCtor(VDecl);
  }
  reset();
  return true;
}

} // namespace ub_tester
