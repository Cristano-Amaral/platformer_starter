#version 330

// Milestone 85 world lit fragment shader. Staged runtime asset.
// lightRayDirection is the direction rays travel. NdotL uses -lightRayDirection.

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D shadowMap;
uniform vec4 colDiffuse;
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

void main()
{
    vec4 texel = texture(texture0, fragTexCoord);
    vec4 albedo = texel * colDiffuse * fragColor;
    vec3 normal = normalize(fragNormal);
    vec3 toLight = normalize(-lightRayDirection);
    float nDotL = max(dot(normal, toLight), 0.0);
    float visibility = (shadowsEnabled > 0.5) ? ShadowVisibility(fragPosition, nDotL) : 1.0;
    float directionalScale = (lightEnabled > 0.5) ? 1.0 : 0.0;

    vec3 ambient = albedo.rgb * ambientColor * ambientIntensity;
    vec3 directional = albedo.rgb * lightColor * lightIntensity * nDotL * visibility * directionalScale;
    finalColor = vec4(ambient + directional, albedo.a);
}
