#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;  // This is essential for textured models

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform vec3 lightPosition;

out vec3 fragNormal;
out vec3 fragPosition;
out vec3 fragLightDir;
out vec2 fragTexCoord;   // Pass texture coordinates to fragment shader

void main() {
    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);
    fragPosition = worldPos.xyz;
    
    fragNormal = mat3(matNormal) * vertexNormal;
    fragTexCoord = vertexTexCoord;  // Pass through texture coordinates
    
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    fragLightDir = lightPosition - worldPos.xyz;
}