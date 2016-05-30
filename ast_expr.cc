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
    //std::cerr<<"Int Const"<<endl;
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
    //std::cerr<<"Var Expr Done"<<endl;
    llvm::Value *v = symtab->LookUpValue(GetIdentifier()->GetName());
    llvm::Twine *twine = new llvm::Twine(this->id->GetName());
    llvm::Value *in = new llvm::LoadInst(v, *twine, irgen->GetBasicBlock());
    return in;
}

llvm::Value *ArithmeticExpr::Emit() {
    Operator *op = this->op;
    //std::cerr<<"Entering Arithmetic Expr"<<endl;
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
            //std::cout<<"Error is here"<<endl;
	    llvm::Value *result = llvm::BinaryOperator::CreateAdd(lhs, rhs, "", bb);
            return result;
        }
        else if(op->IsOp("*")) {
            llvm::Value *vec;
            llvm::Value *fp;
            if(lhs->getType() == irgen->GetType(Type::vec2Type)) {
                vec = lhs;
                fp = rhs;
		//std::cout<<"Left Vec"<<endl;
            }
            else if(rhs->getType() == irgen->GetType(Type::vec2Type)) {
                vec = rhs;
                fp = lhs;
		//std::cout<<"Right Vec"<<endl;
            }
            else {
                vec = NULL;
                fp = NULL;
		//std::cout<<"Some special type";
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
	else if (op->IsOp("-"))
	{
	    llvm::Value *result = llvm::BinaryOperator::CreateSub(lhs, rhs, "", bb);
	    return result;
	}
	else if (op->IsOp("/"))
	{
	    llvm::Value *result = llvm::BinaryOperator::CreateSDiv(lhs, rhs, "", bb);
	    return result;
	}
    }
    //std::cout<<"returning null"<<endl;
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
    llvm::Value *store;
    llvm::Value *rv = this->right->Emit();
    if( llvm::StoreInst* si = dynamic_cast<llvm::StoreInst*>(rv) ) {
   	 rv = si->getValueOperand();
 	 }
    llvm::Value *lv = this->left->Emit();
    llvm::LoadInst *ld = llvm::cast<llvm::LoadInst>(lv);
    llvm::Value *loc = ld->getPointerOperand();
    llvm::BasicBlock *bb = irgen->GetBasicBlock();
    FieldAccess* l=dynamic_cast<FieldAccess*>(left);
    FieldAccess* r=dynamic_cast<FieldAccess*>(right);
    char *swiz=NULL;
    char *rswiz=NULL;
     if(op->IsOp("=")) {
	if (l)
	{
		llvm::Value* la=l->EmitAddress();
		
		if (rv->getType() == irgen->GetType(Type::floatType)){
			swiz=l->GetField()->GetName();
			llvm::Constant *idx;
				//std::cerr<<"Assign float to FA"<<endl;
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
				//
					
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
					llvm::Value *extract=llvm::ExtractElementInst::Create(rv,ridx,"",bb);
					store=llvm::InsertElementInst::Create(loc,extract,idx,"",bb);
					llvm::Value* result = new llvm::StoreInst(store, la, "",bb);
				}
			}
			else{
				//std::cerr<<"Assign FA to FA"<<endl;
				llvm::Value *ra = r->EmitAddress();
				llvm::Value *rloc=new llvm::LoadInst(ra,"",bb);
				swiz=l->GetField()->GetName();
				rswiz=r->GetField()->GetName();
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
					store=llvm::InsertElementInst::Create(loc,extract,idx,"",bb);
					llvm::Value* result = new llvm::StoreInst(store, la, "",bb);

				}			
			}		
		}
		
	}	
     	else
	{
           llvm::Value* in = new llvm::StoreInst(rv, loc, bb);
	}
    }

    else if((lv->getType() == irgen->GetType(Type::floatType)) && (rv->getType() == irgen->GetType(Type::floatType))) {
        if(op->IsOp("+=")) {
            llvm::Value *add = llvm::BinaryOperator::CreateFAdd(lv, rv, "PlusEqual", bb);
            llvm::Value *in = new llvm::StoreInst(add, loc, bb);
	}
        else if(op->IsOp("-=")) {
            llvm::Value *sub = llvm::BinaryOperator::CreateFSub(lv, rv, "MinusEqual", bb);
            llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
	}
        else if(op->IsOp("*=")) {
            llvm::Value *mul = llvm::BinaryOperator::CreateFMul(lv, rv, "MulEqual", bb);
            llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
        }
        else if(op->IsOp("/=")) {
            llvm::Value *div = llvm::BinaryOperator::CreateFDiv(lv, rv, "DivEqual", bb);
            llvm::Value *in = new llvm::StoreInst(div, loc, bb);
        }
    }

    else if((lv->getType() == irgen->GetType(Type::intType)) && (rv->getType() == irgen->GetType(Type::intType))) {
        if(op->IsOp("+=")) {
            llvm::Value *add = llvm::BinaryOperator::CreateAdd(lv, rv, "PlusEqual", bb);
            llvm::Value *in = new llvm::StoreInst(add, loc, bb);
	}
        else if(op->IsOp("-=")) {
            llvm::Value *sub = llvm::BinaryOperator::CreateSub(lv, rv, "MinusEqual", bb);
            llvm::Value *in = new llvm::StoreInst(sub, loc, bb);
        }
        else if(op->IsOp("*=")) {
            llvm::Value *mul = llvm::BinaryOperator::CreateMul(lv, rv, "MulEqual", bb);
            llvm::Value *in = new llvm::StoreInst(mul, loc, bb);
        }
        else if(op->IsOp("/=")) {
            llvm::Value *div = llvm::BinaryOperator::CreateSDiv(lv, rv, "DivEqual", bb);
            llvm::Value *in = new llvm::StoreInst(div, loc, bb);
        }
    }
    return rv;
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
llvm::Value *FieldAccess::EmitAddress()
{
  VarExpr* isVar=dynamic_cast<VarExpr*>(base);
  if (isVar){
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
    llvm::CmpInst::Predicate pred;

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
    if(op->IsOp("&&")){
        llvm::Value *v = llvm::BinaryOperator::CreateAnd(lhs, rhs, "LogicalAnd", bb);
        return v;
    }
    if(op->IsOp("||")) 
    {
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


