//===--- ParentMap.cpp - Mappings from Stmts to their Parents ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the ParentMap class.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/ParentMap.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "llvm/ADT/DenseMap.h"

using namespace clang;

typedef llvm::DenseMap<Stmt*, Stmt*> MapTy;

static void BuildParentMap(MapTy& M, Stmt* S) {
  for (Stmt::child_iterator I=S->child_begin(), E=S->child_end(); I!=E; ++I)
    if (*I) {
      M[*I] = S;
      BuildParentMap(M, *I);
    }
}

ParentMap::ParentMap(Stmt* S) : Impl(0) {
  if (S) {
    MapTy *M = new MapTy();
    BuildParentMap(*M, S);
    Impl = M;    
  }
}

ParentMap::~ParentMap() {
  delete (MapTy*) Impl;
}

Stmt* ParentMap::getParent(Stmt* S) const {
  MapTy* M = (MapTy*) Impl;
  MapTy::iterator I = M->find(S);
  return I == M->end() ? 0 : I->second;
}

bool ParentMap::isConsumedExpr(Expr* E) const {
  Stmt *P = getParent(E);
  Stmt *DirectChild = E;
  
  // Ignore parents that are parentheses or casts.
  while (P && (isa<ParenExpr>(E) || isa<CastExpr>(E))) {
    DirectChild = P;
    P = getParent(P);
  }
  
  if (!P)
    return false;
  
  switch (P->getStmtClass()) {
    default:
      return isa<Expr>(P);
    case Stmt::DeclStmtClass:
      return true;
    case Stmt::BinaryOperatorClass: {
      BinaryOperator *BE = cast<BinaryOperator>(P);
      return BE->getOpcode()==BinaryOperator::Comma && DirectChild==BE->getLHS();
    }
    case Stmt::ForStmtClass:
      return DirectChild == cast<ForStmt>(P)->getCond();
    case Stmt::WhileStmtClass:
      return DirectChild == cast<WhileStmt>(P)->getCond();      
    case Stmt::DoStmtClass:
      return DirectChild == cast<DoStmt>(P)->getCond();
    case Stmt::IfStmtClass:
      return DirectChild == cast<IfStmt>(P)->getCond();
    case Stmt::IndirectGotoStmtClass:
      return DirectChild == cast<IndirectGotoStmt>(P)->getTarget();
    case Stmt::SwitchStmtClass:
      return DirectChild == cast<SwitchStmt>(P)->getCond();
    case Stmt::ReturnStmtClass:
      return true;
  }
}

