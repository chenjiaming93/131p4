vec2 a;
float assign()
{
   vec2 b;
   b=a;
   b.xy++;
   b.yx+=0.5;
   b.y-=1.0;
   return b.y;
}
