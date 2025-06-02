#version 450

in vec2 TexCoord;
in vec3 Color;

out vec4 FragColor;

uniform sampler2D texture0;
uniform bool useTexture;

void main()
{
    vec4 texColor = texture(texture0, TexCoord);
    
    // Se a textura for totalmente transparente, descarta o fragmento
    if(texColor.a < 0.1)
        discard;
        
    if(useTexture)
    {
        FragColor = texColor;
    }
    else
    {
        FragColor = vec4(Color, 1.0);
    }
} 