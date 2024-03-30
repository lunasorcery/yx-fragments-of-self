// Generated with Shader Minifier 1.3.6 (https://github.com/laurentlb/Shader_Minifier/)
#ifndef FRAG_PRESENT_H_
# define FRAG_PRESENT_H_
# define VAR_accumulatorTex "a"
# define VAR_fragColor "v"
# define VAR_iResolution "s"

const char *present_frag =
 "#version 430\n"
 "layout(location=0) out vec4 v;"
 "layout(location=0) uniform vec4 s;"
 "layout(binding=0) uniform sampler2D a;"
 "void main()"
 "{"
   "vec4 d=texelFetch(a,ivec2(gl_FragCoord.xy),0);"
   "vec3 l=d.xyz/d.w;"
   "vec2 g=gl_FragCoord.xy/s.xy;"
   "g-=.5;"
   "l*=1.-dot(g,g)*.6;"
   "l*=6.;"
   "l=1.-exp(l*-2.);"
   "l=pow(l,vec3(1,1.1,1.2));"
   "l=pow(l,vec3(.45));"
   "g*=s.zw;"
   "l*=step(abs(g.y),.5/1.5);"
   "l*=step(abs(g.x),.75);"
   "v=vec4(vec3(l),1);"
 "}";

#endif // FRAG_PRESENT_H_
