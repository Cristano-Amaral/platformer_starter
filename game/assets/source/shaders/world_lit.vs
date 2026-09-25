#version 330

// Milestone 85 world lit vertex shader. Staged runtime asset.
// Milestone 94: vegetationInstanced selects per-instance transforms.
// Direction convention: CPU stores light *ray travel* direction.

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in mat4 instanceTransform;
in vec4 vertexBoneIndices;
in vec4 vertexBoneWeights;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
// 0 keeps the single-model path used by Terrain, props, and greybox meshes.
// Vegetation sets 1 so each instance supplies its own transform.
uniform int vegetationInstanced;
uniform int skinningEnabled;
uniform mat4 boneMatrices[64];

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec4 fragColor;

void main()
{
    vec3 localPosition = vertexPosition;
    vec3 localNormal = vertexNormal;
    if (skinningEnabled != 0)
    {
        mat4 skin = boneMatrices[int(vertexBoneIndices.x)] * vertexBoneWeights.x
            + boneMatrices[int(vertexBoneIndices.y)] * vertexBoneWeights.y
            + boneMatrices[int(vertexBoneIndices.z)] * vertexBoneWeights.z
            + boneMatrices[int(vertexBoneIndices.w)] * vertexBoneWeights.w;
        localPosition = vec3(skin * vec4(vertexPosition, 1.0));
        localNormal = normalize(mat3(skin) * vertexNormal);
    }
    if (vegetationInstanced != 0)
    {
        mat4 model = instanceTransform;
        fragPosition = vec3(model * vec4(localPosition, 1.0));
        fragTexCoord = vertexTexCoord;
        fragColor = vertexColor;
        fragNormal = normalize(transpose(inverse(mat3(model))) * localNormal);
        gl_Position = mvp * model * vec4(localPosition, 1.0);
    }
    else
    {
        fragPosition = vec3(matModel * vec4(localPosition, 1.0));
        fragTexCoord = vertexTexCoord;
        fragColor = vertexColor;
        fragNormal = normalize(vec3(matNormal * vec4(localNormal, 0.0)));
        gl_Position = mvp * vec4(localPosition, 1.0);
    }
}
