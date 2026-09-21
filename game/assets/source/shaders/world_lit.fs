#version 330

// Milestone 85 world lit fragment shader. Staged runtime asset.
// lightRayDirection is the direction rays travel. NdotL uses -lightRayDirection.

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
// Extra Terrain albedo layers must NOT use texture1..texture3. raylib
// DrawMesh binds MATERIAL_MAP index i to texture unit i and writes those
// sampler uniforms: texture1 becomes the shadow depth map on unit 1.
uniform sampler2D terrainAlbedo1;
uniform sampler2D terrainAlbedo2;
uniform sampler2D terrainAlbedo3;
uniform sampler2D shadowMap;
uniform vec4 colDiffuse;
uniform int terrainLayerCount;
uniform vec2 terrainOriginXZ;
uniform vec4 terrainLayerTiling;
uniform vec3 ambientColor;
uniform float ambientIntensity;
uniform vec3 lightRayDirection;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float lightEnabled;
uniform float shadowsEnabled;
uniform mat4 lightVP;
uniform float shadowBias;
uniform int shadowMapResolution;
uniform int localLightCount;
uniform vec4 localLightPosRange[8];
uniform vec4 localLightColorIntensity[8];
uniform vec4 localLightDirType[8];
uniform vec4 localLightConeCos[8];

out vec4 finalColor;

float ShadowVisibility(vec3 worldPosition, float nDotL)
{
    vec4 lightSpace = lightVP * vec4(worldPosition, 1.0);
    if (lightSpace.w <= 0.0)
    {
        return 1.0;
    }
    vec3 projected = lightSpace.xyz / lightSpace.w;
    projected = projected * 0.5 + 0.5;
    if (projected.x <= 0.0 || projected.x >= 1.0 || projected.y <= 0.0 || projected.y >= 1.0
        || projected.z <= 0.0 || projected.z >= 1.0)
    {
        return 1.0;
    }

    float bias = max(shadowBias * (1.0 - nDotL), shadowBias * 0.35);
    float texel = 1.0 / float(shadowMapResolution);
    float shadowed = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            vec2 sampleUv = projected.xy + vec2(float(x), float(y)) * texel;
            float closest = texture(shadowMap, sampleUv).r;
            shadowed += (projected.z - bias > closest) ? 1.0 : 0.0;
        }
    }
    return 1.0 - (shadowed / 9.0);
}

vec4 SampleTerrainAlbedo()
{
    vec2 delta = vec2(fragPosition.x - terrainOriginXZ.x, fragPosition.z - terrainOriginXZ.y);
    vec4 weights = fragColor;
    float weightSum = weights.r + weights.g + weights.b + weights.a;
    if (weightSum > 1.0e-6)
    {
        weights /= weightSum;
    }
    else
    {
        weights = vec4(1.0, 0.0, 0.0, 0.0);
    }

    vec4 texel = texture(texture0, delta * terrainLayerTiling.x) * weights.r;
    if (terrainLayerCount > 1)
    {
        texel += texture(terrainAlbedo1, delta * terrainLayerTiling.y) * weights.g;
    }
    if (terrainLayerCount > 2)
    {
        texel += texture(terrainAlbedo2, delta * terrainLayerTiling.z) * weights.b;
    }
    if (terrainLayerCount > 3)
    {
        texel += texture(terrainAlbedo3, delta * terrainLayerTiling.w) * weights.a;
    }
    return texel;
}

void main()
{
    vec4 texel;
    vec4 albedo;
    if (terrainLayerCount > 0)
    {
        // fragColor is Terrain material weights (R,G,B,A -> layers 0..3),
        // not an ordinary mesh tint.
        texel = SampleTerrainAlbedo();
        albedo = texel * colDiffuse;
    }
    else
    {
        texel = texture(texture0, fragTexCoord);
        albedo = texel * colDiffuse * fragColor;
    }
    vec3 normal = normalize(fragNormal);
    vec3 toLight = normalize(-lightRayDirection);
    float nDotL = max(dot(normal, toLight), 0.0);
    float visibility = (shadowsEnabled > 0.5) ? ShadowVisibility(fragPosition, nDotL) : 1.0;
    float directionalScale = (lightEnabled > 0.5) ? 1.0 : 0.0;

    vec3 ambient = albedo.rgb * ambientColor * ambientIntensity;
    vec3 directional = albedo.rgb * lightColor * lightIntensity * nDotL * visibility * directionalScale;
    vec3 local = vec3(0.0);
    int count = localLightCount;
    if (count > 8)
    {
        count = 8;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (i >= count)
        {
            break;
        }
        vec4 posRange = localLightPosRange[i];
        vec4 colorIntensity = localLightColorIntensity[i];
        vec4 dirType = localLightDirType[i];
        vec4 cone = localLightConeCos[i];
        vec3 toFrag = fragPosition - posRange.xyz;
        float dist = length(toFrag);
        float range = posRange.w;
        float distAtt = 0.0;
        if (range > 0.0 && dist < range)
        {
            float falloff = 1.0 - (dist / range);
            distAtt = falloff * falloff;
        }
        if (distAtt <= 0.0)
        {
            continue;
        }
        vec3 toLocalLight = normalize(posRange.xyz - fragPosition);
        float localNdotL = max(dot(normal, toLocalLight), 0.0);
        float angular = 1.0;
        if (dirType.w > 1.5)
        {
            vec3 spotAxis = normalize(dirType.xyz);
            vec3 lightToFrag = (dist > 1.0e-8) ? (toFrag / dist) : spotAxis;
            float cosTheta = dot(lightToFrag, spotAxis);
            float innerCos = cone.x;
            float outerCos = cone.y;
            if (cosTheta <= outerCos)
            {
                angular = 0.0;
            }
            else if (cosTheta < innerCos)
            {
                float denom = innerCos - outerCos;
                angular = (denom > 1.0e-5) ? ((cosTheta - outerCos) / denom) : 1.0;
            }
        }
        local += albedo.rgb * colorIntensity.rgb * colorIntensity.a * localNdotL * distAtt * angular;
    }
    finalColor = vec4(ambient + directional + local, albedo.a);
}
