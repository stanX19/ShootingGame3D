#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec3 fragLightDir;

out vec4 finalColor;

uniform vec3 lightColor;
uniform vec4 colDiffuse;
uniform sampler2D texture0;
uniform sampler2D texture2;
uniform int normalMapAvailable;

vec3 getSurfaceNormal(vec3 geometricNormal) {
	if (normalMapAvailable == 0)
		return geometricNormal;

	vec3 positionDx = dFdx(fragPosition);
	vec3 positionDy = dFdy(fragPosition);
	vec2 texcoordDx = dFdx(fragTexCoord);
	vec2 texcoordDy = dFdy(fragTexCoord);
	float determinant = texcoordDx.x * texcoordDy.y - texcoordDx.y * texcoordDy.x;
	if (abs(determinant) < 0.000000000001)
		return geometricNormal;

	vec3 tangent = normalize(positionDx * texcoordDy.y - positionDy * texcoordDx.y);
	tangent = normalize(tangent - geometricNormal * dot(geometricNormal, tangent));
	vec3 bitangent = normalize(cross(geometricNormal, tangent));
	vec3 tangentNormal = texture(texture2, fragTexCoord).rgb * 2.0 - 1.0;
	return normalize(tangent * tangentNormal.x + bitangent * tangentNormal.y + geometricNormal * tangentNormal.z);
}

void main() {
	vec3 norm = getSurfaceNormal(normalize(fragNormal));
	vec3 lightDir = normalize(fragLightDir);

	vec4 texelColor = texture(texture0, fragTexCoord);

	vec4 baseColor = texelColor * colDiffuse;

	float diff = max(dot(norm, lightDir), 0.0);  // how much direct light
	vec3 diffuse = diff * lightColor;

	vec3 ambientLight = 0.1 * lightColor;

	vec3 lightingEffect = (ambientLight + diffuse) * baseColor.rgb;
	vec3 result = mix(baseColor.rgb, lightingEffect, 0.6);  // 60% original color, 40% lighting 

	finalColor = vec4(result, baseColor.a);
}
