#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in mat4 instanceTransform;

uniform mat4 mvp;
uniform vec3 lightPosition;

out vec3 fragNormal;
out vec3 fragPosition;
out vec3 fragLightDir;
out vec2 fragTexCoord;

void main() {
    vec4 worldPos = instanceTransform * vec4(vertexPosition, 1.0);
    fragPosition = worldPos.xyz;
    
    fragNormal = mat3(instanceTransform) * vertexNormal;
    fragTexCoord = vertexTexCoord;
    
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
    fragLightDir = lightPosition - worldPos.xyz;
}
