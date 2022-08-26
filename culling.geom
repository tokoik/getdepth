#version 430 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 16) out;

// 入力
in float w[];
in vec4 idiff[];
in vec4 ispec[];
in vec2 texcoord[];

// 出力
out vec4 id;
out vec4 is;
out vec2 tc;

void main()
{
  vec3 s = vec3(w[0], w[1], w[2]);

  if (all(greaterThan(s, vec3(0.0))))
  {
    for (int i = 0; i < gl_in.length(); ++i) 
    {
      gl_Position = gl_in[i].gl_Position;
      id = idiff[i];
      is = ispec[i];
      tc = texcoord[i];
      EmitVertex();
    }

    EndPrimitive();
  }
}
