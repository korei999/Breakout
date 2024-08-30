#version 320 es

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

out vec4 gFragPos; /* output per emitvertex */

void
emitFace(mat4 m)
{
    for (int i = 0; i < 3; i++)
    {
        gFragPos = gl_in[i].gl_Position;
        gl_Position = m * gFragPos;
        EmitVertex();
    }
    EndPrimitive();
}

void
main()
{
    gl_Layer = 0;
    emitFace(uShadowMatrices[0]);
}
