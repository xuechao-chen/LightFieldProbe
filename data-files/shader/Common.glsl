#ifndef Common_glsl
#define Common_glsl

// Compatibility and helper code from G3D
#include <g3dmath.glsl>
#include <Texture/Texture.glsl>
#include "Octahedral.glsl"


const float minThickness = 0.03; // meters
const float maxThickness = 0.50; // meters

// Points exactly on the boundary in octahedral space (x = 0 and y = 0 planes) map to two different
// locations in octahedral space. We shorten the segments slightly to give unambigous locations that lead
// to intervals that lie within an octant.
const float rayBumpEpsilon = 0.001; // meters

// If we go all the way around a cell and don't move farther than this (in m)
// then we quit the trace
const float minProgressDistance = 0.01;

//  zyx bit pattern indicating which probe we're currently using within the cell on [0, 7]
#define CycleIndex int

// On [0, L.probeCounts.x * L.probeCounts.y * L.probeCounts.z - 1]
#define ProbeIndex int

// probe xyz indices
#define GridCoord ivec3

// Enumerated value
#define TraceResult int
#define TRACE_RESULT_MISS    0
#define TRACE_RESULT_HIT     1
#define TRACE_RESULT_UNKNOWN 2

struct LightFieldSurface {
	sampler2DArray          radianceProbeGrid;
	sampler2DArray          normalProbeGrid;
	sampler2DArray          distanceProbeGrid;
	sampler2DArray          lowResolutionDistanceProbeGrid;
	Vector3int32            probeCounts;
	Point3                  probeStartPosition;
	Vector3                 probeStep;
	int                     lowResolutionDownsampleFactor;
	sampler2DArray          irradianceProbeGrid;
	sampler2DArray          meanDistProbeGrid;
};


float distanceSquared(Point2 v0, Point2 v1) {
	Point2 d = v1 - v0;
	return dot(d, d);
}

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

/** Returns the index of the probe at the floor along each dimension. */
ProbeIndex baseProbeIndex(in LightFieldSurface L, Point3 X) {
	return gridCoordToProbeIndex(L, baseGridCoord(L, X));
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

/** probeCoords Coordinates of the probe, computed as part of the process. */
ProbeIndex nearestProbeIndex(in LightFieldSurface L, Point3 X, out Point3 probeCoords) {
	probeCoords = clamp(round((X - L.probeStartPosition) / L.probeStep),
		Point3(0, 0, 0),
		Point3(L.probeCounts) - Point3(1, 1, 1));

	return gridCoordToProbeIndex(L, probeCoords);
}

/**
	\param neighbors The 8 probes surrounding X
	\return Index into the neighbors array of the index of the nearest probe to X
*/
CycleIndex nearestProbeIndices(in LightFieldSurface L, Point3 X) {
	Point3 maxProbeCoords = Point3(L.probeCounts) - Point3(1, 1, 1);
	Point3 floatProbeCoords = (X - L.probeStartPosition) / L.probeStep;
	Point3 baseProbeCoords = clamp(floor(floatProbeCoords), Point3(0, 0, 0), maxProbeCoords);

	float minDist = 10.0f;
	int nearestIndex = -1;

	for (int i = 0; i < 8; ++i) {
		Point3 newProbeCoords = min(baseProbeCoords + vec3(i & 1, (i >> 1) & 1, (i >> 2) & 1), maxProbeCoords);
		float d = length(newProbeCoords - floatProbeCoords);
		if (d < minDist) {
			minDist = d;
			nearestIndex = i;
		}
	}

	return nearestIndex;
}


Point3 gridCoordToPosition(in LightFieldSurface L, GridCoord c) {
	return L.probeStep * Vector3(c) + L.probeStartPosition;
}


Point3 probeLocation(in LightFieldSurface L, ProbeIndex index) {
	return gridCoordToPosition(L, probeIndexToGridCoord(L, index));
}


/** GLSL's dot on ivec3 returns a float. This is an all-integer version */
int idot(ivec3 a, ivec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}


/**
	\param baseProbeIndex Index into L.radianceProbeGrid's TEXTURE_2D_ARRAY. This is the probe
	at the floor of the current ray sampling position.

   \param relativeIndex on [0, 7]. This is used as a set of three 1-bit offsets

   Returns a probe index into L.radianceProbeGrid. It may be the *same* index as
   baseProbeIndex.

   This will wrap when the camera is outside of the probe field probes...but that's OK.
   If that case arises, then the trace is likely to
   be poor quality anyway. Regardless, this function will still return the index
   of some valid probe, and that probe can either be used or fail because it does not
   have visibility to the location desired.

   \see nextCycleIndex, baseProbeIndex
 */
ProbeIndex relativeProbeIndex(in LightFieldSurface L, ProbeIndex baseProbeIndex, CycleIndex relativeIndex) {
	// Guaranteed to be a power of 2
	ProbeIndex numProbes = L.probeCounts.x * L.probeCounts.y * L.probeCounts.z;

	ivec3 offset = ivec3(relativeIndex & 1, (relativeIndex >> 1) & 1, (relativeIndex >> 2) & 1);
	ivec3 stride = ivec3(1, L.probeCounts.x, L.probeCounts.x * L.probeCounts.y);

	return (baseProbeIndex + idot(offset, stride)) & (numProbes - 1);
}


/** Given a CycleIndex [0, 7] on a cube of probes, returns the next CycleIndex to use.
	\see relativeProbeIndex
*/
CycleIndex nextCycleIndex(CycleIndex cycleIndex) {
	return (cycleIndex + 3) & 7;
}


float squaredLength(Vector3 v) {
	return dot(v, v);
}


/** Two-element sort: maybe swaps a and b so that a' = min(a, b), b' = max(a, b). */
void minSwap(inout float a, inout float b) {
	float temp = min(a, b);
	b = max(a, b);
	a = temp;
}


/** Sort the three values in v from least to
	greatest using an exchange network (i.e., no branches) */
void sort(inout float3 v) {
	minSwap(v[0], v[1]);
	minSwap(v[1], v[2]);
	minSwap(v[0], v[1]);
}

vec2 size(in sampler2DArray tex)
{
	return vec2(textureSize(tex, 0));
}

vec2 invSize(in sampler2DArray tex)
{
	return vec2(1.0) / size(tex);
}

vec3 packNormal(vec3 N)
{
	return N * vec3(0.5) + vec3(0.5);
}

vec3 unpackNormal(vec3 packedNormal)
{
	return normalize(packedNormal * vec3(2.0) - vec3(1.0));
}

#endif // Header guard