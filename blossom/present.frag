/* framework header */
#version 430
layout(location = 0) out vec4 fragColor;
layout(location = 0) uniform vec4 iResolution;
layout(binding = 0) uniform sampler2D accumulatorTex;




void main()
{
	// readback the buffer
	vec4 tex = texelFetch(accumulatorTex,ivec2(gl_FragCoord.xy),0);

	// divide accumulated color by the sample count
	vec3 color = tex.rgb / tex.a;
	
	//fragColor=vec4(color*color,1);return;

	// vignette to darken the corners
	vec2 uv = gl_FragCoord.xy/iResolution.xy;
	uv-=.5;
	color *= 1.-dot(uv,uv)*.6;

    // exposure and tonemap
    color *= 6.;
    color = 1.-exp(color*-2.);

    // warm grade
    color = pow(color,vec3(1,1.1,1.2));
     
    // pop
    //color = mix(color, smoothstep(0., 1., color), 0.4);
    
	// gamma correction
	color = pow(color, vec3(.45));
	
	//color /= vec3(0.9,0.85,0.8);

    // aspect ratio
    uv*=iResolution.zw;
    color *= step(abs(uv.y),.5/(3./2.));
    color *= step(abs(uv.x),.5*(3./2.));

    // "final" color
    fragColor = vec4(vec3(color),1);
}
