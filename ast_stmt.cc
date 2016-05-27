/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "symtable.h"

#include "irgen.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"                                                   


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    printf("\n");
}

llvm::Value *Program::Emit() {
    llvm::Module *module = irgen->GetOrCreateModule("glsl.bc");
    symtab->global = true;
    for(int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit();
    }
    llvm::WriteBitcodeToFile(module, llvm::outs());
    return NULL;
}

llvm::Value *ForStmt::Emit() {
    scope s;
    symtab->Push(&s);
    symtab->global = false;

    llvm::Function *f = irgen->GetFunction();
    llvm::LLVMContext *context = irgen->GetContext();

    llvm::BasicBlock *cb = irgen->GetBasicBlock();
    llvm::BasicBlock *fb = llvm::BasicBlock::Create(*context, "footer", f);
    llvm::BasicBlock *sb = llvm::BasicBlock::Create(*context, "step", f);
    llvm::BasicBlock *db = llvm::BasicBlock::Create(*context, "body", f);
    llvm::BasicBlock *hb = llvm::BasicBlock::Create(*context, "header", f);

    init->Emit();

    llvm::BranchInst::Create(hb, cb);
    hb->moveAfter(cb);
    irgen->SetBasicBlock(hb);

    llvm::Value *testVal = test->Emit();
    llvm::BranchInst::Create(db, fb, testVal, hb);

    irgen->SetBasicBlock(db);
    body->Emit();
    llvm::BranchInst::Create(sb, db);

    sb->moveAfter(db);
    irgen->SetBasicBlock(sb);
    step->Emit();
    llvm::BranchInst::Create(hb, sb);
    fb->moveAfter(sb);
    irgen->SetBasicBlock(fb);

    symtab->Pop();
    return NULL;
}

llvm::Value *WhileStmt::Emit() {
    scope s;
    symtab->Push(&s);
    symtab->global = false;

    llvm::Function *f = irgen->GetFunction();
    llvm::LLVMContext *context = irgen->GetContext();

    llvm::BasicBlock *cb = irgen->GetBasicBlock();
    llvm::BasicBlock *hb = llvm::BasicBlock::Create(*context, "header", f);
    llvm::BasicBlock *fb = llvm::BasicBlock::Create(*context, "footer", f);
    llvm::BasicBlock *db = llvm::BasicBlock::Create(*context, "body", f);

    llvm::BranchInst::Create(hb, cb);
    hb->moveAfter(cb);
    irgen->SetBasicBlock(hb);

    llvm::Value *testVal = test->Emit();
    llvm::BranchInst::Create(db, fb, testVal, hb);

    irgen->SetBasicBlock(db);
    body->Emit();
    llvm::BranchInst::Create(hb, db);
    fb->moveAfter(db);
    irgen->SetBasicBlock(fb);

    symtab->Pop();
    return NULL;
}

llvm::Value *IfStmt::Emit() {
    scope s;
    symtab->Push(&s);
    symtab->global = false;

    llvm::Function *f = irgen->GetFunction();
    llvm::LLVMContext *context = irgen->GetContext();
    llvm::Value *testVal = test->Emit();

    llvm::BasicBlock *cb = irgen->GetBasicBlock();
    llvm::BasicBlock *fb = llvm::BasicBlock::Create(*context, "footer", f);
    llvm::BasicBlock *eb = NULL;
    if (elseBody != NULL)
        eb = llvm::BasicBlock::Create(*context, "else", f);
    llvm::BasicBlock *tb = llvm::BasicBlock::Create(*context, "then", f);

    llvm::BranchInst::Create(tb, elseBody ? eb:fb, testVal, cb);
    tb->moveAfter(cb);
    irgen->SetBasicBlock(tb);
    body->Emit();
    eb->moveAfter(tb);

    if(tb->getTerminator() == NULL)
        llvm::BranchInst::Create(fb, tb);

    irgen->SetBasicBlock(eb);
    elseBody->Emit();
    fb->moveAfter(eb);

    if (eb != NULL && eb->getTerminator() == NULL)
        llvm::BranchInst::Create(fb, eb);

    if(pred_begin(fb) == pred_end(fb))
        new llvm::UnreachableInst(*context, fb);
    else
        irgen->SetBasicBlock(fb);

    symtab->Pop();
    return NULL;
}

llvm::Value *SwitchStmt::Emit() {
    scope s;
    symtab->Push(&s);
    symtab->global = false;

    llvm::Function *f = irgen->GetFunction();
    llvm::LLVMContext *context = irgen->GetContext();

    llvm::Value *e = expr->Emit();

    llvm::BasicBlock *fb = llvm::BasicBlock::Create(*context, "footer", f);
    symtab->breakBlock = fb;

    llvm::BasicBlock *dflt = llvm::BasicBlock::Create(*context, "default", f);
    List<llvm::BasicBlock*> *bbList = new List<llvm::BasicBlock*>;
    List<Stmt*> *caseList = new List<Stmt*>;

    for(int i = 0; i < cases->NumElements(); i++) {
        if(dynamic_cast<Case*>(cases->Nth(i)) != NULL) {
            llvm::BasicBlock* cs = llvm::BasicBlock::Create(*context, "case", f);
            bbList->Append(cs);
            caseList->Append(cases->Nth(i));
        }
        else if(dynamic_cast<Default*>(cases->Nth(i)) != NULL){
            bbList->Append(dflt);
            caseList->Append(cases->Nth(i));
        }
    }

    llvm::SwitchInst *sw = llvm::SwitchInst::Create(e, fb, cases->NumElements(), irgen->GetBasicBlock());
    llvm::BranchInst::Create(dflt, irgen->GetBasicBlock());

    BreakStmt *b = NULL;
    int count = 0;
    for(int i = 0; i < bbList->NumElements(); i++) {
        llvm::BasicBlock* cb = bbList->Nth(i);
        cb->moveAfter(irgen->GetBasicBlock());
        irgen->SetBasicBlock(cb);
        Stmt *s = caseList->Nth(i);
        if(dynamic_cast<Case*>(s) != NULL) {
            Case *c;
            c = dynamic_cast<Case*>(s);
            llvm::Value *labelVal = c->GetLabel()->Emit();
            sw->addCase(llvm::cast<llvm::ConstantInt>(labelVal), cb);
            c->Emit(); 
        }
        else if(dynamic_cast<Default*>(caseList->Nth(i)) != NULL) {
            dflt->moveAfter(irgen->GetBasicBlock());
            irgen->SetBasicBlock(dflt);
            caseList->Nth(i)->Emit();
        }
        b = dynamic_cast<BreakStmt*>(cases->Nth(count++));
        if(b != NULL) {
            b->Emit();
            count++;
        }
    }
    fb->moveAfter(irgen->GetBasicBlock());
    if(pred_begin(fb) == pred_end(fb))
        new llvm::UnreachableInst(*context, fb);
    else
        irgen->SetBasicBlock(fb);

    symtab->Pop();
    return NULL;
}

llvm::Value *StmtBlock::Emit() {
    llvm::Value *v = NULL;
    for(int i = 0; i < decls->NumElements(); i++) {
        decls->Nth(i)->Emit();
    }
    for(int i = 0; i < stmts->NumElements(); i++) {
        stmts->Nth(i)->Emit();
    }
    return NULL;
}

llvm::Value *DeclStmt::Emit() {
    llvm::Value* val = decl->Emit();
    return val;
}

llvm::Value *BreakStmt::Emit() {
    llvm::BasicBlock *cb = irgen->GetBasicBlock();
    llvm::BranchInst::Create(symtab->breakBlock , cb);
    llvm::LLVMContext *context = irgen->GetContext();
    return NULL;
}

llvm::Value *ContinueStmt::Emit() {
    return NULL;
}

llvm::Value *ConditionalStmt::Emit() {
    return NULL;
}
 
llvm::Value *LoopStmt::Emit() {
    return NULL;
}

llvm::Value *Default::Emit() {
    llvm::Function *f = irgen->GetFunction();
    llvm::LLVMContext *context = irgen->GetContext();
    stmt->Emit();
    return NULL;
}

llvm::Value *Case::Emit() {
    llvm::Function *f = irgen->GetFunction();
    llvm::LLVMContext *context = irgen->GetContext();
    stmt->Emit();
    return NULL;
}

llvm::Value *ReturnStmt::Emit() {
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    llvm::LLVMContext *context = irgen->GetContext(); 
    llvm::Value *val = expr->Emit();
    return llvm::ReturnInst::Create(*context, val, bb);
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

DeclStmt::DeclStmt(Decl *d) {
    Assert(d != NULL);
    (decl=d)->SetParent(this);
}

void DeclStmt::PrintChildren(int indentLevel) {
    decl->Print(indentLevel+1);
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && b != NULL);
    (init=i)->SetParent(this);
    step = s;
    if ( s )
      (step=s)->SetParent(this);
}

void ForStmt::PrintChildren(int indentLevel) {
    init->Print(indentLevel+1, "(init) ");
    test->Print(indentLevel+1, "(test) ");
    if ( step )
      step->Print(indentLevel+1, "(step) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::PrintChildren(int indentLevel) {
    if (test) test->Print(indentLevel+1, "(test) ");
    if (body) body->Print(indentLevel+1, "(then) ");
    if (elseBody) elseBody->Print(indentLevel+1, "(else) ");
}

ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    expr = e;
    if (e != NULL) expr->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    if ( expr ) 
      expr->Print(indentLevel+1);
}

SwitchLabel::SwitchLabel(Expr *l, Stmt *s) {
    Assert(l != NULL && s != NULL);
    (label=l)->SetParent(this);
    (stmt=s)->SetParent(this);
}

SwitchLabel::SwitchLabel(Stmt *s) {
    Assert(s != NULL);
    label = NULL;
    (stmt=s)->SetParent(this);
}

void SwitchLabel::PrintChildren(int indentLevel) {
    if (label) label->Print(indentLevel+1);
    if (stmt)  stmt->Print(indentLevel+1);
}

SwitchStmt::SwitchStmt(Expr *e, List<Stmt *> *c, Default *d) {
    Assert(e != NULL && c != NULL && c->NumElements() != 0 );
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    def = d;
    if (def) def->SetParent(this);
}

void SwitchStmt::PrintChildren(int indentLevel) {
    if (expr) expr->Print(indentLevel+1);
    if (cases) cases->PrintAll(indentLevel+1);
    if (def) def->Print(indentLevel+1);
}


