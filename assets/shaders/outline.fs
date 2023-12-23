precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main () {
    vec4 my_color = texture2D(texture0, fragTexCoord);
    float size = 64.0;
    float scale = 0.2 / size;

    vec2 corner1 = fragTexCoord + vec2(scale);
    vec2 corner2 = fragTexCoord + vec2(-scale);
    vec2 corner3 = fragTexCoord + vec2(scale, -scale);
    vec2 corner4 = fragTexCoord + vec2(-scale, scale);

    vec4 around = vec4(0.0);
    around.x = texture2D(texture0, corner1).a;
    around.y = texture2D(texture0, corner2).a;
    around.z = texture2D(texture0, corner3).a;
    around.w = texture2D(texture0, corner4).a;

    float outline = min(around.x + around.y + around.z + around.w, 1.0);
    vec4 color = mix(vec4(0.0), fragColor, outline);
    gl_FragColor = mix(color, my_color, my_color.a);
}
