#pragma once

// Robust geometric predicates using adaptive precision arithmetic.
// Based on Jonathan Shewchuk's "Adaptive Precision Floating-Point Arithmetic
// and Fast Robust Predicates for Computational Geometry" (1997).
//
// Public domain reference implementation adapted for C++.
// This provides exact orientation and incircle tests that never give
// incorrect results due to floating-point roundoff errors.

namespace sgl {
namespace robust {

// Initialize error bound constants. Must be called once before use.
void init();

// Returns a positive value if (pa, pb, pc) are counter-clockwise,
// a negative value if clockwise, and zero if collinear.
// The result is exact — no false collinearities or wrong orientations.
double orient2d(const double *pa, const double *pb, const double *pc);

// Returns a positive value if pd lies inside the circle passing through
// pa, pb, pc (which must be counter-clockwise), negative if outside,
// and zero if on the circle. Exact result.
double incircle(const double *pa, const double *pb, const double *pc, const double *pd);

} // namespace robust
} // namespace sgl
