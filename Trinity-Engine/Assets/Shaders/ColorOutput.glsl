#ifndef TRINITY_COLOR_OUTPUT_GLSL
#define TRINITY_COLOR_OUTPUT_GLSL

vec3 SrgbEotf(vec3 srgbColor)
{
    vec3 l_ClampedSrgbColor = clamp(srgbColor, vec3(0.0), vec3(1.0));
    vec3 l_UsePower = pow((l_ClampedSrgbColor + 0.055) / 1.055, vec3(2.4));
    vec3 l_UseScale = l_ClampedSrgbColor / 12.92;
    bvec3 l_UseLowSegment = lessThanEqual(l_ClampedSrgbColor, vec3(0.04045));
    return mix(l_UsePower, l_UseScale, l_UseLowSegment);
}

vec3 SrgbOetf(vec3 linearColor)
{
    vec3 l_ClampedLinearColor = max(linearColor, vec3(0.0));
    vec3 l_UsePower = pow(l_ClampedLinearColor, vec3(1.0 / 2.4));
    vec3 l_UseScale = l_ClampedLinearColor * 12.92;
    vec3 l_UseOffset = 1.055 * l_UsePower - 0.055;
    bvec3 l_UseLowSegment = lessThanEqual(l_ClampedLinearColor, vec3(0.0031308));
    return mix(l_UseOffset, l_UseScale, l_UseLowSegment);
}

vec3 ApplySceneColorInput(vec3 inputColor, uint sceneInputTransfer)
{
    if (sceneInputTransfer == 1u)
    {
        return SrgbEotf(inputColor);
    }

    return inputColor;
}

vec4 ApplySceneColorOutput(vec4 linearColor, uint colorOutputTransfer)
{
    if (colorOutputTransfer == 1u)
    {
        return vec4(SrgbOetf(linearColor.rgb), linearColor.a);
    }

    return linearColor;
}

#endif