attribute vec4 vVertex; 
attribute vec2 vTexCoords;
varying vec2 vVaryingTexCoords;
varying vec2 vVertex_org;
uniform mat4 mTrans;
void main(void)
{
	/*
	vec4 vTex;
    gl_Position = vVertex;
	vVertex_org = vVertex;
	vTex.xy = vTexCoords.xy;
	vTex.z = 0.0f;
	vTex.w = 1.0f;
	vTex = vTex * mTrans;
	vVaryingTexCoords.xy = vTex.xy;
	*/
	//vVertex.y *= 720.0/576;
    gl_Position = vVertex;
	vVertex_org = vVertex;
    vVaryingTexCoords = vTexCoords;
}
