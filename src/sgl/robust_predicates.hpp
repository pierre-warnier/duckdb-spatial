#pragma once

// Robust geometric predicates using adaptive precision arithmetic.
// Based on Jonathan Shewchuk's "Adaptive Precision Floating-Point Arithmetic
// and Fast Robust Predicates for Computational Geometry" (1997).
//
// Public domain reference implementation adapted for C++.
//
// orient2d() is fully adaptive and produces exact results.
// incircle() implements the fast-path error-bound check only; in the
// error-bound region it returns the approximate double-precision determinant
// rather than the exact adaptive result. Fine for most practical inputs, but
// callers that require guaranteed exactness near-degenerate incircle tests
// should compute their own refinement or tolerate the approximation.

namespace sgl {
namespace robust {

// Initialize error bound constants. Thread-safe; idempotent.
// Called automatically on first use of orient2d/incircle.
void init();

// Returns a positive value if (pa, pb, pc) are counter-clockwise,
// a negative value if clockwise, and zero if collinear.
// Adaptive-precision: exact sign determination, no false collinearities.
double orient2d(const double *pa, const double *pb, const double *pc);

// Returns a positive value if pd lies inside the circle passing through
// pa, pb, pc (which must be counter-clockwise), negative if outside,
// and zero if on the circle.
// NOTE: fast-path only — returns the approximate determinant when within
// the error bound. Not fully adaptive (see header comment).
double incircle(const double *pa, const double *pb, const double *pc, const double *pd);

} // namespace robust
} // namespace sgl
