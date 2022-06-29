#version 330 core

in vec3 vertex_position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 frag_pos;

void main() {
    frag_pos = vec3(model * vec4(vertex_position, 1.0));
    gl_Position = projection * view * model * vec4(vertex_position, 1.0);
}
