vec2 a;

float assignvec(float f) 
{
   float t;
   vec4 v4;

   t = f + 0.5;

   v4.xy = a;
   v4.w = v4.z = t;

   return v4.x + v4.y + v4.w + v4.z;
} 
