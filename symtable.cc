/*
 * Symbol table implementation
 *
 */

#include "symtable.h"
#include "ast.h"
#include "ast_decl.h"
#include "ast_type.h"

SymbolTable::SymbolTable() {
  symbolTable = new vector<map<string, pair<Decl*, llvm::Value*> > >();
  currScope = 0;
}

void SymbolTable::Push() {
  map<string, pair<Decl*, llvm::Value*> > scope = map<string, pair<Decl*, llvm::Value*> >();
  symbolTable->push_back(scope);
  currScope++;
}

void SymbolTable::Pop() {
  symbolTable->pop_back();
  currScope--;
}

void SymbolTable::AddSymbol(string id, Decl *decl, llvm::Value *val) {
  symbolTable->at(currScope).insert(pair<string, pair<Decl*, llvm::Value*> >(id, pair<Decl*, llvm::Value*>(decl, val)));
}

llvm::Value *SymbolTable::LookUpValue(string id) {
  int cnt = 0;
  for(int i = currScope; i >= 0; i--) {
    cnt = symbolTable->at(i).count(id);
    if(cnt > 0)
      return symbolTable->at(i).at(id).second;
  }
  return NULL;
}