#version 450

in vec2 TexCoord;
in vec3 Color;

out vec4 FragColor;

uniform sampler2D texture1;
uniform bool useTexture;

void main()
{
    if (useTexture)
        FragColor = texture(texture1, TexCoord);
    else
        FragColor = vec4(Color, 1.0);
} 