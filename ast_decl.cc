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
    llvm::Type *type = IRGenerator::AstToLLVM(this->GetType(), irgen->GetContext());
    llvm::Twine *twine = new llvm::Twine(this->id->GetName());
    
    // Global Var
    if (symtab->currScope == 1) {
        llvm::GlobalVariable *var = new llvm::GlobalVariable(*module,type,false,llvm::GlobalValue::ExternalLinkage,llvm::Constant::getNullValue(type),*twine);
    }
    //local var
    else {
        llvm::BasicBlock *bb = Node::irgen->GetBasicBlock();
        llvm::Value *val =  new llvm::AllocaInst(type,*twine, bb);
    }
    return NULL;
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
    symtab->Push();
    vector<llvm::Type*> v;
    llvm::Type *type = NULL;
    if(this->returnType == Type::intType)
        type = irgen->GetIntType();
    else if(this->returnType == Type::boolType)
        type = irgen->GetBoolType();
    else if(this->returnType == Type::floatType)
        type = irgen->GetFloatType();

    for(int i = 0; i < formals->NumElements(); i++) {
        Type *ty = formals->Nth(i)->GetType();
        llvm::Type *ts = IRGenerator::AstToLLVM(ty, irgen->GetContext());
        v.push_back(ts);
    }

    llvm::ArrayRef<llvm::Type*> arrR(v);
    llvm::FunctionType *funType = llvm::FunctionType::get(type, arrR, false);
    llvm::Function *fun = llvm::cast<llvm::Function>(irgen->GetOrCreateModule("glsl.bc")->getOrInsertFunction(llvm::StringRef(this->id->GetName()), funType));
    irgen->SetFunction(fun);
    llvm::LLVMContext *context = irgen->GetContext();
    const llvm::Twine *twine = new llvm::Twine(this->id->GetName());

    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*context, *twine, fun, irgen->GetBasicBlock());
    irgen->SetBasicBlock(bb);

    int i = 0;
    for(llvm::Function::arg_iterator arg = fun->arg_begin(); arg != fun->arg_end(); arg++, i++) {
        formals->Nth(i)->Emit();
        string id = formals->Nth(i)->GetIdName();
        arg->setName(id);
        llvm::Value *val = symtab->LookUpValue(id);
        llvm::Value *v = &*arg;
        new llvm::StoreInst(v, val, bb);
    }
    body->Emit();
    symtab->Pop();
    return NULL;
}

