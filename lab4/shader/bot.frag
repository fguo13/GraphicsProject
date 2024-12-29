#version 330 core

in vec3 worldPosition;
in vec3 worldNormal;
in vec2 texCoords;

out vec3 finalColor;

uniform vec3 lightPosition;
uniform vec3 lightIntensity;
uniform sampler2D textureSampler;
uniform vec3 material;
uniform bool useTexture;

void main()
{
vec3 textureColor;
if(useTexture)
	{
	    textureColor = pow(texture(textureSampler, texCoords).rgb, vec3(2.2));
	    if(texture(textureSampler, texCoords).a < 0.5)
	    {
	        discard;
	    }
	}
	else
	{
	    textureColor = material;
	}
	// Lighting
	vec3 lightDir = lightPosition - worldPosition;
	float lightDist = dot(lightDir, lightDir);
	lightDir = normalize(lightDir);
	vec3 v = textureColor * lightIntensity * clamp(dot(lightDir, worldNormal), 0.0, 1.0) / lightDist;

	// Tone mapping
	v = v / (1.0 + v);

	// Gamma correction
	finalColor = pow(v, vec3(1.0 / 2.2));

}
