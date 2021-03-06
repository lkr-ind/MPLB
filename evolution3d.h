/**
 * Copyright 2019 United Kingdom Research and Innovation
 *
 * Authors: See AUTHORS
 *
 * Contact: [jianping.meng@stfc.ac.uk and/or jpmeng@gmail.com]
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice
 *    this list of conditions and the following disclaimer in the documentation
 *    and or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * ANDANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

/*! @brief   Head file for wrap functions.
 * @author  Jianping Meng
 * @details Declare wrap functions for implementing the main evolution
 * cycle
 */

#ifndef EVOLUTION3D_H_
#define EVOLUTION3D_H_
#include "boundary.h"
#include "flowfield.h"
#include "scheme.h"
#include "type.h"

/*!
 * In this module, we provide subroutines that implement a update system over
 * the whole block and block by block if multi-block
 * The subroutines belongs to three categories:
 * 1. updating boundary with the chosen boundary conditions
 * 2. updating bulk with the chose spatial scheme while the time-marching will
 *    be implemented in this module.
 * 3. updating the macroscopic variables, equilibrium function, body force term
 *    and other parameters as required.
 * Requirement:
 * 1. The ops_par_loop will be called here
 * 2. The subroutines should not directly alter any variables defined in other
 *    modules.  Instead, the kernel functions should be used. This mechanism
 *    allows the subroutines to directly read variables such as g_f
 */

#ifdef OPS_3D
// Stream-collision scheme related
/*!
 * Overall wrap for stream-collision scheme
 */
void StreamCollision3D();
/*!
 * Ops_par_loop for the stream step
 */
void Stream3D();
/*!
 * Ops_par_loop for the collision step
 */
void Collision3D();

// Common routines
/*!
 * Calculating the residual errors for each macroscopic variables
 */
void CalcResidualError3D();

void InitialiseSolution3D();
/*!
 * Mainly for a steady simulation
 */
void DispResidualError3D(const int iter, const Real timePeriod);

void UpdateMacroVars3D();
void UpdateTau3D();
void UpdateFeqandBodyforce3D();
void CopyDistribution3D(const ops_dat* fSrc, ops_dat* fDest);

void TreatBlockBoundary3D(const int blockIndex, const int componentID,
                          const Real* givenVars, int* range,
                          const VertexTypes boundaryType);

#endif /* OPS_3D */
#endif /* EVOLUTION3D_H_ */
