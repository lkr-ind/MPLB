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

/*! @brief   Implementing functions related to the flow field
 * @author  Jianping Meng
 * @details Implementing functions related to create the flow
 * field (allocate memory), set up the geometry and the boundary
 * property, and deallocate the memory.
 */
#ifndef FLOWFIELD_H
#define FLOWFIELD_H
#include <algorithm>
#include <cmath>
#include <string>
#include "boundary.h"
#include "model.h"
#include "scheme.h"
#include "type.h"
/*!
 * This module is set for defining blocks and variables defined on a block
 * including distribution functions, macroscopic variables, node properties,
 * and relevant parameters.
 * The responsibilities including:
 * 1. Create all variables from files or annually written subroutines
 * 2. Initialise the required macroscopic variables and thereby the
 *    distribution functions.
 * 3. Provide some tools for accessing variables.
 */
extern int SPACEDIM;
extern ops_block* g_Block;
/*!
 * The size of g_f in each node will be determined by the employed quadrature
 * and the model. For example, if we are simulating a two-phase flow, then the
 * size will be the product of NUMXI and NUMCOMPONENTS.
 */
extern ops_dat* g_f;
/*! might be changed to a local temporary variable
 * if we use some control routine in the main.cpp
 */
extern ops_dat* g_fStage;
extern ops_dat* g_feq;
/*!
 * Bodyforce, which is independent of the particle velocity
 */
extern ops_dat* g_Bodyforce;
/*!
 * g_MacroVars: for storing the macroscopic variables, to reduce
 * the complexity of calculating equilibrium, it will has a specific order
 */
extern ops_dat* g_MacroVars;
/*!
 * Save the macroscopic variables at the previous step
 * Typically used for steady flow.
 */
extern ops_dat* g_MacroVarsCopy;
/*!
 * Relaxation time
 * Depend on some macroscopic variables like rho,T
 */
extern ops_dat* g_Tau;
/*!
 * the residual error for steady flows
 * for each macroscopic variable, there are two values: the absolute
 * and relative
 * for each component of a vector, two values are allocated
 */
extern Real* g_ResidualError;
extern ops_reduction* g_ResidualErrorHandle;
// Boundary fitting mesh
// The following variables are introduced for
// implementing finite difference schemes
/*!
 * g_DiscreteConvectionTerm: for finite difference scheme
 */
extern ops_dat* g_DiscreteConvectionTerm;
/*! metrics structure
 * | xi_x  0 xi_y  1 |
 * | eta_x 2 eta_y 3 |
 *
 */
extern ops_dat* g_Metrics;
// Boundary fitting mesh
// Cutting cell
/*!
 * g_NodeType: boundary or fluid
 */
extern ops_dat* g_NodeType;
/*!
 * immersed solid? or the end point of the body.
 */
extern ops_dat* g_GeometryProperty;
/*!
 * Coordinate
 */
extern ops_dat* g_CoordinateXYZ;
// Cutting cell
int* IterRngWhole();
int* IterRngJmin();
int* IterRngJmax();
int* IterRngImin();
int* IterRngImax();
int* IterRngBulk();
int* IterRngKmax();
int* IterRngKmin();
/*!
 *Get the pointer pointing to the starting position of IterRng of this block
 *No NULL check for efficiency
 *Note: it looks that ops_par_loop call does not support const point.
 */
inline int* BlockIterRng(const int blockId, int* iterRng) {
    return &iterRng[blockId * 2 * SPACEDIM];
}
/*!
 * Return the starting position of memory in which we store the size of each
 * block
 */
const int* BlockSize(const int blockId);
const int BlockNum();
const int SpaceDim();
const int HaloDepth();
const Real TimeStep();
const Real* pTimeStep();
const Real* TauRef();
// const int* GetBlockNum();
const std::string CaseName();
const int HaloPtNum();
Real TotalMeshSize();
const ops_halo_group HaloGroup();
void SetTimeStep(Real dt);
void SetCaseName(const std::string caseName);
void setCaseName(const char* caseName);
void SetTauRef(const std::vector<Real> tauRef);
void SetBlockSize(const std::vector<int> blockSize);
void SetBlockNum(const int blockNum);

/*!
 * Manually setup the flow field.
 */
void SetupFlowfield();
void SetupFlowfieldfromHdf5();
void DefineVariables();
void WriteFlowfieldToHdf5(const long timeStep);
void WriteDistributionsToHdf5(const long timeStep);
void WriteNodePropertyToHdf5(const long timeStep);
void DestroyFlowfield();
void DefineHaloTransfer();
void DefineHaloTransfer3D();
void SetHaloDepth(const int haloDepth);
void SetHaloRelationNum(const int haloRelationNum);
// caseName: case name
// spaceDim: 2D or 3D application
void DefineCase(std::string caseName, const int spaceDim);
#endif
