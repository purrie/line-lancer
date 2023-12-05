#version 100

precision mediump float;

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main () {
    vec4 my_color = texture2D(texture0, fragTexCoord);
    float size = 64.0;
    float scale = 0.1 / size;

    vec4 neighbors = vec4(0.0);
    neighbors.x = texture2D(texture0, fragTexCoord + vec2(scale)).a;
    neighbors.y = texture2D(texture0, fragTexCoord + vec2(-scale)).a;
    neighbors.z = texture2D(texture0, fragTexCoord + vec2(scale, -scale)).a;
    neighbors.w = texture2D(texture0, fragTexCoord + vec2(-scale, scale)).a;

    float outline = min(dot(neighbors, vec4(1.0)), 1.0);
    vec4 color = mix(vec4(0.0), fragColor, outline);
    gl_FragColor = mix(color, my_color, my_color.a);
}
