/**
 * File: symtable.h
 * ----------- 
 *  Header file for Symbol table implementation.
 */

#ifndef _H_symtable
#define _H_symtable

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <map>
#include "ast.h"
#include "ast_decl.h"
#include "irgen.h"

using namespace std;

typedef map<string, llvm::Value*> scope;

class SymbolTable {

  public:

    vector<scope> sv;
    bool global;
    SymbolTable();
    void Push(scope *s);
    void Pop();
    void AddSymbol(string id, llvm::Value* val);
    llvm::Value *LookUpValue(string id);
    llvm::Value *LookUpHelper(string id, scope *s);
    scope *CurrScope();
};

#endif
