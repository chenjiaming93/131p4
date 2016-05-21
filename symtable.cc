/*
 * Symbol table implementation
 *
 */

#include "symtable.h"

SymbolTable::SymbolTable() {
  symbolTable = new vector<map<string, Decl*> >();
  scopeStack = new vector<scope>();
  currScope = 0;
}

void SymbolTable::Push(scope sp) {
  map<string, Decl*> newScope = map<string, Decl*>();;
  symbolTable->push_back(newScope);
  scopeStack->push_back(sp);
  currScope++;
}

void SymbolTable::Pop() {
  symbolTable->pop_back();
  scopeStack->pop_back();
  currScope--;
}

void SymbolTable::AddSymbol(string id, Decl *decl) {
  if(LookUpCurr(id) != NULL)
    ReportError::DeclConflict(decl, LookUpCurr(id));
  symbolTable->at(currScope).insert(pair<string, Decl*>(id, decl));
}

Decl *SymbolTable::LookUpCurr(string id) {
  int cnt = symbolTable->at(currScope).count(id);
  if (cnt > 0)
    return symbolTable->at(currScope).at(id);
  return NULL;
}

Decl *SymbolTable::LookUpScope(string id) {
  int cnt = 0;
  for(int i = currScope; i >= 0; i--) {
    cnt = symbolTable->at(i).count(id);
    if(cnt > 0)
      return symbolTable->at(i).at(id);
  }
  return NULL;
}