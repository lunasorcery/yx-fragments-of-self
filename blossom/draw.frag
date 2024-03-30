/* framework header */
#version 430
layout(location = 0) out vec4 fragColor;
layout(location = 0) uniform vec4 iResolution;
layout(location = 1) uniform int iFrame;

uniform sampler2D iChannel1;


/* vvv your shader goes here vvv */

#define pi acos(-1.)
#define tau (pi*2.)

float seed;
float hash() {
	float p=fract((seed++)*.1031);
	p+=(p*(p+19.19))*3.;
	return fract((p+p)*p);
}
vec2 hash2(){return vec2(hash(),hash());}


mat2 rotate(float b)
{
    float c = cos(b);
    float s = sin(b);
    return mat2(c,-s,s,c);
}

float sdBox(vec2 p, vec2 b)
{
    vec2 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,q.y),0.0);
}

float sdBox(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0));
}

float sdBox(vec2 p, vec2 b, float r)
{
    return sdBox(p,b-r)-r;
}

float sdRoundedCylinder(vec3 p, float radius, float halfHeight, float bevel)
{
    vec2 p2 = vec2(length(p.xz),p.y);
    return sdBox(p2,vec2(radius,halfHeight),bevel);
}

int mat = -1;
int lastHitMat = -1;
const int kMatGround = 0;
const int kMatPaper = 1;
//const int kMatRedPlastic = 2;
const int kMatRibbon = 3;

float kPageWidth = 100.;
float kPageHeight = 150.;
float kPageCurvatureInset = 20.;
float kPageDepth = 10.;

float sdFiber(vec2 p)
{
    float shell = length(p)-.35;
    return shell;

    p = mod(p,.125)-.0625;
    return max(shell, length(p)-.05);
}

float sdPartialRibbon(vec3 p)
{
    //p.y += texture(iChannel1,p.xz*.01).r*.1-.05;
    
    p.z = abs(p.z)-4.;
    p.z = abs(p.z)-2.;
    p.z = abs(p.z)-1.;
    p.z = abs(p.z)-.5;
    p.z = abs(p.z)-.25;
    
    //p.y += sin(p.x*2.)*.0625;
	p.y += sin(p.x*2.)*.05;
    return sdFiber(p.zy);
}

float sdCrossRibbon(vec3 p)
{
    float bound = abs(p.z)-7.75;
    //p.y-=2.;
    p.x = mod(p.x,pi*.25)-pi*.125;
    return max(bound,length(p.xy)-.35);
}

float sdRibbon(vec3 p)
{
    float proxy = sdBox(p.zy,vec2(8.,.35));
	if (proxy < 1)
	{
		float c = sdCrossRibbon(p);
		p.z -= .125;
		float a = sdPartialRibbon(p);
		p.z += .25;
		float b = sdPartialRibbon(p*vec3(-1,1,1));
    
		return min(c,min(a,b));
    }
	return proxy;
}

float scene(vec3 p)
{
    vec3 op=p;
    
    float ground = p.y + 300.;

    p.z = abs(p.z) - kPageCurvatureInset;
    float paper = p.y;
    float pageDepth = kPageDepth;
    if (op.z > 0.) {
        pageDepth *= 0.7;
    }
    if (p.z < 0.) {
        //woodCenter -= cos((pi*.5)*(p.z/kPageCurvatureInset))*kPageDepth;
        //woodCenter -= sqrt(1.-abs(p.z/kPageCurvatureInset))*kPageDepth;
        float x = abs(p.z/kPageCurvatureInset);
        paper -= sqrt(1.-x*x)*pageDepth;
    } else {
        //woodCenter = p.y-(cos(pi*(p.z/(kPageWidth-kPageCurvatureInset)))*.5+.5)*kPageDepth;
        paper -= smoothstep(0.,1.,1.-p.z/(kPageWidth-kPageCurvatureInset))*pageDepth;
    }
    
    paper = max(paper,sdBox(op.xz,vec2(kPageHeight*.5,kPageWidth)));

	if (abs(paper) < 0.1) {
		float disp_xz0 = texture(iChannel1,op.xz  *0.015625  *.1).r;
		float disp_xz1 = texture(iChannel1,op.xz  *0.015625  *.05).r;
		float disp_xz2 = texture(iChannel1,op.xz  *0.015625  *.025).r;
		paper -= (disp_xz0+disp_xz1+disp_xz2)*.02;
	}

    /*p=op;
    p.z -= kPageWidth*.5;
    p.y -= kPageDepth + 10.;
    float debugBall = min(
        min(
            length(p)-5.,
            length(p.xz)-1.
        ),
        min(
            length(p.xy)-1.,
            length(p.zy)-1.
        )
    );*/

    p=op;
    p.y -= 200.;
    float light = length(p)-30.;
    light = 1e9;

    p=op;
    p.y-=5.;
    p.z-=1.5;
    p.y-=(sin(max(p.x*.05,-pi*.5))*.5+.5)*.5;
	p.y+=cos(p.z*.5)*.1;
    float ribbon = sdRibbon(p*2.)*.5;

    float best = ground;
    best=min(best,paper);
    //best=min(best,debugBall);
    best=min(best,ribbon);
    best=min(best,light);

    
    if(best==ground)
    {
        mat = kMatGround;
    }
    else if (best==paper)
    {
        mat = kMatPaper;
    }
    else// if (best==ribbon)
    {
        mat = kMatRibbon;
    }
    /*else if (best==light)
    {
        mat = kMatLight;
    }
    else
    {
        mat = kMatRedPlastic;
    }*/
    
    return best;
}

vec3 ortho(vec3 a){
    vec3 b=cross(vec3(-1,-1,-1),a);
    // assume b is nonzero
    return (b);
}

// various bits of lighting code "borrowed" from 
// http://blog.hvidtfeldts.net/index.php/2015/01/path-tracing-3d-fractals/
vec3 getSampleBiased(vec3  dir, float power) {
	dir = normalize(dir);
	vec3 o1 = normalize(ortho(dir));
	vec3 o2 = normalize(cross(dir, o1));
	vec2 r = hash2();
	r.x=r.x*2.*pi;
	r.y=pow(r.y,1.0/(power+1.0));
	float oneminus = sqrt(1.0-r.y*r.y);
	return cos(r.x)*oneminus*o1+sin(r.x)*oneminus*o2+r.y*dir;
}

vec3 getConeSample(vec3 dir, float extent) {
	dir = normalize(dir);
	vec3 o1 = normalize(ortho(dir));
	vec3 o2 = normalize(cross(dir, o1));
	vec2 r =  hash2();
	r.x=r.x*2.*pi;
	r.y=1.0-r.y*extent;
	float oneminus = sqrt(1.0-r.y*r.y);
	return cos(r.x)*oneminus*o1+sin(r.x)*oneminus*o2+r.y*dir;
}

vec3 sky(vec3 sunDir, vec3 viewDir) {
    /*float keylightPower = 4.;
    float keylightAmplitude = 20.;
    
    float softlight = max(0.,dot(normalize(sunDir*vec3(-1,1.,-1)),viewDir)+.2);
    float keylight = pow(max(0.,dot(sunDir,viewDir)-.5),keylightPower);
    
    vec3 softlightColor = vec3(0,1,2);
    vec3 keylightColor = vec3(1.,.5,.1)*.5;
    
    return vec3(
		softlight*.015*softlightColor + 
        keylight * keylightAmplitude*keylightColor
	)*1.5;*/
    
    float keylightPower = 4.;
    float keylightAmplitude = 20.;
    float keylight = pow(max(0.,dot(sunDir,viewDir)-.5)*2.,keylightPower);
    
    vec3 softlightColor = vec3(0,.25,1)*.05;
    vec3 keylightColor = vec3(5,4,3)*.25;
    
	vec3 skyColor = mix(softlightColor, keylightColor, keylight);
	vec3 neutral = vec3(dot(skyColor,vec3(0.2126,0.7152,0.0722)));
    return mix(skyColor,neutral,.5);
    
    /*return vec3(
		softlight*vec3(.03,.06,.1)*2. + keylight * vec3(10,7,4)
	)*1.5;*/

    /*float softlight = max(0.,dot(sunDir,viewDir)+.2);
    float keylight = pow(max(0.,dot(sunDir,viewDir)-.5),3.);
    
    return vec3(
		softlight*.5 + keylight * 10.
	)*1.5;*/
}    

bool trace5(vec3 cam, vec3 dir, out vec3 h, out vec3 n, out float k) {
	float t=0.;
    for(int i=0;i<100;++i)
    {
        k = scene(cam+dir*t);
        if (abs(k) < .001)
            break;
        t += k;
    }

    h = cam+dir*t;
	
    // if we hit something
    if(abs(k)<.001)
    {
        vec2 o = vec2(.001, 0);
        n = normalize(vec3(
            scene(h+o.xyy) - k,
            scene(h+o.yxy) - k,
            scene(h+o.yyx) - k 
        ));
        return true;
    }
    return false;
}

/*float floorPattern(vec2 uv)
{
    float kUnit1 = 10.;
    float kUnit2 = 5.;
    float kUnit3 = 1.;
    float kThick1 = 0.1;
    float kThick2 = 0.05;
    float kThick3 = 0.03;

    vec2 uv1 = abs(mod(uv,kUnit1)-kUnit1*.5);
    vec2 uv2 = abs(mod(uv,kUnit2)-kUnit2*.5);
    vec2 uv3 = abs(mod(uv,kUnit3)-kUnit3*.5);
    float lines1 = -max(uv1.x,uv1.y)+kUnit1*.5-kThick1;
    float lines2 = -max(uv2.x,uv2.y)+kUnit2*.5-kThick2;
    float lines3 = -max(uv3.x,uv3.y)+kUnit3*.5-kThick3;
    
    return min(lines1,min(lines2,lines3));
}*/

float linearstep(float a, float b, float c)
{
	c = clamp(c,a,b);
	return (c-a)/(b-a);
}

vec2 linearstep2(vec2 a, vec2 b, vec2 c)
{
	return vec2(
		linearstep(a.x, b.x, c.x),
		linearstep(a.y, b.y, c.y)
	);
}

vec3 trace2(vec3 cam, vec3 dir)
{
    //const vec3 sunDirection = normalize(vec3(-1,.1,-.5));
    //const vec3 sunDirection = normalize(vec3(.0,.5,1));
    //const vec3 sunDirection = normalize(vec3(-.5,.2,-.7));
    //const vec3 sunDirection = normalize(vec3(-1.,.8,-.7));
    //const vec3 sunDirection = normalize(vec3(1.,.7,-.3));
    //const vec3 sunDirection = normalize(vec3(.5,.3,1));
    //const vec3 sunDirection = normalize(vec3(0,1,0));
    
    /*float z = (iMouse.x/iResolution.x)*2.-1.;
    float x = 1.-2.*(iMouse.y/iResolution.y);
    float len = length(vec2(x,z));
    if (len > 1.) {
        x /= len*1.01;
        z /= len*1.01;
    }
    float y = sqrt(1.-x*x-z*z);
    vec3 sunDirection = vec3(x,y,z);*/
    
    vec3 sunDirection = normalize(vec3(0,.5,-.8));
    
    vec3 accum = vec3(1);
    for(int ibounce=0;ibounce<5;++ibounce)
    {
        vec3 h,n;
        float k;
        if (trace5(cam,dir,h,n,k))
        {
            lastHitMat = mat;
            cam = h+n*.01;
            
            //return vec3(dot(-dir,n)*.5+.5);
            
            if (mat == kMatGround)
            {
            	dir=getSampleBiased(n,1.);
				//accum *= mix(vec3(.25,.3,.35),vec3(.8),step(0.,floorPattern(h.xz)));

                accum *= .8;
            }
            else if (mat == kMatPaper)
            {
            	dir=getSampleBiased(n,1.);
                vec3 col = vec3(211,183,155)/255.;
				col = mix(col,vec3(1),.25);
				accum *= col*col*col;
                
                //accum *= step(.1,abs(h.x));
                //accum *= step(.1,abs(h.z-kPageWidth*.5));
                //accum *= step(.1,abs(h.z-kPageCurvatureInset));
                
                // words
				vec2 words_uv = linearstep2(vec2(10.,-50.),vec2(110.,50.),h.zx);
				accum *= texture(iChannel1, words_uv).b;
            }
            /*else if (mat == kMatRedPlastic)
            {
                float fresnel = pow(1.-min(.99,dot(-dir,n)),5.);
                fresnel = mix(.04,1.,fresnel);
                if (hash() < fresnel)
                {
                	dir=reflect(dir,n);
                }
                else
                {
            		dir=getSampleBiased(n,1.);
                    accum *= vec3(180,2,1)/255.;
                }
            }*/
            else if (mat == kMatRibbon)
            {
                float fresnel = pow(1.-min(.99,dot(-dir,n)),5.);
                fresnel = mix(.04,1.,fresnel);
                if (hash() < fresnel)
                {
                	dir=reflect(dir,n);
                }
                else
                {
            		dir=getSampleBiased(n,1.);
                    accum *= vec3(120,2,0)/255.;
                }
            }
        }
        else if (abs(k) > .1) {
            return sky(sunDirection, dir) * accum;
        } else {
            break;
        }
    }
    
    return sky(sunDirection, dir) * accum;
    
    // deliberately fail the pixel
    return vec3(-1);
}

vec2 bokeh(){
    // hexagon
    vec2 a = hash2();
    a.x=a.x*3.-1.;
    a-=step(1.,a.x+a.y);
	a.x += a.y * .5;
	a.y *= sqrt(.75);
    return a;
}

void main()
{
    // seed the RNG (again taken from Devour)
    seed = float(((iFrame*73856093)^int(gl_FragCoord.x)*19349663^int(gl_FragCoord.y)*83492791)%38069);
    //seed = float((iFrame*73856093)%38069);

    // get UVs
    //vec2 uv = (gl_FragCoord.xy+hash2()-.5)/iResolution.xy-.5;
	vec2 uv = (gl_FragCoord.xy+hash2()*1.5-.75)/iResolution.xy-.5;
    
    // correct UVs for aspect ratio
    float aspect = iResolution.x/iResolution.y;
    uv.x*=aspect;
	uv *= max(1.,(3./2.)/aspect);

    // camera params
    //const vec3 camPos = vec3(200,170,60+10);
    const vec3 camPos = vec3(200,150,80-1);
    const vec3 lookAt = vec3(0,2.,30+10-1);
    const float focusDistance=distance(camPos,lookAt)*.99;
    const vec2 apertureRadius=vec2(1)*1.5;
    
    // make a camera
    vec3 cam = vec3(0);
    vec3 dir = normalize(vec3(uv,3.7));
    
    // add some bokeh
    vec2 bokehJitter=bokeh();
    cam.xy+=bokehJitter*apertureRadius;
    dir.xy-=bokehJitter*apertureRadius*dir.z/focusDistance;
    
    // rotate/move the camera
    vec3 lookDir = lookAt-camPos;
    float pitch = -atan(lookDir.y,length(lookDir.xz));
    float yaw = -atan(lookDir.x,lookDir.z);
    cam.yz *= rotate(pitch);
    dir.yz *= rotate(pitch);
    cam.xz *= rotate(yaw);
    dir.xz *= rotate(yaw);
    cam += camPos;
    
    // compute the pixel color
	vec3 pixel = trace2(cam,dir);
    
    fragColor = (!isnan(pixel.r) && pixel.r >= 0.) ? vec4(pixel,1) : vec4(0);
}