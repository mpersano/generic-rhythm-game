#version 450 core

layout(points) in;
layout(triangle_strip, max_vertices=4) out;

uniform vec3 eye;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

in WorldVertex {
    vec3 position;
    vec3 velocity;
    vec2 size;
    float alpha;
} gs_in[];

out SpriteVertex {
    vec2 texCoord;
    float alpha;
} gs_out;

void main(void)
{
    vec3 worldPosition = vec3(modelMatrix * vec4(gs_in[0].position, 1));
    vec3 worldVelocity = normalMatrix * gs_in[0].velocity;

    vec2 size = gs_in[0].size;
    float alpha = gs_in[0].alpha;

    vec3 velVec = normalize(worldVelocity);
    vec3 eyeVec = eye - worldPosition;
    vec3 eyeOnVelVecPlane = eye - dot(eyeVec, velVec) * velVec;
    vec3 projectedEyeVec = eyeOnVelVecPlane - worldPosition;
    vec3 sideVec = normalize(cross(projectedEyeVec, velVec));

    vec3 spriteVerts[4];
    spriteVerts[0] = worldPosition - sideVec * 0.5 * size.x + velVec * 0.5 * size.y;
    spriteVerts[1] = spriteVerts[0] + sideVec * size.x;
    spriteVerts[2] = spriteVerts[0] + velVec * size.y;
    spriteVerts[3] = spriteVerts[1] + velVec * size.y;

    mat4 viewProjectionMatrix = projectionMatrix * viewMatrix;

    gl_Position = viewProjectionMatrix * vec4(spriteVerts[0], 1);
    gs_out.texCoord = vec2(0, 0);
    gs_out.alpha = alpha;
    EmitVertex();

    gl_Position = viewProjectionMatrix * vec4(spriteVerts[1], 1);
    gs_out.texCoord = vec2(1, 0);
    gs_out.alpha = alpha;
    EmitVertex();

    gl_Position = viewProjectionMatrix * vec4(spriteVerts[2], 1);
    gs_out.texCoord = vec2(0, 1);
    gs_out.alpha = alpha;
    EmitVertex();

    gl_Position = viewProjectionMatrix * vec4(spriteVerts[3], 1);
    gs_out.texCoord = vec2(1, 1);
    gs_out.alpha = alpha;
    EmitVertex();
}
