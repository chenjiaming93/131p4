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

class SymbolTable {

  public:

    SymbolTable();
    void Push();
    void Pop();
    void AddSymbol(string id, Decl *decl, llvm::Value *val);
    llvm::Value *LookUpValue(string id);
    vector<map<string, pair<Decl*, llvm::Value*> > > *symbolTable;
    int currScope;

};

#endif
