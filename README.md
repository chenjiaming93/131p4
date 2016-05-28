# CSE131

Describe briefly how you approached the project in Project_Description.txt 

Fill in the name email and pid in the script submission.sh 
Execute the script like 

$ ./submission.sh 

if it fails to execute then try changing the permissions like 
$ chmod +x submission.sh 

if it fails for other reasons, then fix the name,email, pid etc whatever the error message says.


After it executes, you would get a zip file which you need to submit on the link given in piazza submission guildelines.


Current Status:
        Bug:
        
        AssignExpr:  
               Need to fix, a=b=c+d, a is always 0.
        ReturnStmt:
               return ; causes SegFault
        ConditionalExpr:
	      Need to fix a= b > 1 ? x++ : x--; x++ and x-- will both be executed anyway.
        
        Implementation:  
        Need to make Call::Emit,ArrayAccess::Emit, FieldAccess for cases z,w, and xy,yz,wx....
        Some others maybe new AssignExpr, new Arthemetic Expr.

          
