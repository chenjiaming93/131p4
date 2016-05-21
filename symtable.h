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
#include <iostream>
#include "location.h"
#include "ast_decl.h"
#include "errors.h"

using namespace std;

class SymbolTable {

  public:
    typedef enum {
      Program,
      Function,
      Loop,
      Block,
      Conditional,
      Switch
    } scope;

    SymbolTable();
    void Push(scope sp);
    void Pop();
    void AddSymbol(string id, Decl *decl);
    Decl *LookUpCurr(string id);
    Decl *LookUpScope(string id);

    vector<scope> *scopeStack;
    vector<map<string, Decl*> > *symbolTable;
    int currScope;
    bool returned;
    FnDecl *lastFun;

};

#endif
