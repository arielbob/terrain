#version 330 core

in vec3 vertex_position;
in vec3 vertex_normal;
in vec2 vertex_uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normal_transform;
uniform float max_height;

out vec3 frag_pos;
out vec4 frag_color;
out vec3 normal;
out float scaled_max_height;
out vec2 uv;

void main() {
    frag_pos = vec3(model * vec4(vertex_position, 1.0));
    gl_Position = projection * view * model * vec4(vertex_position, 1.0);
    scaled_max_height = (model * vec4(0.0f, max_height, 0.0f, 1.0f)).y;
    
    // NOTE: w of vec4 is 0 to ignore the translation of the model matrix
    normal = vec3(normal_transform * vec4(vertex_normal, 0.0));
    uv = vertex_uv;
}
