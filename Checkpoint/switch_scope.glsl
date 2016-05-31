//precision highp float;  

int y;	// =  1;

int test(){
	int x = 1;
	int z = 1;
	switch(y){
		case 1:
		 int y = 5;
		 int x = 8;
		 z = y + x;
		 break;
	}
	
	return x*10000 + y*100 + z;
}
// Expected output: 10113

/*
void main()
{
  if(test() == 10113) // Assert function returns expected value
    gl_FragColor = vec4(0,1,0,1.0); // green
  else
    gl_FragColor = vec4(1,0,0,1.0);  // red
}
*/
