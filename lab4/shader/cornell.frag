#version 330 core

// Inputs from the vertex shader
in vec3 color;
in vec3 worldPosition;
in vec3 worldNormal;

// Output color
out vec3 finalColor;

// Uniforms for light position and intensity
uniform vec3 lightPosition;
uniform vec3 lightIntensity;

void main()
{
    // Normalize the normal vector for accurate lighting calculation
    vec3 normal = normalize(worldNormal);

    // Calculate the direction vector from the fragment position to the light source
    vec3 lightDir = normalize(lightPosition - worldPosition);

    // Calculate the Lambertian reflectance (diffuse) component
    float cosTheta = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = color * lightIntensity * cosTheta;

    // Tone mapping (Reinhard)
    vec3 mappedColor = diffuse / (1.0 + diffuse);

    // Gamma correction
    float gamma = 2.2;
    vec3 gammaCorrectedColor = pow(mappedColor, vec3(1.0 / gamma));

    // Set the final color output
    finalColor = gammaCorrectedColor;
}
