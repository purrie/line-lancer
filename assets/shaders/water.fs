precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float time;
uniform vec2 size;

void main() {
    float displace_y = fragTexCoord.y - time * 0.1;
    float displace_x = fragTexCoord.x - time * 0.1;

    displace_y = displace_y * 20.0;
    displace_x = displace_x * 20.0;

    displace_y = sin(time * 0.9 + displace_x) * cos(time) * 0.05 + time * 0.001;
    displace_x = sin(time + displace_y) * cos(1.0 / time) * 0.05 - time * 0.005;

    float posx = fragTexCoord.x * (size.x / 64.0);
    float posy = fragTexCoord.y * (size.y / 64.0);
    posx = posx - displace_x;
    posy = posy - displace_y;

    gl_FragColor = texture2D(texture0, vec2(posx, posy)) * colDiffuse * fragColor;
}
