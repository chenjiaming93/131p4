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
#include "ast_decl.h"
#include "ast.h"
#include "irgen.h"
#include "llvm/IR/Value.h"

using namespace std;

typedef map<string, llvm::Value*> scope;

class SymbolTable {

  protected:
    vector<scope> sv;

  public:

    SymbolTable();
    bool global;
    llvm::BasicBlock *breakBlock;
    llvm::BasicBlock *continueBlock;

    void Push(scope *s);
    void Pop();
    void AddSymbol(string id, llvm::Value *val);
    llvm::Value *LookUpValue(string id);
    llvm::Value *LookUpHelper(string id, scope *s);
};

#endif
