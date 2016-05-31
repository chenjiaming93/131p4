//precision highp float;   


bool test(){
  int x = 0;
	int y = 1;
	bool checkpoint = true;
	
	if(x == 0 || ++x == 1)
		checkpoint = checkpoint && true;
 	else
 	   checkpoint = false;
		
	checkpoint = checkpoint && x == 0;
	
	if(y == 0 && ++y == 1)
		checkpoint = false;
	checkpoint = checkpoint && y == 1;
	
	x = 0;
	y = 0;
	if(++x == 0 && ++y == 0)
		checkpoint = false;
	
	checkpoint = checkpoint && x == 1 && y == 0;	
	
	
	x = 0;
	y = 0;
	if(++x == 1 || ++y == 0)
		checkpoint = checkpoint && true;

	
	checkpoint = checkpoint && x == 1 && y == 0;
	
	
	return checkpoint;
}
// Expected output: -1

/*
void main()
{
  if(test() == 14911715) // Assert function returns expected value
    gl_FragColor = vec4(0,1,0,1.0); // green
  else
    gl_FragColor = vec4(1,0,0,1.0);  // red
}
*/
