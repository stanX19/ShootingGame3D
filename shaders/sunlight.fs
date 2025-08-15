#version 330

in vec3 fragNormal;
in vec3 fragPosition;
in vec3 fragLightDir;
in vec2 fragTexCoord;

out vec4 finalColor;

uniform vec3 lightColor;
uniform vec4 colDiffuse;
uniform sampler2D texture0;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(fragLightDir);
    
    // Sample the texture - this is crucial!
    vec4 texelColor = texture(texture0, fragTexCoord);
    
    // Combine texture with material color
    vec4 baseColor = texelColor * colDiffuse;
    
    // Diffuse shading
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Ambient lighting
    vec3 ambientLight = 0.1 * lightColor;
    vec3 ambientBackground = 0.4 * vec3(1.0, 1.0, 1.0);
    
    // Apply lighting to the textured base color
    vec3 lightingEffect = (ambientLight + diffuse) * baseColor.rgb;
    vec3 result = mix(baseColor.rgb, lightingEffect, 0.6);
    
    finalColor = vec4(result, baseColor.a);
}