#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;

uniform mat4 mvp;         // Model-View-Projection matrix
uniform mat4 matModel;    // Model matrix to transform normals
uniform mat4 matNormal;   // Inverse transpose of matModel
uniform vec3 lightPosition; // Light source position

out vec3 fragNormal;
out vec3 fragPosition;
out vec3 fragLightDir;    // Vector from vertex to light

void main()
{
    vec4 worldPos = matModel * vec4(vertexPosition, 1.0);
    fragPosition = worldPos.xyz;

    fragNormal = mat3(matNormal) * vertexNormal;  // Transform normal correctly
    gl_Position = mvp * vec4(vertexPosition, 1.0);

    // Calculate the vector from the vertex to the light source.
    fragLightDir = lightPosition - worldPos.xyz;
}
