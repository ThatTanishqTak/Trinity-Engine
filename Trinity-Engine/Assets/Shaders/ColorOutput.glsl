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


vec3 ApplyColorTransfer(vec3 inputColor, uint transferMode)
{
    if (transferMode == 1u)
    {
        return SrgbEotf(inputColor);
    }

    if (transferMode == 2u)
    {
        return SrgbOetf(inputColor);
    }

    return inputColor;
}

vec3 ApplySceneColorInput(vec3 inputColor, uint sceneInputTransfer)
{
    return ApplyColorTransfer(inputColor, sceneInputTransfer);
}

vec4 ApplySceneColorOutput(vec4 linearColor, uint colorOutputTransfer)
{
    return vec4(ApplyColorTransfer(linearColor.rgb, colorOutputTransfer), linearColor.a);
}

vec3 ApplyUiColorInput(vec3 inputColor, uint uiInputTransfer)
{
    return ApplyColorTransfer(inputColor, uiInputTransfer);
}

vec4 ApplyUiColorOutput(vec4 linearColor, uint uiOutputTransfer)
{
    return vec4(ApplyColorTransfer(linearColor.rgb, uiOutputTransfer), linearColor.a);
}

#endif