#version 320 es

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 uShadowMatrices[6];

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

    gl_Layer = 1;
    emitFace(uShadowMatrices[1]);

    gl_Layer = 2;
    emitFace(uShadowMatrices[2]);

    gl_Layer = 3;
    emitFace(uShadowMatrices[3]);

    gl_Layer = 4;
    emitFace(uShadowMatrices[4]);

    gl_Layer = 5;
    emitFace(uShadowMatrices[5]);
}
