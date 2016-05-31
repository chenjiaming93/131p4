//precision highp float;   

vec2 v2; // = vec2(1.0, 2.0);
vec2 v21; // = vec2(3.3, 6.7);
vec3 v3; // = vec3(-1.0, 3.0, 5.0);
vec3 v31; // = vec3(2.2, 4.4, 6.6);
vec4 v4; // = vec4(0.0, 1.0, 2.0, 3.0);
vec4 v41; // = vec4(9.9, 8.7, 4.7, 2.9);

float hash(vec2 l2, vec3 l3, vec4 l4){
   float hash = l2.x;
   hash *= 10.0;
   hash += l2.y;
   hash *= 10.0;
   hash += l3.x;
   hash *= 10.0;
   hash += l3.y;
   hash *= 10.0;
   hash += l3.z;
   hash *= 10.0;
   hash += l4.x;
   hash *= 10.0;
   hash += l4.y;
   hash *= 10.0;
   hash += l4.z;
   hash *= 10.0;
   hash += l4.w;      
   return hash;
}

float assign_test() {

	v2 *= v3.xz;
	v2 += v4.wx;
	v2 -= v2.yx;
	v2 /= v4.zy;

	v3 *= v2.xxx;
	v3 += v3.xyz;
	v3 /= v4.wyz;
	v3 -= v3.xyz;

	v4 += v3.xyzx;
	v4 -= v2.xyxy;
	v4 *= v4.wzyx;
	v4 /= v2.yyxx;

	v2 = v2;
	v2 -= v21;
	v2 += v21;
	v2 *= v21;
	v2 /= v21;

	v3 =  v3;
	v3 -= v31;
	v3 += v31;
	v3 *= v31;
	v3 /= v31;

	v4 =  v4;
	v4 -= v41;
	v4 += v41;
	v4 *= v41;
	v4 /= v41;

	return hash(v2, v3, v4); 
}

/* void main() {
  if(assign_test() == -3.200029e+08) // Assert function call returns expected value
    gl_FragColor = vec4(0,1,0,1.0); // green
  else
    gl_FragColor = vec4(1,0,0,1.0);  // red
} */
 
