#pragma group("Raymarching")
#pragma slider(RAYMARCH_STEPS, 25, 200, 100, "Steps")
#pragma slider(RAYMARCH_MAX_DISTANCE, 5, 1000, 100, "Max Dist")
#pragma switch(RAYMARCH_RELAXED, false, "Normal tracing", "Relaxed tracing")
#pragma switch(RAYMARCH_INTERNAL, false, "March positive", "March both")
#pragma switch(RAYMARCH_SOFT_SHADE_SIMPLE, false, "Simple softshade", "Penumbra softshade")
#pragma slider(RAYMARCH_SOFT_SHADE_STEPS, 10, 100, 50, "Shade Steps")
#pragma endgroup()

#ifndef EPS
vec3 eps = vec3(0.01, 0.0, 0.0);
#define EPS eps
#endif

#ifndef RAYMARCH_MAX_DISTANCE
#define RAYMARCH_MAX_DISTANCE   10000.0
#endif

#ifndef RAYMARCH_MIN_DISTANCE
#define RAYMARCH_MIN_DISTANCE   0.02
#endif

#ifndef RAYMARCH_STEP_SCALE
#define RAYMARCH_STEP_SCALE   1.0
#endif

float map(vec3 p);

// Return the normalized normal vector at point p by sampling the distance field at p and nearby points.
vec3 normal(vec3 p) {
	float d = map(p);
	return normalize(vec3(
		d - map(p-EPS.xyz),
		d - map(p-EPS.yxz),
		d - map(p-EPS.yzx)
	));
}

// Marches along a ray and returns a vec2 where:
// x = total distance traveled (t)
// y = closeness to the surface (distance at hit)
vec2 intersect(vec3 ro, vec3 rd) {
	// .x = t
    // .y = dt
	vec2 hit = vec2(0.1, 0.1);

#ifdef RAYMARCH_RELAXED
#ifdef RAYMARCH_INTERNAL
	float prev = 0.0, sgn = map(ro)<0.0 ? -1.0 : 1.0;
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
		if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
            hit.y = map(ro + rd * hit.x);
			float L_est = 1.0 / max(1.0, abs( sgn * hit.y - prev) / (hit.x - prev));
			prev = sgn * hit.y * L_est;
	        hit.x += sgn * hit.y * L_est;
		}
		if (hit.x > RAYMARCH_MAX_DISTANCE) {
			break;
		}
	}
#else	
	float prev = 0.0;
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
		if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
            hit.y = map(ro + rd * hit.x);
			float L_est = 1.0 / max(1.0, abs( hit.y - prev) / (hit.x - prev));
			prev = hit.y * L_est;
	        hit.x += hit.y * L_est;
		}
		if (hit.x > RAYMARCH_MAX_DISTANCE) {
			break;
		}
	}
#endif	
#else

#ifdef RAYMARCH_INTERNAL
	float sgn = map(ro)<0.0 ? -1.0 : 1.0;
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
	if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
		hit.y = map(ro + rd * hit.x);
		hit.x += hit.y * sgn * RAYMARCH_STEP_SCALE;
	}
	if (hit.x > RAYMARCH_MAX_DISTANCE) {
		break;
	}
}
#else
	for (int i=0; i<RAYMARCH_STEPS; i++ ) { 		
		if (abs(hit.y) > RAYMARCH_MIN_DISTANCE) {
			hit.y = map(ro + rd * hit.x);
			hit.x += hit.y * RAYMARCH_STEP_SCALE;
		}
		if (hit.x > RAYMARCH_MAX_DISTANCE) {
			break;
		}
	}
#endif		
#endif
    return hit;
}

#ifdef RAYMARCH_SOFT_SHADE_SIMPLE
float softshadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k )
{
    float h, res = 1.0;
    float t = mint;
    for( int i=0; i<RAYMARCH_SOFT_SHADE_STEPS && t<maxt; i++ )
    {
        h = map(ro + rd*t);
        if( h<mint )
            return 0.0;
        res = min( res, k*h/t );
        t += h;
    }
    return res;
}
#else
float softshadow(vec3 ro, vec3 rd, float mint, float maxt, float w )
{
    float res = 1.0;
    float t = mint;
    for( int i=0; i<RAYMARCH_SOFT_SHADE_STEPS && t<maxt; i++ )
    {
        float h = map(ro + t*rd);
        res = min( res, h/(w*t) );
        t += clamp(h, 0.005, 0.50);
        if( res<-1.0 || t>maxt ) break;
    }
    res = max(res,-1.0);
    return 0.25*(1.0+res)*(1.0+res)*(2.0-res);
}
#endif
