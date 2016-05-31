//precision highp float;   



float fun(int a){
    float f;
    switch(a + 1){
    	case 0:
        case 1: f = 0.00;
        /* $FALL-THROUGH$ */
        case 2: f = 1.00; 
        /* $FALL-THROUGH$ */
        case 3: f = 2.00; break;
        default: f = 3.00; break;
    }
    
    if(f == 2.0)
    	f += 10.0;
    	
    return f;
}



// Expected output: 12.0

/*
void main()
{
  if(test() == 2.0) // Assert function returns expected value
    gl_FragColor = vec4(0,1,0,1.0); // green
  else
    gl_FragColor = vec4(1,0,0,1.0);  // red
}
*/
