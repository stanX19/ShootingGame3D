#version 330

in vec3 fragNormal;
in vec3 fragPosition;

uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec4 objectColor;

out vec4 finalColor;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(-lightDirection); // Direction **to** light

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightColor * diff;
    vec3 ambient = ambientColor;

    vec3 result = (ambient + diffuse) * objectColor.rgb;
    finalColor = vec4(result, objectColor.a);
}
