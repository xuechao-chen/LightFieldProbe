#ifndef LightFieldProbe_glsl
#define LightFieldProbe_glsl

// Compatibility and helper code from G3D
#include <g3dmath.glsl>
#include <Texture/Texture.glsl>

// From G3D, but included directly in this supplement
// because it is core to the algorithm
#include "Octahedral.glsl"

// On [0, L.probeCounts.x * L.probeCounts.y * L.probeCounts.z - 1]
#define ProbeIndex int

// probe xyz indices
#define GridCoord ivec3

struct LightFieldSurface {
	sampler2DArray          radianceProbeGrid;
	sampler2DArray          normalProbeGrid;
	sampler2DArray          distanceProbeGrid;
	sampler2DArray          lowResolutionDistanceProbeGrid;
    Vector3int32            probeCounts;
    Point3                  probeStartPosition;
    Vector3                 probeStep;
    int                     lowResolutionDownsampleFactor;
	sampler2DArray			irradianceProbeGrid;
	sampler2DArray			meanDistProbeGrid;
};

/** 
 \param probeCoords Integer (stored in float) coordinates of the probe on the probe grid 
 */
ProbeIndex gridCoordToProbeIndex(in LightFieldSurface L, in Point3 probeCoords) {
    return int(probeCoords.x + probeCoords.y * L.probeCounts.x + probeCoords.z * L.probeCounts.x * L.probeCounts.y);
}

GridCoord baseGridCoord(in LightFieldSurface L, Point3 X) {
    return clamp(GridCoord((X - L.probeStartPosition) / L.probeStep),
                GridCoord(0, 0, 0), 
                GridCoord(L.probeCounts) - GridCoord(1, 1, 1));
}

GridCoord probeIndexToGridCoord(in LightFieldSurface L, ProbeIndex index) {
    // Assumes probeCounts are powers of two.
    // Precomputing the MSB actually slows this code down substantially
    ivec3 iPos;
    iPos.x = index & (L.probeCounts.x - 1);
    iPos.y = (index & ((L.probeCounts.x * L.probeCounts.y) - 1)) >> findMSB(L.probeCounts.x);
    iPos.z = index >> findMSB(L.probeCounts.x * L.probeCounts.y);

    return iPos;
}

Point3 gridCoordToPosition(in LightFieldSurface L, GridCoord c) {
    return L.probeStep * Vector3(c) + L.probeStartPosition;
}

// Engine-specific arguments and helper functions have been removed from the following code 

Irradiance3 computePrefilteredIrradiance(LightFieldSurface lightFieldSurface, Point3 wsPosition, Point3 wsNormal) {
    GridCoord baseGridCoord = baseGridCoord(lightFieldSurface, wsPosition);
    Point3 baseProbePos = gridCoordToPosition(lightFieldSurface, baseGridCoord);
    Irradiance3 sumIrradiance = Irradiance3(0);
    float sumWeight = 0.0;
    // Trilinear interpolation values along axes
    Vector3 alpha = clamp((wsPosition - baseProbePos) / lightFieldSurface.probeStep, Vector3(0), Vector3(1));

    // Iterate over the adjacent probes defining the surrounding vertex "cage"
    for (int i = 0; i < 8; ++i) {
        // Compute the offset grid coord and clamp to the probe grid boundary
        GridCoord  offset = ivec3(i, i >> 1, i >> 2) & ivec3(1);
        GridCoord  probeGridCoord = clamp(baseGridCoord + offset, GridCoord(0), GridCoord(lightFieldSurface.probeCounts - 1));
        ProbeIndex p = gridCoordToProbeIndex(lightFieldSurface, probeGridCoord);

        // Compute the trilinear weights based on the grid cell vertex to smoothly
        // transition between probes. Avoid ever going entirely to zero because that
        // will cause problems at the border probes.
        Vector3 trilinear = lerp(1 - alpha, alpha, offset);
        float weight = trilinear.x * trilinear.y * trilinear.z;

        // Make cosine falloff in tangent plane with respect to the angle from the surface to the probe so that we never
        // test a probe that is *behind* the surface.
        // It doesn't have to be cosine, but that is efficient to compute and we must clip to the tangent plane.
        Point3 probePos = gridCoordToPosition(lightFieldSurface, probeGridCoord);
        Vector3 probeToPoint = wsPosition - probePos;
        Vector3 dir = normalize(-probeToPoint);

        // Smooth back-face test
        weight *= max(0.05, dot(dir, wsNormal));//TODO:

        //float2 temp = texture(lightFieldSurface.meanDistProbeGrid.sampler, vec4(-dir, p)).rg;

		vec2 hitProbeTexCoord = (octEncode(-dir)*0.5+0.5);
        float2 temp = texture(lightFieldSurface.meanDistProbeGrid, float3(hitProbeTexCoord, p)).rg;

		float mean = temp.x;
        float variance = abs(temp.y - square(mean));

        float distToProbe = length(probeToPoint);
        // http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5
        float t_sub_mean = distToProbe - mean;
        float chebychev = variance / (variance + square(t_sub_mean));

        weight *= ((distToProbe <= mean) ? 1.0 : max(chebychev, 0.0));

        // Avoid zero weight
        weight = max(0.0002, weight);

        sumWeight += weight;

        Vector3 irradianceDir = wsNormal;
		hitProbeTexCoord = (octEncode(irradianceDir)*0.5 + 0.5);
		Irradiance3 probeIrradiance = texture(lightFieldSurface.irradianceProbeGrid, float3(hitProbeTexCoord, p)).rgb;
		//return probeIrradiance;
        // Debug probe contribution by visualizing as colors
        // probeIrradiance = 0.5 * probeIndexToColor(lightFieldSurface, p);

        sumIrradiance += weight * probeIrradiance;
    }

    return 2.0 * pi * sumIrradiance / sumWeight;
}

#endif // Header guard
