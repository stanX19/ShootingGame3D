#version 330

in vec3 fragNormal;
in vec3 fragPos;

out vec4 fragColor;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec4 objectColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(-lightDirection);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 lighting = ambientColor + diff * lightColor;

    fragColor = vec4(lighting * objectColor.rgb, objectColor.a);
}
