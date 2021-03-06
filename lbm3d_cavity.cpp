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

/** @brief An example main source code of stimulating 3D lid-driven cavity flow
 *  @author Jianping Meng
 **/
#include <cmath>
#include <iostream>
#include <ostream>
#include <string>
#include "boundary.h"
#include "evolution.h"
#include "evolution3d.h"
#include "flowfield.h"
#include "hilemms.h"
#include "model.h"
#include "ops_seq.h"
#include "scheme.h"
#include "type.h"

void simulate() {

    std::string caseName{"3D_lid_Driven_cavity"};
    int spaceDim{3};
    DefineCase(caseName, spaceDim);

    std::vector<std::string> compoNames{"Fluid"};
    std::vector<int> compoid{0};
    std::vector<std::string> lattNames{"d3q19"};
    DefineComponents(compoNames, compoid, lattNames);

    std::vector<VariableTypes> marcoVarTypes{Variable_Rho, Variable_U,
                                             Variable_V, Variable_W};
    std::vector<std::string> macroVarNames{"rho", "u", "v", "w"};
    std::vector<int> macroVarId{0, 1, 2, 3};
    std::vector<int> macroCompoId{0, 0, 0, 0};
    DefineMacroVars(marcoVarTypes, macroVarNames, macroVarId, macroCompoId);

    std::vector<EquilibriumType> equTypes{Equilibrium_BGKIsothermal2nd};
    std::vector<int> equCompoId{0};
    DefineEquilibrium(equTypes, equCompoId);

    std::vector<BodyForceType> bodyForceTypes{BodyForce_None};
    std::vector<int> bodyForceCompoId{0};
    DefineBodyForce(bodyForceTypes, bodyForceCompoId);

    SchemeType scheme{Scheme_StreamCollision};
    DefineScheme(scheme);

    // Setting boundary conditions
    int blockIndex{0};
    int componentId{0};
    std::vector<VariableTypes> macroVarTypesatBoundary{Variable_U, Variable_V,
                                                       Variable_W};
    std::vector<Real> noSlipStationaryWall{0, 0, 0};
    // Left noSlipStationaryWall
    DefineBlockBoundary(blockIndex, componentId, BoundarySurface_Left,
                        BoundaryType_EQMDiffuseRefl, macroVarTypesatBoundary,
                        noSlipStationaryWall);
    // Right noSlipStationaryWall
    DefineBlockBoundary(blockIndex, componentId, BoundarySurface_Right,
                        BoundaryType_EQMDiffuseRefl, macroVarTypesatBoundary,
                        noSlipStationaryWall);
    // Top noslipMovingWall
    std::vector<Real> noSlipMovingWall{0.001, 0, 0};
    DefineBlockBoundary(blockIndex, componentId, BoundarySurface_Top,
                        BoundaryType_EQMDiffuseRefl, macroVarTypesatBoundary,
                        noSlipMovingWall);
    // bottom noSlipStationaryWall
    DefineBlockBoundary(blockIndex, componentId, BoundarySurface_Bottom,
                        BoundaryType_EQMDiffuseRefl, macroVarTypesatBoundary,
                        noSlipStationaryWall);
    // front noSlipStationaryWall
    DefineBlockBoundary(blockIndex, componentId, BoundarySurface_Front,
                        BoundaryType_EQMDiffuseRefl, macroVarTypesatBoundary,
                        noSlipStationaryWall);
    // back noSlipStationaryWall
    DefineBlockBoundary(blockIndex, componentId, BoundarySurface_Back,
                        BoundaryType_EQMDiffuseRefl, macroVarTypesatBoundary,
                        noSlipStationaryWall);

    int blockNum{1};
    std::vector<int> blockSize{33, 33, 33};
    Real meshSize{1. / 32};
    std::vector<Real> startPos{0.0, 0.0, 0.0};
    DefineProblemDomain(blockNum, blockSize, meshSize, startPos);

    DefineInitialCondition();

    std::vector<Real> tauRef{0.01};
    SetTauRef(tauRef);
    SetTimeStep(meshSize / SoundSpeed());

    const Real convergenceCriteria{1E-7};
    const int checkPeriod{1000};
    Iterate(convergenceCriteria, checkPeriod);
}

int main(int argc, char** argv) {
    // OPS initialisation
    ops_init(argc, argv, 1);
    double ct0, ct1, et0, et1;
    ops_timers(&ct0, &et0);
    simulate();
    ops_timers(&ct1, &et1);
    ops_printf("\nTotal Wall time %lf\n", et1 - et0);
    // Print OPS performance details to output stream
    ops_timing_output(stdout);
    ops_exit();
}