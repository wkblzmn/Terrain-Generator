#version 330 core

layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aPos;

    // xyww trick: sets NDC depth to 1.0 (maximum), so skybox is always
    // rendered behind everything. Requires GL_LEQUAL depth function.
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
