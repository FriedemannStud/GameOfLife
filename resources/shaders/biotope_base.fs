#version 330

in vec2 fragTexCoord;
out vec4 finalColor;
uniform sampler2D texture0;
uniform sampler2D previousFrame;
uniform float fadeRate;

const vec4 COLOR_RED = vec4(1.0, 0.23, 0.39, 1.0);
const vec4 COLOR_BLUE = vec4(0.0, 0.86, 1.0, 1.0);
const vec4 COLOR_BG = vec4(0.08, 0.09, 0.12, 1.0);

void main() {
    float state = texture(texture0, fragTexCoord).r;
    
    // Invert Y coordinate for the FBO texture to fix OpenGL upside-down rendering
    vec2 prevTexCoord = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);
    vec4 prevColor = texture(previousFrame, prevTexCoord);
    
    vec4 currentColor = COLOR_BG;
    if (state > 0.9) {
        currentColor = COLOR_RED;
    } else if (state > 0.4) {
        currentColor = COLOR_BLUE;
    } else {
        // Temporal trails (fossils)
        currentColor = max(COLOR_BG, prevColor * fadeRate);
    }

    finalColor = currentColor;
}