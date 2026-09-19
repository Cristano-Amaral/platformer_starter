#version 330

// Milestone 85 directional shadow depth vertex shader. Staged runtime asset.

in vec3 vertexPosition;
uniform mat4 mvp;

void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
