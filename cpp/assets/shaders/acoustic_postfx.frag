#version 120
// EchoLabyrinth 5.3 — Stage 3 GLSL Acoustic Post-Processing Fragment Shader
// Implements:
//  1. 9-tap Gaussian Bloom Glow on bright sonar wavefronts & crystal echo traces
//  2. Radial Chromatic Aberration during Scream tier (noise >= 85) & Hit-Stop impacts
//  3. Acoustic Shockwave UV ripple distortion on Radial Pulse / Seismic Stomp
//  4. Subtle Subterranean CRT Monitor Scanlines & Edge Vignette

uniform sampler2D u_texture;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_bloomIntensity;
uniform float u_chromaticAberration;
uniform float u_crtScanline;
uniform vec2 u_shockwaveCenterUv;
uniform float u_shockwaveRadius;
uniform float u_shockwaveStrength;

void main() {
    vec2 uv = gl_TexCoord[0].xy;

    // 1. Acoustic Shockwave UV refraction ripple
    if (u_shockwaveStrength > 0.0005 && u_shockwaveRadius > 0.01) {
        vec2 aspect = vec2(u_resolution.x / max(1.0, u_resolution.y), 1.0);
        vec2 delta = (uv - u_shockwaveCenterUv) * aspect;
        float dist = length(delta);
        float band = abs(dist - u_shockwaveRadius);
        if (band < 0.065) {
            float wave = (1.0 - band / 0.065) * sin((dist - u_shockwaveRadius) * 95.0);
            vec2 dir = (dist > 0.0001) ? normalize(delta) : vec2(0.0, 0.0);
            uv += dir * wave * u_shockwaveStrength;
        }
    }

    uv = clamp(uv, vec2(0.001), vec2(0.999));

    // 2. Radial Chromatic Aberration (separates R/G/B channels near screen edges on Scream/Hit-Stop)
    vec2 centerOffset = uv - vec2(0.5, 0.5);
    float radialFactor = dot(centerOffset, centerOffset) * 2.2;
    vec2 caShift = centerOffset * u_chromaticAberration * (0.45 + radialFactor);

    float rChan = texture2D(u_texture, clamp(uv + caShift, vec2(0.001), vec2(0.999))).r;
    vec4 baseSample = texture2D(u_texture, uv);
    float gChan = baseSample.g;
    float bChan = texture2D(u_texture, clamp(uv - caShift, vec2(0.001), vec2(0.999))).b;
    vec3 color = vec3(rChan, gChan, bChan);

    // 3. 9-Tap Bright-Pass Acoustic Bloom
    if (u_bloomIntensity > 0.005) {
        vec2 texel = 2.25 / max(vec2(1.0, 1.0), u_resolution);
        vec3 bloomAccum = vec3(0.0);
        for (int ox = -1; ox <= 1; ++ox) {
            for (int oy = -1; oy <= 1; ++oy) {
                vec2 sampleUv = clamp(uv + vec2(float(ox), float(oy)) * texel, vec2(0.001), vec2(0.999));
                vec3 s = texture2D(u_texture, sampleUv).rgb;
                float luma = dot(s, vec3(0.299, 0.587, 0.114));
                float brightWeight = smoothstep(0.38, 0.88, luma);
                bloomAccum += s * brightWeight;
            }
        }
        bloomAccum /= 9.0;
        color += bloomAccum * u_bloomIntensity;
    }

    // 4. Subtle Subterranean CRT Scanline & Edge Vignette
    if (u_crtScanline > 0.001) {
        float scan = sin(uv.y * u_resolution.y * 3.14159265 + u_time * 1.5) * 0.5 + 0.5;
        color *= (1.0 - u_crtScanline * 0.65 * scan);
    }

    float vig = 1.0 - dot(centerOffset, centerOffset) * 0.48;
    color *= clamp(vig, 0.55, 1.0);

    gl_FragColor = vec4(clamp(color, vec3(0.0), vec3(1.0)), baseSample.a) * gl_Color;
}
