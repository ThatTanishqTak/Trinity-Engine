#ifndef TRINITY_COLOR_OUTPUT_GLSL
#define TRINITY_COLOR_OUTPUT_GLSL

vec3 LinearToSrgb(vec3 linearColor)
{
    vec3 l_UsePower = pow(max(linearColor, vec3(0.0)), vec3(1.0 / 2.4));
    vec3 l_UseScale = linearColor * 12.92;
    vec3 l_UseOffset = 1.055 * l_UsePower - 0.055;
    bvec3 l_UseLowSegment = lessThanEqual(linearColor, vec3(0.0031308));
    return mix(l_UseOffset, l_UseScale, l_UseLowSegment);
}

vec4 ApplySceneColorOutput(vec4 linearColor, uint colorOutputTransfer)
{
    if (colorOutputTransfer == 1u)
    {
        return vec4(LinearToSrgb(linearColor.rgb), linearColor.a);
    }

    return linearColor;
}

#endif