#version 330 core

in vec2 TexCoords;
in vec4 worldPosition;
out vec4 FragColor;

uniform sampler2D textureSampler;
uniform mat4 lightMVP;
uniform sampler2D shadowMap;



void main() {
vec4 fragPosLightSpace = lightMVP * vec4(worldPosition);
    FragColor = texture(textureSampler, TexCoords); // Sample the repeated texture
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        // get closest depth value from light perspective
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        // get depth of current fragment from light's perspective
        float currentDepth = projCoords.z;
        // check whether current frag is in shadow
        float shadow = currentDepth - 0.0001 > closestDepth  ? 1.0 : 0.0;
        if(projCoords.z > 1.0)
                shadow = 0.0;




    FragColor.rbg = vec3(shadow);
    FragColor.rgb = vec3(closestDepth); //欧若拉
    //FragColor = texture(textureSampler, TexCoords) * (1 - shadow); // Sample the repeated texture

}
