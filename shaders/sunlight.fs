#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragLightDir;

out vec4 finalColor;

uniform vec3 lightColor;
uniform vec4 colDiffuse;
uniform sampler2D texture0;

void main() {
	vec3 norm = normalize(fragNormal);
	vec3 lightDir = normalize(fragLightDir);

	vec4 texelColor = texture(texture0, fragTexCoord);

	vec4 baseColor = mix(texelColor * colDiffuse, colDiffuse, 0.5);

	float diff = max(dot(norm, lightDir), 0.0);  // how much direct light
	vec3 diffuse = diff * lightColor;

	vec3 ambientLight = 0.1 * lightColor;

	vec3 lightingEffect = (ambientLight + diffuse) * baseColor.rgb;
	vec3 result = mix(baseColor.rgb, lightingEffect, 0.6);  // 60% original color, 40% lighting 

	finalColor = vec4(result, baseColor.a);
}