#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_lightSpaceMatrix;  // light view * light projection, for shadow mapping

out vec2 v_texCoord;
out vec3 v_normal;
out vec3 v_fragPos;
out vec3 v_tangent;
out vec3 v_bitangent;
out vec4 v_fragPosLightSpace;  // world position transformed into light clip space

void main() {
    // Transform position to world space first, then to clip space.
    // We need the world space position in the fragment shader for
    // lighting — distance to light, direction to camera, etc.
    vec4 worldPos = u_model * vec4(a_position, 1.0);
    gl_Position   = u_projection * u_view * worldPos;

    v_texCoord = a_texCoord;
    v_fragPos  = vec3(worldPos);

    // Transform world position into light's clip space.
    // The fragment shader will use this for shadow map lookup.
    v_fragPosLightSpace = u_lightSpaceMatrix * worldPos;

    // Normal matrix: transpose of inverse of model matrix.
    // Correctly handles non-uniform scaling — if you scale X by 2
    // and Y by 1, normals would point wrong without this correction.
    mat3 normalMatrix = mat3(transpose(inverse(u_model)));
    v_normal    = normalize(normalMatrix * a_normal);
    v_tangent   = normalize(normalMatrix * a_tangent);
    v_bitangent = normalize(normalMatrix * a_bitangent);
}
