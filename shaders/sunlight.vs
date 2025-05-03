#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

// Input uniform values
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec4 objectColor;

// Output fragment color
out vec4 finalColor;

void main() {
    // Normalize the normal vector for lighting calculations
    vec3 normal = normalize(fragNormal);
    
    // Normalize the light direction vector
    vec3 lightDir = normalize(-lightDirection);
    
    // Calculate diffuse lighting (Lambert) with more pronounced effect
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Apply a stronger lighting effect for better visibility
    float strengthenedDiff = diff * 1.5; // Increase lighting intensity
    
    // Use a higher ambient lighting value for better visibility on shadowed parts
    vec3 ambient = ambientColor * 0.5; // Increased from original value
    
    // Combine ambient and diffuse lighting with a brighter result
    vec3 lighting = ambient + strengthenedDiff * lightColor;
    
    // Use object color if available, otherwise use vertex color
    vec4 baseColor = objectColor.a > 0.0 ? objectColor : fragColor;
    
    // Apply lighting to the base color
    vec3 litColor = baseColor.rgb * lighting;
    
    // Add a subtle specular highlight for extra visual interest
    float specularStrength = 0.5;
    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0) - fragPosition); // Simplified view direction
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine diffuse and specular lighting
    finalColor = vec4(litColor + specular, baseColor.a);
}