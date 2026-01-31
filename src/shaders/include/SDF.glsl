float sdBox(vec3 p, vec3 s) {
    p = abs(p)-s;
	return length(max(p, 0.))+min(max(p.x, max(p.y, p.z)), 0.);
}

mat3 rotate3D(float ang, vec3 axis){
    axis = normalize(axis);
    float s=sin(ang), c=cos(ang), oc=1.0-c;
    return mat3(
        oc*axis.x*axis.x + c,
        oc*axis.x*axis.y - axis.z*s,
        oc*axis.z*axis.x + axis.y*s,

        oc*axis.x*axis.y + axis.z*s,
        oc*axis.y*axis.y + c,
        oc*axis.y*axis.z - axis.x*s,

        oc*axis.z*axis.x - axis.y*s,
        oc*axis.y*axis.z + axis.x*s,
        oc*axis.z*axis.z + c
    );
}



vec2 GetDistMat(vec3 p){
    vec3 a = vec3(1.0, 0.0, 0.0);

    vec3 q = p;
    float d = q.y;
    float mat = 0.0;

    float iVal = 250.0;

    for(int k=0; k<FOLD_STEPS; k++){
        mat3 foldRot = rotate3D(0.5, a.zxz); 
        vec3 cell    = mod(q * foldRot, iVal + iVal) - iVal;
        q            = iVal * 0.9 - abs(cell);

        float dFoldX = min(q.x, q.y);
        float dMin   = min(dFoldX, q.z);

        float prevD = d;
        d = max(d, dMin);

        if(d != prevD){
            mat = float(k) / float(FOLD_STEPS-1);
        }

        iVal *= 0.75;
        if(iVal < FOLD_FLOOR) break;
    }

    //d = max(p.y, d);

    //d = max(-sdBox(p, vec3(250., 100., 250.)), d);


    return vec2(d, mat);
}


float GetDist(vec3 p){
    return GetDistMat(p).x;
}

