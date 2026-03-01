#version 330 core

in vec2 v_texCoord;
in vec3 v_normal;
in vec3 v_fragPos;

out vec4 FragColor;

void main() {
    // Visualize normals as RGB color — a standard debugging technique.
    //
    // Normals are in range [-1, 1]. We remap to [0, 1] for display:
    //   normal.x → red channel   (left/right orientation)
    //   normal.y → green channel (up/down orientation)
    //   normal.z → blue channel  (front/back orientation)
    //
    // What you'll see:
    //   Faces pointing right  → reddish
    //   Faces pointing up     → greenish
    //   Faces pointing toward camera → bluish
    //
    // This is the fastest way to verify your normals are correct
    // before adding real lighting. If normals are wrong, lighting
    // will look broken — catching it here saves debugging later.

    vec3 normalColor = normalize(v_normal) * 0.5 + 0.5;
    FragColor = vec4(normalColor, 1.0);
}
