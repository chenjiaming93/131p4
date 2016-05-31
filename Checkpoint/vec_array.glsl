//precision highp float;   

vec2 v2;		// = vec2(1.0,  2.0);
vec4 v4;	// =  vec4(1.0,2.0,3.0,4.0);

bool test(){
	bool checkpoint = true;
	
	checkpoint = checkpoint && v2[0] == 1.0;
	checkpoint = checkpoint && v4[3] == 4.0;

    vec4 array[2];
    array[0] = v4;
    array[1] = v4;
    
    array[0][0]++;
    
    checkpoint = checkpoint && array[0].x == 2.0;
    checkpoint = checkpoint && v4.x == 1.0; 

    
    array[0] += array[0];
    checkpoint = checkpoint && array[0].w == 8.0;
    
    ++array[1].xy;
    
    checkpoint = checkpoint && array[1].x == 2.0 && array[1][1] == 3.0;
    
    return checkpoint;
}
// Expected output: 3432355

/*
void main()
{
  if(test() == 14911715) // Assert function returns expected value
    gl_FragColor = vec4(0,1,0,1.0); // green
  else
    gl_FragColor = vec4(1,0,0,1.0);  // red
}
*/
