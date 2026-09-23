#version 330

// Milestone 85 directional shadow depth vertex shader. Staged runtime asset.
// Milestone 94: vegetationInstanced selects per-instance transforms.

in vec3 vertexPosition;
in mat4 instanceTransform;

uniform mat4 mvp;
// 0 keeps the single-model depth path. Vegetation sets 1.
uniform int vegetationInstanced;

void main()
{
    if (vegetationInstanced != 0)
    {
        gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
    }
    else
    {
        gl_Position = mvp * vec4(vertexPosition, 1.0);
    }
}
