#version 330

// Milestone 85 directional shadow depth vertex shader. Staged runtime asset.
// Milestone 94: vegetationInstanced selects per-instance transforms.

in vec3 vertexPosition;
in mat4 instanceTransform;
in vec4 vertexBoneIndices;
in vec4 vertexBoneWeights;

uniform mat4 mvp;
// 0 keeps the single-model depth path. Vegetation sets 1.
uniform int vegetationInstanced;
uniform int skinningEnabled;
uniform mat4 boneMatrices[64];

void main()
{
    vec3 localPosition = vertexPosition;
    if (skinningEnabled != 0)
    {
        mat4 skin = boneMatrices[int(vertexBoneIndices.x)] * vertexBoneWeights.x
            + boneMatrices[int(vertexBoneIndices.y)] * vertexBoneWeights.y
            + boneMatrices[int(vertexBoneIndices.z)] * vertexBoneWeights.z
            + boneMatrices[int(vertexBoneIndices.w)] * vertexBoneWeights.w;
        localPosition = vec3(skin * vec4(vertexPosition, 1.0));
    }
    if (vegetationInstanced != 0)
    {
        gl_Position = mvp * instanceTransform * vec4(localPosition, 1.0);
    }
    else
    {
        gl_Position = mvp * vec4(localPosition, 1.0);
    }
}
