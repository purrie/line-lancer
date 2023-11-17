#version 100

precision mediump float;

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float time;
uniform vec2 size;

void main() {
    float displace_y = fragTexCoord.y - time * 0.1;
    float displace_x = fragTexCoord.x - time * 0.1;
    displace_y = displace_y * 20.0;
    displace_x = displace_x * 20.0;

    vec2 scale = size / vec2(64);
    vec2 p = fragTexCoord * scale;
    p.x -= sin(time + displace_y) * cos(1.0/time) * 0.01 - time * 0.005;
    p.y -= sin(time * 0.9 + displace_x) * cos(time) * 0.01 + time * 0.001;

    gl_FragColor = texture2D(texture0, p) * colDiffuse * fragColor;
}
