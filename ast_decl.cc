/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "symtable.h"
#include "irgen.h"
  
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

VarDecl::VarDecl(Identifier *n, Type *t, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    typeq = NULL;
}

VarDecl::VarDecl(Identifier *n, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && tq != NULL);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    type = NULL;
}

VarDecl::VarDecl(Identifier *n, Type *t, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL && tq != NULL);
    (type=t)->SetParent(this);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
}
  
void VarDecl::PrintChildren(int indentLevel) { 
   if (typeq) typeq->Print(indentLevel+1);
   if (type) type->Print(indentLevel+1);
   if (id) id->Print(indentLevel+1);
   if (assignTo) assignTo->Print(indentLevel+1, "(initializer) ");
}

llvm::Value *VarDecl::Emit() {
    llvm::Module *module = irgen->GetOrCreateModule("glsl.bc");
    llvm::Type *type = irgen->GetType(this->GetType());
    llvm::Twine *twine = new llvm::Twine(this->GetIdentifier()->GetName());

    // Global Var
    if (symtab->global == true) {
        llvm::GlobalVariable *var = new llvm::GlobalVariable(*module, type, false, llvm::GlobalValue::ExternalLinkage, llvm::Constant::getNullValue(type), *twine);
        symtab->AddSymbol(this->GetIdentifier()->GetName(), var);
        return var;
    }
    //local var
    else {
        llvm::BasicBlock *bb = Node::irgen->GetBasicBlock();
        llvm::Value *value =  new llvm::AllocaInst(type, *twine, bb);
        symtab->AddSymbol(this->GetIdentifier()->GetName(), value); 
        return value;
    }
}


FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    returnTypeq = NULL;
}

FnDecl::FnDecl(Identifier *n, Type *r, TypeQualifier *rq, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r != NULL && rq != NULL&& d != NULL);
    (returnType=r)->SetParent(this);
    (returnTypeq=rq)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    if (returnType) returnType->Print(indentLevel+1, "(return type) ");
    if (id) id->Print(indentLevel+1);
    if (formals) formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}

llvm::Value* FnDecl::Emit() {
    symtab->global = false;
    scope s;
    symtab->Push(&s);

    llvm::Module *module = irgen->GetOrCreateModule("glsl.bc");
    llvm::Type *type = irgen->GetType(this->GetType());
    llvm::Twine *twine = new llvm::Twine(this->GetIdentifier()->GetName());

    vector<llvm::Type*> v;
    for(int i = 0; i < this->GetFormals()->NumElements(); i++) {
        VarDecl* decl = this->GetFormals()->Nth(i);
        v.push_back(irgen->GetType(decl->GetType()));
    }

    llvm::ArrayRef<llvm::Type*> arrR(v);
    llvm::FunctionType *funType = llvm::FunctionType::get(type, arrR, false);
    llvm::Function *fun = llvm::cast<llvm::Function>(irgen->GetOrCreateModule("glsl.bc")->getOrInsertFunction(llvm::StringRef(this->id->GetName()), funType));
    irgen->SetFunction(fun);

    llvm::LLVMContext *context = irgen->GetContext();


    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*context, *twine, fun, irgen->GetBasicBlock());
    irgen->SetBasicBlock(bb);

    int i = 0;
    for(llvm::Function::arg_iterator arg = fun->arg_begin(); arg != fun->arg_end(); arg++, i++) {
        VarDecl* decl = this->GetFormals()->Nth(i);
        llvm::Value* v = decl->Emit();
        string id = decl->GetIdentifier()->GetName();
        arg->setName(id);
        llvm::Value *val = new llvm::StoreInst(arg, v, irgen->GetBasicBlock());
    }
    body->Emit();
    symtab->Pop();
    return fun;
}

