#version 330

// Input vertex attributes (from vertex shader)
in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

// Input uniform values
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec4 objectColor;  // Object color uniform

// Output fragment color
out vec4 finalColor;

void main() {
    // Normalize the normal vector for lighting calculations
    vec3 normal = normalize(fragNormal);
    
    // Normalize the light direction
    vec3 lightDir = normalize(-lightDirection);
    
    // Calculate diffuse lighting (Lambert) - max prevents negative values
    float diff = max(dot(normal, lightDir), 0.0);
    
    // Combine ambient and diffuse lighting
    vec3 lighting = ambientColor + diff * lightColor;
    
    // Apply lighting to object color
    // Use a mix between object color and vertex color to ensure visibility
    vec4 color = objectColor.a > 0.0 ? objectColor : fragColor;
    
    // Ensure object is visible by adding a minimum brightness
    vec3 litColor = color.rgb * max(lighting, vec3(0.2));
    
    // Set the final color with original alpha
    finalColor = vec4(litColor, color.a);
    
    // Debug: uncomment to see objects in pure red
    // finalColor = vec4(1.0, 0.0, 0.0, 1.0);
}