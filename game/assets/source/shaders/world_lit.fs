#version 330

// Milestone 85 world lit fragment shader. Staged runtime asset.
// lightRayDirection is the direction rays travel. NdotL uses -lightRayDirection.
// Milestone 97: Terrain-only tangent-space Normal blend and bounded roughness
// specular. Non-Terrain objects keep the existing Lambert path.

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
// Terrain albedo/normal/roughness and weight maps are sampler2DArray on units
// DrawMesh does not claim. texture1 is the directional shadow depth map.
uniform sampler2DArray terrainAlbedo;
uniform sampler2DArray terrainNormal;
uniform sampler2DArray terrainRoughness;
uniform sampler2DArray terrainWeights;
uniform sampler2D shadowMap;
uniform vec4 colDiffuse;
uniform int terrainLayerCount;
uniform int terrainWeightMapCount;
uniform vec2 terrainOriginXZ;
uniform vec2 terrainSizeXZ;
uniform vec2 terrainWeightResolution;
uniform float terrainLayerTiling[16];
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
uniform int groundCoverCutout;
uniform vec3 viewPos;

out vec4 finalColor;

const float kDefaultTerrainRoughness = 0.72;
const float kMinTerrainRoughness = 0.04;
const float kTerrainSpecularIntensity = 0.18;
const float kTerrainSpecularPowerMin = 8.0;
const float kTerrainSpecularPowerMax = 128.0;

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

vec3 TerrainWorldNormal(vec3 geometricNormal, vec3 tangentNormal)
{
    vec3 N = normalize(geometricNormal);
    vec3 tangent = vec3(1.0, 0.0, 0.0) - N * N.x;
    if (dot(tangent, tangent) < 1.0e-8)
    {
        tangent = vec3(0.0, 0.0, 1.0) - N * N.z;
    }
    vec3 T = normalize(tangent);
    vec3 B = normalize(cross(T, N));
    vec3 world = T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z;
    float lenSq = dot(world, world);
    if (lenSq < 1.0e-12)
    {
        return N;
    }
    return world * inversesqrt(lenSq);
}

void SampleTerrainMaterial(out vec4 albedoTexel, out vec3 tangentNormal, out float roughness)
{
    vec2 delta = vec2(fragPosition.x - terrainOriginXZ.x, fragPosition.z - terrainOriginXZ.y);
    float u = terrainSizeXZ.x > 0.0 ? delta.x / terrainSizeXZ.x : 0.0;
    float v = terrainSizeXZ.y > 0.0 ? delta.y / terrainSizeXZ.y : 0.0;
    u = clamp(u, 0.0, 1.0);
    v = clamp(v, 0.0, 1.0);
    vec2 weightUv = vec2(u, v) * ((terrainWeightResolution - vec2(1.0)) / terrainWeightResolution)
        + (vec2(0.5) / terrainWeightResolution);

    int maps = terrainWeightMapCount;
    if (maps < 1)
    {
        maps = 1;
    }
    if (maps > 4)
    {
        maps = 4;
    }

    vec4 texel = vec4(0.0);
    vec3 normalSum = vec3(0.0);
    float roughSum = 0.0;
    float weightSum = 0.0;
    for (int mapIndex = 0; mapIndex < 4; ++mapIndex)
    {
        if (mapIndex >= maps)
        {
            break;
        }
        vec4 weightSample = texture(terrainWeights, vec3(weightUv, float(mapIndex)));
        float channels[4];
        channels[0] = weightSample.r;
        channels[1] = weightSample.g;
        channels[2] = weightSample.b;
        channels[3] = weightSample.a;
        for (int channel = 0; channel < 4; ++channel)
        {
            int layer = mapIndex * 4 + channel;
            if (layer >= terrainLayerCount || layer >= 16)
            {
                continue;
            }
            float weight = channels[channel];
            weightSum += weight;
            vec3 uvw = vec3(delta * terrainLayerTiling[layer], float(layer));
            texel += texture(terrainAlbedo, uvw) * weight;
            vec3 encoded = texture(terrainNormal, uvw).xyz;
            vec3 ts = encoded * 2.0 - 1.0;
            float tsLenSq = dot(ts, ts);
            if (tsLenSq > 1.0e-12)
            {
                ts *= inversesqrt(tsLenSq);
            }
            else
            {
                ts = vec3(0.0, 0.0, 1.0);
            }
            normalSum += ts * weight;
            // Roughness is a scalar map. Sample the red channel of the PNG.
            roughSum += texture(terrainRoughness, uvw).r * weight;
        }
    }
    if (weightSum > 1.0e-6)
    {
        albedoTexel = texel / weightSum;
        vec3 blended = normalSum / weightSum;
        float blendLenSq = dot(blended, blended);
        tangentNormal = (blendLenSq > 1.0e-12) ? blended * inversesqrt(blendLenSq) : vec3(0.0, 0.0, 1.0);
        roughness = clamp(roughSum / weightSum, kMinTerrainRoughness, 1.0);
    }
    else
    {
        albedoTexel = texel;
        tangentNormal = vec3(0.0, 0.0, 1.0);
        roughness = kDefaultTerrainRoughness;
    }
}

void main()
{
    vec4 texel;
    vec4 albedo;
    vec3 normal = normalize(fragNormal);
    float roughness = kDefaultTerrainRoughness;
    bool terrainSurface = terrainLayerCount > 0;
    if (terrainSurface)
    {
        // Terrain blend weights live in terrainWeights, not in fragColor.
        // Vertex color must not tint the splat.
        vec3 tangentNormal;
        SampleTerrainMaterial(texel, tangentNormal, roughness);
        albedo = texel * colDiffuse;
        normal = TerrainWorldNormal(fragNormal, tangentNormal);
    }
    else
    {
        texel = texture(texture0, fragTexCoord);
        albedo = texel * colDiffuse * fragColor;
        if (groundCoverCutout != 0 && texel.a < 0.5)
        {
            discard;
        }
    }
    vec3 toLight = normalize(-lightRayDirection);
    float nDotL = max(dot(normal, toLight), 0.0);
    float visibility = (shadowsEnabled > 0.5) ? ShadowVisibility(fragPosition, nDotL) : 1.0;
    float directionalScale = (lightEnabled > 0.5) ? 1.0 : 0.0;

    vec3 ambient = albedo.rgb * ambientColor * ambientIntensity;
    vec3 directional = albedo.rgb * lightColor * lightIntensity * nDotL * visibility * directionalScale;
    vec3 viewDir = vec3(0.0, 1.0, 0.0);
    float specPower = kTerrainSpecularPowerMin;
    float specIntensity = 0.0;
    if (terrainSurface)
    {
        viewDir = normalize(viewPos - fragPosition);
        float gloss = 1.0 - roughness;
        float glossSq = gloss * gloss;
        specPower = mix(kTerrainSpecularPowerMin, kTerrainSpecularPowerMax, glossSq);
        specIntensity = kTerrainSpecularIntensity * glossSq;
        vec3 halfVec = normalize(toLight + viewDir);
        float spec = pow(max(dot(normal, halfVec), 0.0), specPower) * specIntensity * nDotL;
        directional += lightColor * lightIntensity * spec * visibility * directionalScale;
    }
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
        vec3 localDiffuse =
            albedo.rgb * colorIntensity.rgb * colorIntensity.a * localNdotL * distAtt * angular;
        vec3 localSpec = vec3(0.0);
        if (terrainSurface && specIntensity > 0.0)
        {
            vec3 localHalf = normalize(toLocalLight + viewDir);
            float spec =
                pow(max(dot(normal, localHalf), 0.0), specPower) * specIntensity * localNdotL;
            localSpec = colorIntensity.rgb * colorIntensity.a * spec * distAtt * angular;
        }
        local += localDiffuse + localSpec;
    }
    finalColor = vec4(ambient + directional + local, albedo.a);
}
