/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include <vector>
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
llvm::Value *VarExpr::EmitAddress(){
    llvm::Value *var=symtab->LookUpValue(id->GetName());
    return var;
}
llvm::Value *VarExpr::Emit() {
    llvm::Value *v = symtab->LookUpValue(GetIdentifier()->GetName());
    llvm::Twine *twine = new llvm::Twine(this->id->GetName());
    llvm::Value *in = new llvm::LoadInst(v, *twine, irgen->GetBasicBlock());
    return in;
}

llvm::Value *ArithmeticExpr::Emit() {
    Operator *op = this->op;    
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    FieldAccess* l = dynamic_cast<FieldAccess*>(left);
    FieldAccess* r = dynamic_cast<FieldAccess*>(right);
    char *swiz=NULL;
    char *rswiz=NULL;
    if(this->left == NULL && this->right != NULL) {
        llvm::Value *rhs = this->right->Emit();
        llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(rhs);
        llvm::Value *loc = ld->getPointerOperand();
        if(rhs->getType() == irgen->GetType(Type::intType)) {
            llvm::Value *val = llvm::ConstantInt::get(irgen->GetIntType(), 1);
            if(op->IsOp("++")) {
                llvm::Value *dec = llvm::BinaryOperator::CreateAdd(rhs, val, "", bb);
                llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
                return dec;
            }
            else {
                llvm::Value *dec = llvm::BinaryOperator::CreateSub(rhs, val, "", bb);
                llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
                return dec;
            }
        }
        if(!r && rhs->getType() == irgen->GetType(Type::floatType)) {
            llvm::Value *val = llvm::ConstantFP::get(irgen->GetFloatType(), 1.0);
            if(op->IsOp("++")) {
                llvm::Value *dec = llvm::BinaryOperator::CreateFAdd(rhs, val, "", bb);
                llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
                return dec;
            }
            else {
                llvm::Value *dec = llvm::BinaryOperator::CreateFSub(rhs, val, "", bb);
                llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
                return dec;
            }
        }
        else if(r) {
            llvm::Value *val = llvm::ConstantFP::get(irgen->GetFloatType(), 1.0);
            rswiz=r->GetField()->GetName();
            llvm::Value* ra=r->EmitAddress();
            llvm::Value *store;
            llvm::Constant *ridx;
            llvm::Value *lv;
            for(int i = 0; i<strlen(rswiz); i++) {
                if(rswiz[i] == 'x')
                    ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                else if(rswiz[i] == 'y')
                    ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                else if(rswiz[i] == 'z')
                    ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                else if(swiz[i] == 'w')
                    ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);                                      
                loc=new llvm::LoadInst(ra,"",bb);
                llvm::Value *extract=llvm::ExtractElementInst::Create(loc,ridx,"",bb);
                if(op->IsOp("++"))
                    lv = llvm::BinaryOperator::CreateFAdd(extract,val, "FA", bb);           
                else
                    lv = llvm::BinaryOperator::CreateFSub(extract,val, "FA", bb);
                store=llvm::InsertElementInst::Create(loc,lv,ridx,"",bb);               
                llvm::Value* result = new llvm::StoreInst(store,ra, "",bb); 
            }
            rhs=right->Emit();
            return rhs;
        }
    }

    if(this->left != NULL && this->right != NULL) {
        llvm::Value *rhs = this->right->Emit();
        llvm::Value *lhs = this->left->Emit();
        int rlen=0;
        int llen=0;
        if(l)
            llen=strlen(l->GetField()->GetName());
        if(r)
            rlen=strlen(r->GetField()->GetName());
        llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(rhs);
        llvm::Value *loc = ld->getPointerOperand();

        if(op->IsOp("+")) {
            if(rhs->getType() == irgen->GetType(Type::intType)) {
                llvm::Value *result = llvm::BinaryOperator::CreateAdd(lhs, rhs, "", bb);
                return result;
            }
            else if (rhs->getType() == irgen->GetType(Type::floatType)&&lhs->getType() == irgen->GetType(Type::floatType)) {
                llvm::Value *result = llvm::BinaryOperator::CreateFAdd(lhs, rhs, "", bb);
                return result;      
            }
            else {
                if (rhs->getType() != irgen->GetType(Type::floatType)&&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *result = llvm::BinaryOperator::CreateFAdd(lhs, rhs, "", bb);
                    return result;
                }
                else if (rhs->getType() == irgen->GetType(Type::floatType) &&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *undefVal = llvm::UndefValue::get(lhs->getType());
                    llvm::Constant *idx;
                    if(!l) {
                        if(lhs->getType()==irgen->GetType(Type::vec2Type))
                            llen=2;
                        else if (lhs->getType()==irgen->GetType(Type::vec3Type))
                            llen=3;
                        else if (lhs->getType()==irgen->GetType(Type::vec4Type))
                            llen=4;
                    }
                    for (int i=0;i<llen;i++) {
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                        undefVal=llvm::InsertElementInst::Create(undefVal,rhs,idx,"newelement",bb);
                    }
                    llvm::Value *result = llvm::BinaryOperator::CreateFAdd(lhs,undefVal, "", bb);
                    return result;
                }
                else {
                    llvm::Value *undefVal = llvm::UndefValue::get(rhs->getType());
                    llvm::Constant *idx;
                    if(!r) {
                        if(rhs->getType()==irgen->GetType(Type::vec2Type))
                            rlen=2;
                        else if (rhs->getType()==irgen->GetType(Type::vec3Type))
                            rlen=3;
                        else if (rhs->getType()==irgen->GetType(Type::vec4Type))
                            rlen=4;
                    }
                    for (int i=0;i<rlen;i++) {
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                        undefVal=llvm::InsertElementInst::Create(undefVal,lhs,idx,"newelement",bb);
                    }
                    llvm::Value *result = llvm::BinaryOperator::CreateFAdd(rhs,undefVal, "", bb);
                    return result;
                }
            }
        }

        else if(op->IsOp("*")) {
            if(rhs->getType() == irgen->GetType(Type::intType)) {
                llvm::Value *result = llvm::BinaryOperator::CreateMul(lhs, rhs, "", bb);
                return result;
            }
            else if (rhs->getType() == irgen->GetType(Type::floatType)&&lhs->getType() == irgen->GetType(Type::floatType)) {       
                llvm::Value *result = llvm::BinaryOperator::CreateFMul(lhs, rhs, "", bb);
                return result;      
            }
            else {
                if (rhs->getType() != irgen->GetType(Type::floatType)&&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *result = llvm::BinaryOperator::CreateFMul(lhs, rhs, "", bb);
                    return result;
                }
                else if (rhs->getType() == irgen->GetType(Type::floatType) &&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *undefVal = llvm::UndefValue::get(lhs->getType());
                    llvm::Constant *idx;
                    if(!l) {
                        if(lhs->getType()==irgen->GetType(Type::vec2Type))
                            llen=2;
                        else if (lhs->getType()==irgen->GetType(Type::vec3Type))
                            llen=3;
                        else if (lhs->getType()==irgen->GetType(Type::vec4Type))
                            llen=4;
                    }
                    for(int i=0;i<llen;i++) {
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                        undefVal=llvm::InsertElementInst::Create(undefVal,rhs,idx,"newelement",bb);
                    }
                    llvm::Value *result = llvm::BinaryOperator::CreateFMul(lhs,undefVal, "", bb);
                    return result;
                }
                else {
                    llvm::Value *undefVal = llvm::UndefValue::get(rhs->getType());
                    llvm::Constant *idx;
                    if(!r) {
                        if (rhs->getType()==irgen->GetType(Type::vec2Type))
                            rlen=2;
                        else if (rhs->getType()==irgen->GetType(Type::vec3Type))
                            rlen=3;
                        else if (rhs->getType()==irgen->GetType(Type::vec4Type))
                            rlen=4;
                    }
                    for (int i=0;i<rlen;i++) {
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                        undefVal=llvm::InsertElementInst::Create(undefVal,lhs,idx,"newelement",bb);
                    }
                    llvm::Value *result = llvm::BinaryOperator::CreateFMul(undefVal,rhs, "", bb);
                    return result;
                }
            }
        }

        else if (op->IsOp("-")) {
            if(rhs->getType() == irgen->GetType(Type::intType)){
                llvm::Value *result = llvm::BinaryOperator::CreateSub(lhs, rhs, "", bb);
                return result;
            }
            else if (rhs->getType() == irgen->GetType(Type::floatType)&&lhs->getType() == irgen->GetType(Type::floatType)) {       
                llvm::Value *result = llvm::BinaryOperator::CreateFSub(lhs, rhs, "", bb);
                return result;      
            }
            else {
                if (rhs->getType() != irgen->GetType(Type::floatType)&&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *result = llvm::BinaryOperator::CreateFSub(lhs, rhs, "", bb);
                    return result;
                }
                else if (rhs->getType() == irgen->GetType(Type::floatType) &&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *undefVal = llvm::UndefValue::get(lhs->getType());
                    llvm::Constant *idx;
                    if (!l) {
                        if(lhs->getType()==irgen->GetType(Type::vec2Type))
                            llen=2;
                        else if(lhs->getType()==irgen->GetType(Type::vec3Type))
                            llen=3;
                        else if(lhs->getType()==irgen->GetType(Type::vec4Type))
                            llen=4;
                    }
                    for(int i=0;i<llen;i++) {
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                        undefVal=llvm::InsertElementInst::Create(undefVal,rhs,idx,"newelement",bb);
                    }
                    llvm::Value *result = llvm::BinaryOperator::CreateFSub(lhs,undefVal, "", bb);
                    return result;
                }
                else {
                    llvm::Value *undefVal = llvm::UndefValue::get(rhs->getType());
                    llvm::Constant *idx;
                    if (!r){
                        if (rhs->getType()==irgen->GetType(Type::vec2Type))
                            rlen=2;
                        else if (rhs->getType()==irgen->GetType(Type::vec3Type))
                            rlen=3;
                        else if (rhs->getType()==irgen->GetType(Type::vec4Type))
                            rlen=4;
                    }
                    for (int i=0;i<rlen;i++) {
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                        undefVal=llvm::InsertElementInst::Create(undefVal,lhs,idx,"newelement",bb);
                    }
                    llvm::Value *result = llvm::BinaryOperator::CreateFSub(undefVal,rhs, "", bb);
                    return result;
                }
            }
        }

        else if (op->IsOp("/")) {
            if(rhs->getType() == irgen->GetType(Type::intType)){
                llvm::Value *result = llvm::BinaryOperator::CreateSDiv(lhs, rhs, "", bb);
                return result;
            }
            else if (rhs->getType() == irgen->GetType(Type::floatType)&&lhs->getType() == irgen->GetType(Type::floatType)) {       
                llvm::Value *result = llvm::BinaryOperator::CreateFDiv(lhs, rhs, "", bb);
                return result;      
            }
            else {
                if (rhs->getType() != irgen->GetType(Type::floatType)&&lhs->getType() != irgen->GetType(Type::floatType)) {
                    llvm::Value *result = llvm::BinaryOperator::CreateFDiv(lhs, rhs, "", bb);
                    return result;
                }
            else if (rhs->getType() == irgen->GetType(Type::floatType) &&lhs->getType() != irgen->GetType(Type::floatType)) {
                llvm::Value *undefVal = llvm::UndefValue::get(lhs->getType());
                llvm::Constant *idx;
                if (!l){
                    if (lhs->getType()==irgen->GetType(Type::vec2Type))
                        llen=2;
                    else if (lhs->getType()==irgen->GetType(Type::vec3Type))
                        llen=3;
                    else if (lhs->getType()==irgen->GetType(Type::vec4Type))
                        llen=4;
                }
                for (int i=0;i<llen;i++) {
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                    undefVal=llvm::InsertElementInst::Create(undefVal,rhs,idx,"newelement",bb);
                }
                llvm::Value *result = llvm::BinaryOperator::CreateFDiv(lhs,undefVal, "", bb);
                return result;
            }
            else {
                llvm::Value *undefVal = llvm::UndefValue::get(rhs->getType());
                llvm::Constant *idx;
                if (!r){
                    if (rhs->getType()==irgen->GetType(Type::vec2Type))
                        rlen=2;
                    else if (rhs->getType()==irgen->GetType(Type::vec3Type))
                        rlen=3;
                    else if (rhs->getType()==irgen->GetType(Type::vec4Type))
                        rlen=4;
                }
                for (int i=0;i<rlen;i++){
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), i);
                    undefVal=llvm::InsertElementInst::Create(undefVal,lhs,idx,"newelement",bb);
                }
                llvm::Value *result = llvm::BinaryOperator::CreateFDiv(undefVal,rhs, "", bb);
                return result;
            }
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
    llvm::CmpInst::Predicate pred = NULL;

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
    llvm::Value *store;
    llvm::Value *rv = this->right->Emit();
    llvm::Value *lv = this->left->Emit();
    llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(lv);
    llvm::Value *loc = ld->getPointerOperand();
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    FieldAccess* l=dynamic_cast<FieldAccess*>(left);
    FieldAccess* r=dynamic_cast<FieldAccess*>(right);
    char *swiz = NULL;
    char *rswiz = NULL;
    if(op->IsOp("=")) {
        if (l) {
            llvm::Value* la=l->EmitAddress();
            if (rv->getType() == irgen->GetType(Type::floatType)){
                swiz=l->GetField()->GetName();
                llvm::Constant *idx;
                for(int i=0;i<strlen(swiz); i++) {
                    if(swiz[i] == 'x')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                    else if(swiz[i] == 'y')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                    else if(swiz[i] == 'z')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                    else if(swiz[i] == 'w')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                    loc=new llvm::LoadInst(la,"",bb);
                    store=llvm::InsertElementInst::Create(loc,rv,idx,"",bb);
                    llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                } 
            }
            else{
                if (!r){
                    swiz = l->GetField()->GetName();
                    llvm::Constant *idx;
                    for(int i=0;i<strlen(swiz); i++) {
                        if(swiz[i] == 'x')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(swiz[i] == 'y')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(swiz[i] == 'z')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(swiz[i] == 'w')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        loc=new llvm::LoadInst(la,"",bb);
                        llvm::Constant* ridx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                        llvm::Value *extract=llvm::ExtractElementInst::Create(rv,ridx,"",bb);
                        store = llvm::InsertElementInst::Create(loc,extract,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                    }
                }
                else{
                    llvm::Value *ra = r->EmitAddress();
                    llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                    swiz = l->GetField()->GetName();
                    rswiz = r->GetField()->GetName();
                    llvm::Constant *idx;
                    llvm::Constant *ridx;
                    for(int i=0;i<strlen(swiz); i++) {
                        if(swiz[i] == 'x')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(swiz[i] == 'y')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(swiz[i] == 'z')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(swiz[i] == 'w')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        if(rswiz[i] == 'x')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(rswiz[i] == 'y')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(rswiz[i] == 'z')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(rswiz[i] == 'w')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        loc=new llvm::LoadInst(la,"",bb);
                        llvm::Value *extract=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                        store = llvm::InsertElementInst::Create(loc,extract,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                    }
                }
            }
        }   
        else {
           llvm::Value* in = new llvm::StoreInst(rv, loc, bb);
        }
    }
    if(op->IsOp("+=")) {
        if(!l&&(lv->getType() == irgen->GetType(Type::floatType))){
            if (r) {
                llvm::Value *ra = r->EmitAddress();
                rswiz = r->GetField()->GetName();
                llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                llvm::Constant *ridx;
                for(int i = 0;i<strlen(rswiz); i++) {
                    if(rswiz[i] == 'x')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                    else if(rswiz[i] == 'y')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                    else if(rswiz[i] == 'z')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                    else if(rswiz[i] == 'w')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                    llvm::Value *extract=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                    llvm::Value *add = llvm::BinaryOperator::CreateFAdd(lv, extract, "RFloatPlusEqual", bb);
                    llvm::Value *in = new llvm::StoreInst(add, loc, bb);
                }                           
            }
            else {
                llvm::Value *add = llvm::BinaryOperator::CreateFAdd(lv, rv, "LFloatPlusEqual", bb);
                llvm::Value *in = new llvm::StoreInst(add, loc, bb);            
            }
        }
        else if(lv->getType() == irgen->GetType(Type::intType)) {
            llvm::Value *add = llvm::BinaryOperator::CreateAdd(lv, rv, "IntPlusEqual", bb);
            llvm::Value *in = new llvm::StoreInst(add, loc, bb);    
        }
        else if(l) {
            llvm::Value* la=l->EmitAddress();

            if(r) {
                llvm::Value *ra = r->EmitAddress();
                llvm::Value *rloc = new llvm::LoadInst(ra,"",bb);
                swiz = l->GetField()->GetName();
                rswiz = r->GetField()->GetName();
                llvm::Constant *idx;
                llvm::Constant *ridx;
                for(int i=0;i<strlen(swiz); i++) {
                    int j=0;
                    if (strlen(rswiz)>1)
                        j=i;
                    if(swiz[i] == 'x')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                    else if(swiz[i] == 'y')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                    else if(swiz[i] == 'z')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                    else if(swiz[i] == 'w')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                    if(rswiz[j] == 'x')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                    else if(rswiz[j] == 'y')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                    else if(rswiz[j] == 'z')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                    else if(rswiz[j] == 'w')
                        ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                    loc = new llvm::LoadInst(la,"",bb);
                    llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                    llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                    llvm::Value *add = llvm::BinaryOperator::CreateFAdd(extract1,extract2, "FAtoFA", bb);
                    loc = new llvm::LoadInst(la,"",bb);
                    store = llvm::InsertElementInst::Create(loc,add,idx,"",bb);
                    llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                }
            }
            else if(rv->getType() == irgen->GetType(Type::floatType)) {
                swiz=l->GetField()->GetName();
                llvm::Constant *idx;            
                for(int i=0;i<strlen(swiz); i++) {
                    if(swiz[i] == 'x')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                    else if(swiz[i] == 'y')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                    else if(swiz[i] == 'z')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                    else if(swiz[i] == 'w')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                    loc=new llvm::LoadInst(la,"",bb);
                    llvm::Value *extract=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                    llvm::Value *add = llvm::BinaryOperator::CreateFAdd(extract,rv, "FtoFA", bb);
                    store=llvm::InsertElementInst::Create(loc,add,idx,"",bb);
                    llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                }
            }
            else {
                swiz = l->GetField()->GetName();
                llvm::Constant *idx;
                for(int i=0;i<strlen(swiz); i++) {
                    if(swiz[i] == 'x')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                    else if(swiz[i] == 'y')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                    else if(swiz[i] == 'z')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                    else if(swiz[i] == 'w')
                        idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        loc=new llvm::LoadInst(la,"",bb);
                    llvm::Constant* ridx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                    llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                    llvm::Value *extract1=llvm::ExtractElementInst::Create(rv,ridx,"",bb);
                    llvm::Value *add = llvm::BinaryOperator::CreateFAdd(extract1,extract2, "FAtoFA", bb);
                    loc=new llvm::LoadInst(la,"",bb);
                    store=llvm::InsertElementInst::Create(loc,add,idx,"",bb);
                    llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                }
            }
        }
        else {
            int veclen=0;
            if (lv->getType()==irgen->GetType(Type::vec2Type))
                veclen=2;
            else if (lv->getType()==irgen->GetType(Type::vec3Type))
                veclen=3;
            else if (lv->getType()==irgen->GetType(Type::vec4Type))
                veclen=4;
            if (r){
                llvm::Value *ra = r->EmitAddress();
                llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                rswiz=r->GetField()->GetName();
                llvm::Constant *ridx;
                if (strlen(rswiz)>1)
                    for(int i=0;i<strlen(rswiz); i++) {
                        if(rswiz[i] == 'x')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(rswiz[i] == 'y')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(rswiz[i] == 'z')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(rswiz[i] == 'w')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        lv=this->left->Emit();
                        llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                                llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                        llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);               
                        llvm::Value *add = llvm::BinaryOperator::CreateFAdd(extract1,extract2, "FAtoVec", bb);
                        store=llvm::InsertElementInst::Create(lv,add,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                    }
                    if (strlen(rswiz)==1) {
                        if(rswiz[0] == 'x')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(rswiz[0] == 'y')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(rswiz[0] == 'z')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(rswiz[0] == 'w')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        for(int i=0;i<veclen;i++) {
                            lv = this->left->Emit();
                            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);
                            llvm::Value *add = llvm::BinaryOperator::CreateFAdd(extract1,extract2, "FAtoVec", bb);
                            store=llvm::InsertElementInst::Create(lv,add,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                        }
                    }
                }
                else if(rv->getType() == irgen->GetType(Type::floatType)) {
                    for (int i=0;i<veclen;i++) {
                        lv=this->left->Emit();
                        llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                        llvm::Value *extract=llvm::ExtractElementInst::Create(lv,idx,"",bb);                
                        llvm::Value *add = llvm::BinaryOperator::CreateFAdd(extract,rv, "FAtoVec", bb);
                        store=llvm::InsertElementInst::Create(lv,add,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                    }
                }
                else {
                    llvm::Value *add = llvm::BinaryOperator::CreateFAdd(lv, rv, "VectoVec", bb);
                    llvm::Value *in = new llvm::StoreInst(add, loc, bb);
                }
            }
        }
        else if(op->IsOp("-=")) {
            if(!l && (lv->getType() == irgen->GetType(Type::floatType))) {
                if (r){
                    llvm::Value *ra = r->EmitAddress();
                    rswiz=r->GetField()->GetName();
                    llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                    llvm::Constant *ridx;
                    for(int i=0;i<strlen(rswiz); i++){
                        if(rswiz[i] == 'x')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(rswiz[i] == 'y')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(rswiz[i] == 'z')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(rswiz[i] == 'w')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        llvm::Value *extract=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                        llvm::Value *sub = llvm::BinaryOperator::CreateFSub(lv, extract, "RFloatPlusEqual", bb);
                        llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
                    }
                }
                else {
                    llvm::Value *sub = llvm::BinaryOperator::CreateFSub(lv, rv, "LFloatPlusEqual", bb);
                    llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
                }
            }
            else if(lv->getType() == irgen->GetType(Type::intType)) {
                llvm::Value *sub = llvm::BinaryOperator::CreateSub(lv, rv, "IntPlusEqual", bb);
                llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
            }
            else if(l) {
                llvm::Value* la = l->EmitAddress();
                if(r) {
                    llvm::Value *ra = r->EmitAddress();
                    llvm::Value *rloc = new llvm::LoadInst(ra,"",bb);
                    swiz=l->GetField()->GetName();
                    rswiz=r->GetField()->GetName();
                    llvm::Constant *idx;
                    llvm::Constant *ridx;
                    for(int i=0;i<strlen(swiz); i++) {
                        int j=0;
                        if (strlen(rswiz)>1)
                            j=i;
                        if(swiz[i] == 'x')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(swiz[i] == 'y')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(swiz[i] == 'z')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(swiz[i] == 'w')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        if(rswiz[j] == 'x')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(rswiz[j] == 'y')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(rswiz[j] == 'z')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(rswiz[j] == 'w')
                            ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        loc=new llvm::LoadInst(la,"",bb);
                        llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                        llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                        llvm::Value *sub = llvm::BinaryOperator::CreateFSub(extract2,extract1, "FAtoFA", bb);
                        loc = new llvm::LoadInst(la,"",bb);
                        store=llvm::InsertElementInst::Create(loc,sub,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                    }
                }
                else if (rv->getType() == irgen->GetType(Type::floatType)){
                    swiz=l->GetField()->GetName();
                    llvm::Constant *idx;
                    for(int i=0;i<strlen(swiz); i++) {
                        if(swiz[i] == 'x')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(swiz[i] == 'y')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(swiz[i] == 'z')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(swiz[i] == 'w')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        loc=new llvm::LoadInst(la,"",bb);
                        llvm::Value *extract=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                        llvm::Value *sub = llvm::BinaryOperator::CreateFSub(extract,rv, "FtoFA", bb);
                        store=llvm::InsertElementInst::Create(loc,sub,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                    }
                }
                else {
                    swiz=l->GetField()->GetName();
                    llvm::Constant *idx;
                    for(int i=0;i<strlen(swiz); i++) {
                        if(swiz[i] == 'x')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                        else if(swiz[i] == 'y')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                        else if(swiz[i] == 'z')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                        else if(swiz[i] == 'w')
                            idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                        loc=new llvm::LoadInst(la,"",bb);
                        llvm::Constant* ridx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                        llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                        llvm::Value *extract1=llvm::ExtractElementInst::Create(rv,ridx,"",bb);
                        llvm::Value *sub = llvm::BinaryOperator::CreateFSub(extract2,extract1, "FAtoFA", bb);
                        loc = new llvm::LoadInst(la,"",bb);
                        store = llvm::InsertElementInst::Create(loc,sub,idx,"",bb);
                        llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                    }
                } 
            }
            else{
                int veclen=0;
                if (lv->getType()==irgen->GetType(Type::vec2Type))
                    veclen=2;
                else if (lv->getType()==irgen->GetType(Type::vec3Type))
                    veclen=3;
                else if (lv->getType()==irgen->GetType(Type::vec4Type))
                    veclen=4;
                if(r) {
                    llvm::Value *ra = r->EmitAddress();
                    llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                    rswiz=r->GetField()->GetName();
                    llvm::Constant *ridx;
                    if (strlen(rswiz)>1)
                        for(int i=0;i<strlen(rswiz); i++) {
                            if(rswiz[i] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[i] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[i] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[i] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            lv=this->left->Emit();
                            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);               
                            llvm::Value *sub = llvm::BinaryOperator::CreateFSub(extract2,extract1, "FAtoVec", bb);
                            store=llvm::InsertElementInst::Create(lv,sub,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                        }
                        if (strlen(rswiz)==1) {
                            if(rswiz[0] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[0] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[0] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[0] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            for (int i=0;i<veclen;i++) {
                                lv=this->left->Emit();
                                llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                                llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                                llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);               
                                llvm::Value *sub = llvm::BinaryOperator::CreateFSub(extract2,extract1, "FAtoVec", bb);
                                store=llvm::InsertElementInst::Create(lv,sub,idx,"",bb);
                                llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                            }
                        }
                    }
                    else if (rv->getType() == irgen->GetType(Type::floatType)) {
                        for (int i=0;i<veclen;i++) {
                            lv=this->left->Emit();
                            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract=llvm::ExtractElementInst::Create(lv,idx,"",bb);
                            llvm::Value *sub = llvm::BinaryOperator::CreateFSub(extract,rv, "FAtoVec", bb);
                            store=llvm::InsertElementInst::Create(lv,sub,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                        }
                    }
                    else {
                        llvm::Value *sub = llvm::BinaryOperator::CreateFSub(lv, rv, "VectoVec", bb);
                        llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
                    }
                }
            }
            else if(op->IsOp("*=")) {
                if(!l&&(lv->getType() == irgen->GetType(Type::floatType))){
                    if(r) {
                        llvm::Value *ra = r->EmitAddress();
                        rswiz=r->GetField()->GetName();
                        llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                        llvm::Constant *ridx;
                        for(int i=0;i<strlen(rswiz); i++) {
                            if(rswiz[i] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[i] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[i] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[i] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);

                            llvm::Value *extract=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(lv, extract, "RFloatPlusEqual", bb);
                            llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
                        }
                    }
                    else {
                        llvm::Value *mul = llvm::BinaryOperator::CreateFMul(lv, rv, "LFloatPlusEqual", bb);
                        llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
                    }
                }
                else if(lv->getType() == irgen->GetType(Type::intType)) {
                    llvm::Value *mul = llvm::BinaryOperator::CreateMul(lv, rv, "IntPlusEqual", bb);
                    llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
                }
                else if (l){
                    llvm::Value* la=l->EmitAddress();
                    if (r){
                        llvm::Value *ra = r->EmitAddress();
                        llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                        swiz=l->GetField()->GetName();
                        rswiz=r->GetField()->GetName();
                        llvm::Constant *idx;
                        llvm::Constant *ridx;
                        for(int i=0;i<strlen(swiz); i++) {
                            int j=0;
                            if (strlen(rswiz)>1)
                                j=i;
                            if(swiz[i] == 'x')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(swiz[i] == 'y')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(swiz[i] == 'z')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(swiz[i] == 'w')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            if(rswiz[j] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[j] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[j] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[j] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            loc=new llvm::LoadInst(la,"",bb);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(extract1,extract2, "FAtoFA", bb);
                            loc=new llvm::LoadInst(la,"",bb);
                            store=llvm::InsertElementInst::Create(loc,mul,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                        }
                    }
                    else if (rv->getType() == irgen->GetType(Type::floatType)){
                        swiz=l->GetField()->GetName();
                        llvm::Constant *idx;
                        for(int i=0;i<strlen(swiz); i++) {
                            if(swiz[i] == 'x')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(swiz[i] == 'y')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(swiz[i] == 'z')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(swiz[i] == 'w')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            loc=new llvm::LoadInst(la,"",bb);
                            llvm::Value *extract=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(extract,rv, "FtoFA", bb);
                            store=llvm::InsertElementInst::Create(loc,mul,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                        }
                    }
                    else {
                        swiz=l->GetField()->GetName();
                        llvm::Constant *idx;
                        for(int i=0;i<strlen(swiz); i++) {
                            if(swiz[i] == 'x')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(swiz[i] == 'y')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(swiz[i] == 'z')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(swiz[i] == 'w')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            loc=new llvm::LoadInst(la,"",bb);
                            llvm::Constant* ridx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rv,ridx,"",bb);
                            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(extract1,extract2, "FAtoFA", bb);
                            loc=new llvm::LoadInst(la,"",bb);
                            store=llvm::InsertElementInst::Create(loc,mul,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                    }
                }
            }
            else{
                int veclen=0;
                if (lv->getType()==irgen->GetType(Type::vec2Type))
                    veclen=2;
                else if (lv->getType()==irgen->GetType(Type::vec3Type))
                    veclen=3;
                else if (lv->getType()==irgen->GetType(Type::vec4Type))
                    veclen=4;
                if (r){
                    llvm::Value *ra = r->EmitAddress();
                    llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                    rswiz=r->GetField()->GetName();
                    llvm::Constant *ridx;
                    if (strlen(rswiz)>1)
                        for(int i=0;i<strlen(rswiz); i++) {
                            if(rswiz[i] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[i] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[i] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[i] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            lv=this->left->Emit();
                            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);               
                            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(extract1,extract2, "FAtoVec", bb);
                            store=llvm::InsertElementInst::Create(lv,mul,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                        }
                        if (strlen(rswiz)==1) {
                            if(rswiz[0] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[0] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[0] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[0] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            for (int i=0;i<veclen;i++) {
                                lv=this->left->Emit();
                                llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                                llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                                llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);
                                llvm::Value *mul = llvm::BinaryOperator::CreateFMul(extract1,extract2, "FAtoVec", bb);
                                store=llvm::InsertElementInst::Create(lv,mul,idx,"",bb);
                                llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                            }
                        }
                    }
                    else if (rv->getType() == irgen->GetType(Type::floatType)){
                        for (int i=0;i<veclen;i++) {
                            lv=this->left->Emit();
                            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract=llvm::ExtractElementInst::Create(lv,idx,"",bb);                
                            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(extract,rv, "FAtoVec", bb);
                            store=llvm::InsertElementInst::Create(lv,mul,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                        }
                    }
                    else {
                        llvm::Value *mul = llvm::BinaryOperator::CreateFMul(lv, rv, "VectoVec", bb);
                        llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
                    }
                }
            }
            else if(op->IsOp("/=")) {
                if(!l&&(lv->getType() == irgen->GetType(Type::floatType))){
                    if (r){
                        llvm::Value *ra = r->EmitAddress();
                        rswiz=r->GetField()->GetName();
                        llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                        llvm::Constant *ridx;
                        for(int i=0;i<strlen(rswiz); i++){
                            if(rswiz[i] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[i] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[i] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[i] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);

                            llvm::Value *extract=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(lv, extract, "RFloatPlusEqual", bb);
                            llvm::Value *in = new llvm::StoreInst(div, loc, bb);
                        }
                    }
                    else {
                        llvm::Value *div = llvm::BinaryOperator::CreateFDiv(lv, rv, "LFloatPlusEqual", bb);
                        llvm::Value *in = new llvm::StoreInst(div, loc, bb);
                    }
                }
                else if(lv->getType() == irgen->GetType(Type::intType)) {
                    llvm::Value *div = llvm::BinaryOperator::CreateSDiv(lv, rv, "IntPlusEqual", bb);
                    llvm::Value *in = new llvm::StoreInst(div, loc, bb);    
                }
                else if (l) {
                    llvm::Value* la=l->EmitAddress();
                    if (r){
                        llvm::Value *ra = r->EmitAddress();
                        llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                        swiz=l->GetField()->GetName();
                        rswiz=r->GetField()->GetName();
                        llvm::Constant *idx = NULL;
                        llvm::Constant *ridx = NULL;
                        for(int i=0;i<strlen(swiz); i++) {
                            int j=0;
                            if (strlen(rswiz)>1)
                                j=i;
                            if(swiz[i] == 'x')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(swiz[i] == 'y')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(swiz[i] == 'z')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(swiz[i] == 'w')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            if(rswiz[j] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[j] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[j] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[j] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            loc=new llvm::LoadInst(la,"",bb);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(extract2,extract1, "FAtoFA", bb);
                            loc=new llvm::LoadInst(la,"",bb);
                            store=llvm::InsertElementInst::Create(loc,div,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                        }
                    }
                    else if (rv->getType() == irgen->GetType(Type::floatType)){
                        swiz=l->GetField()->GetName();
                        llvm::Constant *idx;
                        for(int i=0;i<strlen(swiz); i++) {
                            if(swiz[i] == 'x')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(swiz[i] == 'y')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(swiz[i] == 'z')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(swiz[i] == 'w')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            loc=new llvm::LoadInst(la,"",bb);
                            llvm::Value *extract=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(extract,rv, "FtoFA", bb);
                            store=llvm::InsertElementInst::Create(loc,div,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                        }
                    }
                    else {
                        swiz=l->GetField()->GetName();
                        llvm::Constant *idx;
                        for(int i=0;i<strlen(swiz); i++) {
                            if(swiz[i] == 'x')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(swiz[i] == 'y')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(swiz[i] == 'z')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(swiz[i] == 'w')
                                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            loc=new llvm::LoadInst(la,"",bb);
                            llvm::Constant* ridx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(loc,idx,"",bb);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rv,ridx,"",bb);
                            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(extract2,extract1, "FAtoFA", bb);
                            loc=new llvm::LoadInst(la,"",bb);
                            store=llvm::InsertElementInst::Create(loc,div,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
                        }
                    }
                }
                else{
                    int veclen=0;
                    if (lv->getType()==irgen->GetType(Type::vec2Type))
                        veclen=2;
                    else if (lv->getType()==irgen->GetType(Type::vec3Type))
                        veclen=3;
                    else if (lv->getType()==irgen->GetType(Type::vec4Type))
                        veclen=4;
                    if (r){
                        llvm::Value *ra = r->EmitAddress();
                        llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
                        rswiz=r->GetField()->GetName();
                        llvm::Constant *ridx;
                    if (strlen(rswiz)>1)
                        for(int i=0;i<strlen(rswiz); i++) {
                            if(rswiz[i] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[i] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[i] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[i] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            lv=this->left->Emit();
                            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                            llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                            llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);               
                            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(extract2,extract1, "FAtoVec", bb);
                            store=llvm::InsertElementInst::Create(lv,div,idx,"",bb);
                            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                        }
                        if (strlen(rswiz)==1) {
                            if(rswiz[0] == 'x')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                            else if(rswiz[0] == 'y')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                            else if(rswiz[0] == 'z')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                            else if(rswiz[0] == 'w')
                                ridx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                            for (int i=0;i<veclen;i++) {
                                lv=this->left->Emit();
                                llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                                llvm::Value *extract1=llvm::ExtractElementInst::Create(rloc,ridx,"",bb);
                                llvm::Value *extract2=llvm::ExtractElementInst::Create(lv,idx,"",bb);
                                llvm::Value *div = llvm::BinaryOperator::CreateFDiv(extract2,extract1, "FAtoVec", bb);
                                store=llvm::InsertElementInst::Create(lv,div,idx,"",bb);
                                llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                            }
                        }
                    }
            else if (rv->getType() == irgen->GetType(Type::floatType)) {
                for (int i=0;i<veclen;i++) {
                    lv=this->left->Emit();
                    llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
                    llvm::Value *extract=llvm::ExtractElementInst::Create(lv,idx,"",bb);                
                    llvm::Value *div = llvm::BinaryOperator::CreateFDiv(extract,rv, "FAtoVec", bb);
                    store=llvm::InsertElementInst::Create(lv,div,idx,"",bb);
                    llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
                }
            }

            else {
                llvm::Value *div = llvm::BinaryOperator::CreateFDiv(lv, rv, "VectoVec", bb);
                llvm::Value *in = new llvm::StoreInst(div, loc, bb);
            }
        }
    }
    return rv;
}

llvm::Value *PostfixExpr::Emit() {
    Operator *op = this->op;
    llvm::Value *v = left->Emit();
    char *swiz = NULL;
    llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(v);
    llvm::Value *loc = ld->getPointerOperand();
    FieldAccess* l = dynamic_cast<FieldAccess*>(left);
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    llvm::Value *val = llvm::ConstantInt::get(irgen->GetIntType(), 1);

    if(v->getType() == irgen->GetType(Type::intType)) {
        if(op->IsOp("++")) {
            llvm::Value *dec = llvm::BinaryOperator::CreateAdd(v, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
        }
        else  {
            llvm::Value *dec = llvm::BinaryOperator::CreateSub(v, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
        }
    }
    else if(!l&&v->getType() == irgen->GetType(Type::floatType)){
        llvm::Value *val = llvm::ConstantFP::get(irgen->GetFloatType(), 1.0);
        if(op->IsOp("++")) {
        
            llvm::Value *dec = llvm::BinaryOperator::CreateFAdd(v, val, "", bb);
            llvm::Value* in = new llvm::StoreInst(dec, loc, bb);
        }
        else {
            llvm::Value *dec = llvm::BinaryOperator::CreateFSub(v, val, "", bb);
            llvm::Value *in = new llvm::StoreInst(dec, loc, bb);
        }
    }
    else if (l) {
       llvm::Value *val = llvm::ConstantFP::get(irgen->GetFloatType(), 1.0);
       llvm::Value* la=l->EmitAddress();
       llvm::Value *store;
       llvm::Value *lv;
       swiz=l->GetField()->GetName();
       llvm::Constant *idx;
       for(int i=0;i<strlen(swiz); i++) {
            if(swiz[i] == 'x')
                idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
            else if(swiz[i] == 'y')
                idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
            else if(swiz[i] == 'z')
                idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
            else if(swiz[i] == 'w')
                idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);                                           
            loc = new llvm::LoadInst(la,"",bb);
            llvm::Value *extract=llvm::ExtractElementInst::Create(loc,idx,"",bb);
            if(op->IsOp("++"))
                lv = llvm::BinaryOperator::CreateFAdd(extract,val, "FA", bb);           
            else
                lv = llvm::BinaryOperator::CreateFSub(extract,val, "FA", bb);
            store = llvm::InsertElementInst::Create(loc,lv,idx,"",bb);
            llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
        }       
    }
    else {
        llvm::Value *val = llvm::ConstantFP::get(irgen->GetFloatType(), 1.0);
        int veclen=0;
        llvm::Value *lv;
        llvm::Value *store;
        if (v->getType()==irgen->GetType(Type::vec2Type))
            veclen=2;
        else if (v->getType()==irgen->GetType(Type::vec3Type))
            veclen=3;
        else if (v->getType()==irgen->GetType(Type::vec4Type))
            veclen=4;
        for (int i=0;i<veclen;i++) {
            v = this->left->Emit();
            llvm::Constant* idx=llvm::ConstantInt::get(irgen->GetIntType(), i);
            llvm::Value *extract=llvm::ExtractElementInst::Create(v,idx,"",bb);             
            if(op->IsOp("++"))
                lv = llvm::BinaryOperator::CreateFAdd(extract,val, "FA", bb);           
            else
                lv = llvm::BinaryOperator::CreateFSub(extract,val, "FA", bb);
            store = llvm::InsertElementInst::Create(v,lv,idx,"",bb);              
            llvm::Value* result = new llvm::StoreInst(store, loc, "",bb);
        }
    }
    return v;
}

llvm::Value *FieldAccess::EmitAddress() {
    VarExpr* isVar=dynamic_cast<VarExpr*>(base);
    if(isVar) {
        return isVar->EmitAddress();
    }
    FieldAccess* isFA=dynamic_cast<FieldAccess*>(base);
    {
        return isFA->EmitAddress();
    }
}

llvm::Value *FieldAccess::Emit() {
    if(this->base != NULL) {
        llvm::Value *val = base->Emit();
        llvm::BasicBlock *bb = irgen->GetBasicBlock();
        vector<llvm::Constant*> swizzles;

        if(this->field != NULL) {
            llvm::Constant *idx;
            char *c = this->field->GetName();
            for(char* i = c; *i; i++) {
                if(*i == 'x')
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), 0);
                else if(*i == 'y')
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), 1);
                else if(*i == 'z')
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), 2);
                else if(*i == 'w')
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), 3);
                else
                    idx = llvm::ConstantInt::get(irgen->GetIntType(), 100);
                swizzles.push_back(idx);
            }
            if(strlen(c) < 2)
                return llvm::ExtractElementInst::Create(val, idx, "", bb);

            llvm::ArrayRef<llvm::Constant*> swizzleArrayRef(swizzles);
            llvm::Constant *m = llvm::ConstantVector::get(swizzleArrayRef);
            llvm::Value *v = new llvm::ShuffleVectorInst(val, val, m, "", bb);
            return v;
        }
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

llvm::Value *EqualityExpr::Emit() {
    Operator *op = this->op;
    llvm::Value *lhs = left->Emit();
    llvm::Value *rhs = right->Emit();
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    llvm::CmpInst::Predicate pred = NULL;

    if((lhs->getType() == irgen->GetType(Type::floatType)) && (rhs->getType() == irgen->GetType(Type::floatType))) {
        if(op->IsOp("=="))
            pred = llvm::CmpInst::FCMP_OEQ;
        if(op->IsOp("!="))
            pred = llvm::CmpInst::FCMP_ONE;
        llvm::Value *v = llvm::CmpInst::Create(llvm::CmpInst::FCmp, pred, lhs, rhs, "", bb);
        return v;
    }    

    if((lhs->getType() == irgen->GetType(Type::intType)) && (rhs->getType() == irgen->GetType(Type::intType))) {
        if(op->IsOp("=="))
            pred = llvm::CmpInst::ICMP_EQ;
        else if(op->IsOp("!="))
            pred = llvm::CmpInst::ICMP_NE;
        llvm::Value *v = llvm::CmpInst::Create(llvm::CmpInst::ICmp, pred, lhs, rhs, "", bb);
        return v;
    }
    return NULL;
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

llvm::Value *ConditionalExpr::Emit(){    
    llvm::Value *testval=this->cond->Emit();
    llvm::Value *tval=this->trueExpr->Emit();
    llvm::Value *fval=this->falseExpr->Emit();
    llvm::Value *result=llvm::SelectInst::Create(testval,tval,fval,"",irgen->GetBasicBlock());
    return result;    
}

llvm::Value *LogicalExpr::Emit() {
    Operator *op = this->op;
    llvm::Value *lhs = left->Emit();
    llvm::Value *rhs = right->Emit();
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    if(op->IsOp("&&")) {
        llvm::Value *v = llvm::BinaryOperator::CreateAnd(lhs, rhs, "LogicalAnd", bb);
        return v;
    }
    if(op->IsOp("||")) {
        llvm::Value *v = llvm::BinaryOperator::CreateOr(lhs, rhs, "LogicalOr", bb);
        return v;
    }
    return NULL;
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

llvm::Value *ArrayAccess::Emit() {
    vector<llvm::Value*> arrayBase;
    arrayBase.push_back(llvm::ConstantInt::get(irgen->GetIntType(), 0));
    arrayBase.push_back(subscript->Emit());
    llvm::Value *elem = llvm::GetElementPtrInst::Create(dynamic_cast<llvm::LoadInst*>(this->base->Emit())->getPointerOperand(), arrayBase, "", irgen->GetBasicBlock());
    return new llvm::LoadInst(elem, "", irgen->GetBasicBlock());
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

llvm::Value *Call::Emit() {
    vector<llvm::Value*> av;
    llvm::Function* f = (llvm::Function*)symtab->LookUpValue(field->GetName());
    llvm::BasicBlock *bb = irgen->GetBasicBlock();

    for(int i = 0; i < actuals->NumElements(); i++) {
        av.push_back(actuals->Nth(i)->Emit());
    }    
    if (f==NULL)
      std::cerr<<"FunctionCall"<<endl;
    return llvm::CallInst::Create(f, av, "FunctionCall", bb);
} 


