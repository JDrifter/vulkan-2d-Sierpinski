#version 450

vec2 pos[3] = vec2[](
    vec2(-0.5, -0.5),
    vec2(0.5, 0.0),
    vec2(-0.5, 0.5)
);

void main() { //once per vertex
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);
    // 4 dim vector, x,y,layer,normalization(1.0 non cambia nulla)

}