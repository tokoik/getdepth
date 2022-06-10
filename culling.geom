#version 430 core

// デバイス番号
uniform int devId;

// カメラの位置への変換行列
uniform mat4 mcam[3];

// クリッピング座標系への変換行列の逆行列
uniform mat4 mc;

layout(triangles) in;
layout(triangle_strip, max_vertices = 16) out;

// テクスチャ座標
in vec2 t[];
out vec2 tc;

void main()
{
  vec3 z = vec3(gl_in[0].gl_Position.z, gl_in[1].gl_Position.z, gl_in[2].gl_Position.z);

  if (all(greaterThan(z, vec3(0.0))))
  {
    for (int i = 0; i < gl_in.length(); ++i) 
    {
      gl_Position = mc * mcam[devId] * gl_in[i].gl_Position;
      tc = t[i];
      EmitVertex();
    }
  }
}
