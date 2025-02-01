#version 450

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 frag_color;
//layout(location = 1) in vec2 frag_tex_coord;

layout(location = 0) out vec4 out_color;

void main() {
    /*
    out_color = vec4(frag_color * texture(tex_sampler, frag_tex_coord).rgb, 1.0);
    */
    vec2 center = gl_PointCoord - vec2(0.5, 0.5);
    float distSq = dot(center, center);
    if (distSq > 0.25) {
        discard;
    }
    out_color = vec4(frag_color, 1.0);
}