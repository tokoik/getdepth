#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// 頂点座標
layout (location = 0) in vec4 pv;

// テクスチャ座標
out vec2 texcoord;

void main()
{
  // 頂点座標をテクスチャ座標に換算
  texcoord = pv.xy * 0.5 + 0.5;

  // 頂点座標をそのまま出力
  gl_Position = pv;
}
