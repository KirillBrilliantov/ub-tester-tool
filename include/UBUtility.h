#pragma once

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include <string>
#include <unordered_set>

namespace ub_tester::util {

std::string getExprAsString(const clang::Expr*, const clang::ASTContext*);
std::string getExprLineNCol(const clang::Expr*, const clang::ASTContext*);
clang::QualType getLowestLevelPointeeType(clang::QualType);

std::string getRangeAsString(const clang::SourceRange& Range,
                             const clang::ASTContext* Context);

clang::SourceLocation getNameLastLoc(const clang::DeclaratorDecl*,
                                     const clang::ASTContext*);
clang::SourceLocation getAfterNameLoc(const clang::DeclaratorDecl*,
                                      const clang::ASTContext*);

std::string getFuncNameWithArgsAsString(const clang::FunctionDecl* FuncDecl);

namespace func_code_avail {

bool hasFuncAvailCode(const clang::FunctionDecl* FuncDecl);
void setHasFuncAvailCode(const clang::FunctionDecl* FuncDecl);

class GetFuncCodeAvailVisitor
    : public clang::RecursiveASTVisitor<GetFuncCodeAvailVisitor> {
public:
  explicit GetFuncCodeAvailVisitor(clang::ASTContext* Context);
  bool VisitFunctionDecl(clang::FunctionDecl* FuncDecl);

private:
  clang::ASTContext* Context;
};

class UtilityConsumer : public clang::ASTConsumer {
public:
  explicit UtilityConsumer(clang::ASTContext* Context);
  virtual void HandleTranslationUnit(clang::ASTContext& Context);

private:
  GetFuncCodeAvailVisitor FuncCodeAvailVisitor;
};

} // namespace func_code_avail
} // namespace ub_tester::util
