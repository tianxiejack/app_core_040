varying vec2 vVaryingTexCoords;
varying vec2 vVertex_org;
uniform sampler2D tex_in;
uniform sampler2D tex_osd;
uniform bool bOsd;
uniform mat4 mTrans;
void main(void)
{
	vec4  vColor;
	vec4 vTex44;

	vTex44.x = vVaryingTexCoords.x - 0.5f; 
	vTex44.y = vVaryingTexCoords.y - 0.5f; 
	vTex44.z = 0.0f;
	vTex44.w = 0.5f;
	vTex44 = vTex44 * mTrans;
	//vTex44.xy *= vTex44.w;
	vTex44.x += 0.5f; 
	vTex44.y += 0.5f;

	if(vTex44.x > 0.0001f && vTex44.x < 1.0001f && vTex44.y > 0.0001f && vTex44.y < 1.0001f)
	{
		vColor = texture(tex_in, vTex44);
	}

	if(bOsd)
	{
		vec4  vOsd;
		vec2  org;

		org = (vVertex_org+1.0)*0.5;
		org.t = 1.0f - org.t;
		vOsd = texture(tex_osd, org);

		if(vOsd.a > 0.0001)
			vColor.rgb = vColor.rgb*(1.0f - vOsd.a) + vOsd * vOsd.a;
	}
	
	gl_FragColor = vColor;
}
