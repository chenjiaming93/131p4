/*
 * Symbol table implementation
 *
 */

#include "symtable.h"
#include "ast.h"
#include "ast_decl.h"
#include "ast_type.h"

SymbolTable::SymbolTable() {
  map<string, llvm::Value*> s;
  vector<scope> ss;
  sv = ss;
  sv.push_back(s);
  global = false;
}

void SymbolTable::Push(scope *s) {
  sv.push_back(*s);
}

void SymbolTable::Pop() {
  sv.pop_back();
}

void SymbolTable::AddSymbol(string id, llvm::Value *val) {
  scope *s = &sv.back();
  s->insert(pair<string, llvm::Value*>(id,val));
}

llvm::Value *SymbolTable::LookUpValue(string id) {
  llvm::Value* val = NULL;
  for(vector<scope>::reverse_iterator it = sv.rbegin(); it != sv.rend(); it++) {
    scope* s = &(*it);
    val = LookUpHelper(id, s);
    if(val != NULL)
      break;
  }
  return val;
}

llvm::Value *SymbolTable::LookUpHelper(string id, scope *s) {
  if(s->count(id) > 0)
    return s->at(id);
  return NULL;
}

scope *SymbolTable::CurrScope() {
  return &(sv.back());
}
