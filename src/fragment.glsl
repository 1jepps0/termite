#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;
uniform bool solid;   // true for cursor / overlays

void main()
{
    if (solid) {
        // ignore texture lookup
        color = vec4(textColor, 1.0);
    } else {
        float alpha = texture(text, TexCoords).r;
        color = vec4(textColor, 1.0) * vec4(1.0, 1.0, 1.0, alpha);
    }
}


