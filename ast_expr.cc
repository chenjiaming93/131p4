/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "symtable.h"

IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}
void IntConstant::PrintChildren(int indentLevel) { 
    printf("%d", value);
}
llvm::Value *IntConstant::Emit() {
    return llvm::ConstantInt::get(irgen->GetIntType(), value);
}

FloatConstant::FloatConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}
void FloatConstant::PrintChildren(int indentLevel) { 
    printf("%g", value);
}
llvm::Value *FloatConstant::Emit() {
    return llvm::ConstantFP::get(irgen->GetFloatType(), value);
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}
void BoolConstant::PrintChildren(int indentLevel) { 
    printf("%s", value ? "true" : "false");
}
llvm::Value *BoolConstant::Emit() {
    return llvm::ConstantInt::get(irgen->GetBoolType(), value);
}

VarExpr::VarExpr(yyltype loc, Identifier *ident) : Expr(loc) {
    Assert(ident != NULL);
    this->id = ident;
}

void VarExpr::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
}

llvm::Value *VarExpr::Emit() {
    llvm::Value *v = symtab->LookUpValue(GetIdentifier()->GetName());
    llvm::Twine *twine = new llvm::Twine(this->id->GetName());
    llvm::Value *in = new llvm::LoadInst(v, *twine, irgen->GetBasicBlock());
    return in;
}

llvm::Value *ArithmeticExpr::Emit() {
    Operator *op = this->op;

    if(this->left == NULL && this->right != NULL) {
        llvm::Value *rv = this->right->Emit();
        llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(rv);
        llvm::Value *loc = ld->getPointerOperand();

        llvm::BasicBlock *bb = irgen->GetBasicBlock();
        llvm::Value *val = llvm::ConstantInt::get(irgen->GetIntType(), 1);

        if(op->IsOp("++")) {
            llvm::Value *incre = llvm::BinaryOperator::CreateAdd(rv, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(incre, loc, bb);
            return incre;
        }
        else if(op->IsOp("--")) {
            llvm::Value *decre = llvm::BinaryOperator::CreateSub(rv, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(decre, loc, bb);
            return decre;
        }
    }

    if (this->left != NULL && this->right != NULL) {
        llvm::BasicBlock *bb = irgen->GetBasicBlock();
        llvm::Value *rhs = this->right->Emit();
        llvm::Value *lhs = this->left->Emit();

        if(op->IsOp("+")) {
            llvm::Value *sum = llvm::BinaryOperator::CreateAdd(lhs, rhs, "", bb);
            return sum;
        }
        else if(op->IsOp("*")) {
            llvm::Value *vec;
            llvm::Value *fp;
            if(lhs->getType() == irgen->GetType(Type::vec2Type)) {
                vec = lhs;
                fp = rhs;
            }
            else if(rhs->getType() == irgen->GetType(Type::vec2Type)) {
                vec = rhs;
                fp = lhs;
            }
            else {
                vec = NULL;
                fp = NULL;
            }

            if(vec == NULL) {
                llvm::Value *mul = llvm::BinaryOperator::CreateMul(lhs, rhs, "", bb);
                return mul;
            }
            else {
                llvm::Value *v = llvm::Constant::getNullValue(irgen->GetType(Type::vec2Type));
                llvm::LoadInst *l = llvm::cast<llvm::LoadInst>(vec);
                llvm::VectorType *vector = llvm::cast<llvm::VectorType>(l);
                for(int i = 0; i < vector->getNumElements(); i++) {
                    llvm::Value *num = llvm::ConstantInt::get(irgen->GetIntType(), i);
                    llvm::Value *elem = llvm::ExtractElementInst::Create(vec, num);
                    llvm::Value *mult = llvm::BinaryOperator::CreateFMul(elem, fp);
                    llvm::InsertElementInst::Create(v, mult, num, "", bb);
                }
                llvm::Value *ptr = l->getPointerOperand();
                llvm::Value *in = new llvm::StoreInst(v, ptr, bb);
                return v;
            }
        }
    }
    return NULL;
}

llvm::Value *RelationalExpr::Emit() {
    Operator *op = this->op;
    llvm::Value *lhs = left->Emit();
    llvm::Value *rhs = right->Emit();
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    llvm::CmpInst::Predicate pred;

    if((lhs->getType() == irgen->GetType(Type::floatType)) && (rhs->getType() == irgen->GetType(Type::floatType))) {
        if(op->IsOp(">"))
            pred = llvm::CmpInst::FCMP_OGT;
        else if(op->IsOp("<"))
            pred = llvm::CmpInst::FCMP_OLT;
        else if(op->IsOp(">="))
            pred = llvm::CmpInst::FCMP_OGE;
        else if(op->IsOp("<="))
            pred = llvm::CmpInst::FCMP_OLE;
        else if(op->IsOp("=="))
            pred = llvm::CmpInst::FCMP_OEQ;
        else if(op->IsOp("!="))
            pred = llvm::CmpInst::FCMP_ONE;

        llvm::Value *v = llvm::CmpInst::Create(llvm::CmpInst::FCmp, pred, lhs, rhs, "", bb);
        return v;
    }

    if((lhs->getType() == irgen->GetType(Type::intType)) && (rhs->getType() == irgen->GetType(Type::intType))) {
        if(op->IsOp(">"))
            pred = llvm::CmpInst::ICMP_SGT;
        else if(op->IsOp("<"))
            pred = llvm::CmpInst::ICMP_SLT;
        else if(op->IsOp(">="))
            pred = llvm::CmpInst::ICMP_SGE;
        else if(op->IsOp("<="))
            pred = llvm::CmpInst::ICMP_SLE;
        else if(op->IsOp("=="))
            pred = llvm::CmpInst::ICMP_EQ;
        else if(op->IsOp("!="))
            pred = llvm::CmpInst::ICMP_NE;

        llvm::Value *v = llvm::CmpInst::Create(llvm::CmpInst::ICmp, pred, lhs, rhs, "", bb);
        return v;
    }
    return NULL;
}

llvm::Value *AssignExpr::Emit() {
    Operator *op = this->op;
    llvm::Value *lv = this->left->Emit();
    llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(lv);
    llvm::Value *loc = ld->getPointerOperand();
    llvm::Value *rv = this->right->Emit();
    llvm::BasicBlock *bb = irgen->GetBasicBlock();

    if(op->IsOp("=")) {
        llvm::Value* in = new llvm::StoreInst(rv, loc, bb);
    }

    else if((lv->getType() == irgen->GetType(Type::floatType)) && (rv->getType() == irgen->GetType(Type::floatType))) {
        if(op->IsOp("+=")) {
            llvm::Value *add = llvm::BinaryOperator::CreateFAdd(lv, rv, "plusequal", bb);
            llvm::Value *in = new llvm::StoreInst(add, loc, bb);
        }
        else if(op->IsOp("-=")) {
            llvm::Value *sub = llvm::BinaryOperator::CreateFSub(lv, rv, "minusequal", bb);
            llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
        }
        else if(op->IsOp("*=")) {
            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(lv, rv, "mulequal", bb);
            llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
        }
        else if(op->IsOp("/=")) {
            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(lv, rv, "divequal", bb);
            llvm::Value *in = new llvm::StoreInst(div, loc, bb);
        }
    }

    else if((lv->getType() == irgen->GetType(Type::intType)) && (rv->getType() == irgen->GetType(Type::intType))) {
        if(op->IsOp("+=")) {
            llvm::Value *add = llvm::BinaryOperator::CreateAdd(lv, rv, "plusequal", bb);
            llvm::Value *in = new llvm::StoreInst(add, loc, bb);
        }
        else if(op->IsOp("-=")) {
            llvm::Value *sub = llvm::BinaryOperator::CreateSub(lv, rv, "minusequal", bb);
            llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
        }
        else if(op->IsOp("*=")) {
            llvm::Value *mul = llvm::BinaryOperator::CreateMul(lv, rv, "mulequal", bb);
            llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
        }
        else if(op->IsOp("/=")) {
            llvm::Value *div = llvm::BinaryOperator::CreateSDiv(lv, rv, "divequal", bb);
            llvm::Value *in = new llvm::StoreInst(div, loc, bb);
        }
    }
    return ld;
}

llvm::Value *PostfixExpr::Emit() {
    Operator *op = this->op;
    llvm::Value *v = left->Emit();
    llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(v);
    llvm::Value *loc = ld->getPointerOperand();

    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    llvm::Value *val = llvm::ConstantInt::get(irgen->GetIntType(), 1);

    if(v->getType() == irgen->GetType(Type::intType)) {
        if(op->IsOp("++")) {
            llvm::Value *dec = llvm::BinaryOperator::CreateAdd(v, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
        }
        else if(op->IsOp("--")) {
            llvm::Value *dec = llvm::BinaryOperator::CreateSub(v, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
        }
    }

    else if(v->getType() == irgen->GetType(Type::floatType)){
        if(op->IsOp("++")){
            llvm::Value *dec = llvm::BinaryOperator::CreateFAdd(v, val, "", bb);
            llvm::Value* in = new llvm::StoreInst(dec, loc, bb);
        }
        else if(op->IsOp("--")){
            llvm::Value *dec = llvm::BinaryOperator::CreateFSub(v, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
        }
    }
    return v;
}

llvm::Value *FieldAccess::Emit() {
    if(this->base != NULL) {
        llvm::Value *val = base->Emit();
        llvm::BasicBlock *bb = irgen->GetBasicBlock();
        llvm::Value *idx;
        string s = "x";

        if(this->field->GetName() == s)
            idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
        else
            idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);

        llvm::Value *v = llvm::ExtractElementInst::Create(val, idx, "", bb);
        return v;
    }
    return NULL;
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

void Operator::PrintChildren(int indentLevel) {
    printf("%s",tokenString);
}

bool Operator::IsOp(const char *op) const {
    return strcmp(tokenString, op) == 0;
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o) 
  : Expr(Join(l->GetLocation(), o->GetLocation())) {
    Assert(l != NULL && o != NULL);
    (left=l)->SetParent(this);
    (op=o)->SetParent(this);
}

void CompoundExpr::PrintChildren(int indentLevel) {
   if (left) left->Print(indentLevel+1);
   op->Print(indentLevel+1);
   if (right) right->Print(indentLevel+1);
}
   
ConditionalExpr::ConditionalExpr(Expr *c, Expr *t, Expr *f)
  : Expr(Join(c->GetLocation(), f->GetLocation())) {
    Assert(c != NULL && t != NULL && f != NULL);
    (cond=c)->SetParent(this);
    (trueExpr=t)->SetParent(this);
    (falseExpr=f)->SetParent(this);
}

void ConditionalExpr::PrintChildren(int indentLevel) {
    cond->Print(indentLevel+1, "(cond) ");
    trueExpr->Print(indentLevel+1, "(true) ");
    falseExpr->Print(indentLevel+1, "(false) ");
}
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::PrintChildren(int indentLevel) {
    base->Print(indentLevel+1);
    subscript->Print(indentLevel+1, "(subscript) ");
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}


void FieldAccess::PrintChildren(int indentLevel) {
    if (base) base->Print(indentLevel+1);
    field->Print(indentLevel+1);
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::PrintChildren(int indentLevel) {
   if (base) base->Print(indentLevel+1);
   if (field) field->Print(indentLevel+1);
   if (actuals) actuals->PrintAll(indentLevel+1, "(actuals) ");
}

