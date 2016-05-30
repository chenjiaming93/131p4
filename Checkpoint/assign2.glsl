vec2 a;
float assign()
{
   vec4 v4;
   v4.xy = a;
   float t;
   t = 1.0;
   v4.y = t;
   v4.xy+= a;
   return v4.y;
}
