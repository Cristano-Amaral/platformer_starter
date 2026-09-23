#version 330

// Milestone 85 world lit vertex shader. Staged runtime asset.
// Milestone 94: vegetationInstanced selects per-instance transforms.
// Direction convention: CPU stores light *ray travel* direction.

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in mat4 instanceTransform;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
// 0 keeps the single-model path used by Terrain, props, and greybox meshes.
// Vegetation sets 1 so each instance supplies its own transform.
uniform int vegetationInstanced;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;

void main()
{
    if (vegetationInstanced != 0)
    {
        mat4 model = instanceTransform;
        fragPosition = vec3(model * vec4(vertexPosition, 1.0));
        fragTexCoord = vertexTexCoord;
        fragColor = vertexColor;
        fragNormal = normalize(transpose(inverse(mat3(model))) * vertexNormal);
        gl_Position = mvp * model * vec4(vertexPosition, 1.0);
    }
    else
    {
        fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
        fragTexCoord = vertexTexCoord;
        fragColor = vertexColor;
        fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
        gl_Position = mvp * vec4(vertexPosition, 1.0);
    }
}
