#version 330

in vec3 fragNormal;
in vec3 fragPosition;
in vec3 fragLightDir;

out vec4 finalColor;

uniform vec3 lightColor; 

uniform vec4 colDiffuse;         // Object base color from material

void main()
{
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(fragLightDir); // Light comes *from* direction

    // Diffuse shading
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Ambient
    vec3 ambientLight = 0.1 * lightColor;
    vec3 ambientBackground = 0.4 * vec3(1.0, 1.0, 1.0);

    // Final color
	vec3 lightingEffect = (ambientLight + diffuse) * colDiffuse.rgb;
    vec3 result = mix(colDiffuse.rgb, lightingEffect, 0.6);
	finalColor = vec4(result, colDiffuse.a);
}
