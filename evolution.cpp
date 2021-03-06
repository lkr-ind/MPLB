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

/*! @brief   Wrap functions for main evolution cycle.
 * @author  Jianping Meng
 * @details Define wrap functions for implementing the main evolution
 * cycle
 */

#include "evolution.h"
#include "model.h"
#include "hilemms.h"
/*
 * In the following routines, there are some variables are defined
 * for the convenience of the translator which may not be able to
 * understand a function parameter in the ops_par_loop call
 * Even though, a variable rather than a numerical literacy will need
 * some modifications in the Python translator.
 */

#ifdef OPS_2D
void UpdateTau() {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerCalcTau, "KerCalcTau", g_Block[blockIndex], SPACEDIM,
                     iterRng,
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_gbl(TauRef(), NUMCOMPONENTS, "double", OPS_READ),
                     ops_arg_dat(g_MacroVars[blockIndex], NUMMACROVAR,
                                 LOCALSTENCIL, "double", OPS_READ),
                     ops_arg_dat(g_Tau[blockIndex], NUMCOMPONENTS, LOCALSTENCIL,
                                 "double", OPS_RW));
    }
}

void Collision() {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerCollide, "KerCollide", g_Block[blockIndex], SPACEDIM,
                     iterRng, ops_arg_gbl(pTimeStep(), 1, "double", OPS_READ),
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL, "double",
                                 OPS_READ),
                     ops_arg_dat(g_feq[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_Tau[blockIndex], NUMCOMPONENTS, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_Bodyforce[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_fStage[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_WRITE));
    }
}

void Stream() {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerStream, "KerStream", g_Block[blockIndex], SPACEDIM,
                     iterRng,
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_GeometryProperty[blockIndex], 1,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_fStage[blockIndex], NUMXI,
                                 ONEPTLATTICESTENCIL, "double", OPS_READ),
                     ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL, "double",
                                 OPS_RW));
    }
}

void UpdateMacroVars() {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerCalcMacroVars, "KerCalcMacroVars", g_Block[blockIndex],
                     SPACEDIM, iterRng,
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL, "double",
                                 OPS_READ),
                     ops_arg_dat(g_MacroVars[blockIndex], NUMMACROVAR,
                                 LOCALSTENCIL, "double", OPS_RW));
    }
}

void UpdateFeqandBodyforce() {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerCalcFeq, "KerCalcPolyFeq", g_Block[blockIndex],
                     SPACEDIM, iterRng,
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_MacroVars[blockIndex], NUMMACROVAR,
                                 LOCALSTENCIL, "double", OPS_READ),
                     ops_arg_dat(g_feq[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_RW));
        // force term to be added
    }
}

void TreatDomainBoundary(const int blockIndex, const int componentID,
                         const Real* givenVars, int* range,
                         const VertexTypes boundaryType)
{
    switch (boundaryType) {
        case Vertex_ExtrapolPressure1ST: {
            ops_par_loop(
                KerCutCellExtrapolPressure1ST, "KerCutCellExtrapolPressure1ST",
                g_Block[blockIndex], SPACEDIM, range,
                ops_arg_gbl(givenVars, NUMMACROVAR, "double", OPS_READ),
                ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                            ONEPTREGULARSTENCIL, "int", OPS_READ),
                ops_arg_dat(g_GeometryProperty[blockIndex], 1, LOCALSTENCIL,
                            "int", OPS_READ),
                ops_arg_dat(g_f[blockIndex], NUMXI, ONEPTREGULARSTENCIL, "double",
                            OPS_RW));
        } break;
        case Vertex_ExtrapolPressure2ND: {
            ops_par_loop(
                KerCutCellExtrapolPressure2ND,
                "KerCutCellExtrapolPressure2ND", g_Block[blockIndex],
                SPACEDIM, range,
                ops_arg_gbl(givenVars, NUMMACROVAR, "double", OPS_READ),
                ops_arg_dat(g_NodeType[blockIndex], 1, ONEPTREGULARSTENCIL,
                            "int", OPS_READ),
                ops_arg_dat(g_GeometryProperty[blockIndex], 1, LOCALSTENCIL,
                            "int", OPS_READ),
                ops_arg_dat(g_f[blockIndex], NUMXI, TWOPTREGULARSTENCIL,
                            "double", OPS_RW));
        } break;
        case Vertex_ZouHeVelocity: {
            ops_par_loop(
                KerCutCellZouHeVelocity, "KerCutCellZouHeVelocity,",
                g_Block[blockIndex], SPACEDIM, range,
                ops_arg_gbl(givenVars, NUMMACROVAR, "double", OPS_READ),
                ops_arg_dat(g_NodeType[blockIndex], 1, LOCALSTENCIL, "int",
                            OPS_READ),
                ops_arg_dat(g_GeometryProperty[blockIndex], 1, LOCALSTENCIL,
                            "int", OPS_READ),
                ops_arg_dat(g_MacroVars[blockIndex], NUMMACROVAR,
                            ONEPTLATTICESTENCIL, "double", OPS_READ),
                ops_arg_dat(g_f[blockIndex], NUMXI, ONEPTLATTICESTENCIL,
                            "double", OPS_RW));
        } break;
        case Vertex_EQMDiffuseRefl: {
            ops_par_loop(
                KerCutCellEQMDiffuseRefl, "KerCutCellEQMDiffuseRefl",
                g_Block[blockIndex], SPACEDIM, range,
                ops_arg_gbl(givenVars, NUMMACROVAR, "double", OPS_READ),
                ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS, LOCALSTENCIL,
                            "int", OPS_READ),
                ops_arg_dat(g_GeometryProperty[blockIndex], 1, LOCALSTENCIL,
                            "int", OPS_READ),
                ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL, "double",
                            OPS_RW),
                ops_arg_gbl(&componentID, 1, "int", OPS_READ));
        } break;
        case Vertex_FreeFlux: {
            ops_par_loop(KerCutCellZeroFlux, "KerCutCellZeroFlux",
                         g_Block[blockIndex], SPACEDIM, range,
                         ops_arg_dat(g_NodeType[blockIndex], 1, LOCALSTENCIL,
                                     "int", OPS_READ),
                         ops_arg_dat(g_GeometryProperty[blockIndex], 1,
                                     LOCALSTENCIL, "int", OPS_READ),
                         ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL,
                                     "double", OPS_RW));
        } break;
        case Vertex_Periodic: {
            ops_par_loop(KerCutCellPeriodic, "KerCutCellPeriodic",
                         g_Block[blockIndex], SPACEDIM, range,
                         ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                     LOCALSTENCIL, "int", OPS_READ),
                         ops_arg_dat(g_GeometryProperty[blockIndex], 1,
                                     LOCALSTENCIL, "int", OPS_READ),
                         ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL,
                                     "double", OPS_RW));
        } break;
        default:
            break;
    }
}

void TreatEmbeddedBoundary() {
    for (int blockIdx = 0; blockIdx < BlockNum(); blockIdx++) {
        int* iterRng = BlockIterRng(blockIdx, IterRngBulk());
        ops_par_loop(
            KerCutCellEmbeddedBoundary, "KerCutCellImmersedBoundary",
            g_Block[blockIdx], SPACEDIM, iterRng,
            ops_arg_dat(g_NodeType[blockIdx], NUMCOMPONENTS, LOCALSTENCIL,
                        "int", OPS_READ),
            ops_arg_dat(g_GeometryProperty[blockIdx], 1, LOCALSTENCIL, "int",
                        OPS_READ),
            ops_arg_dat(g_f[blockIdx], NUMXI, LOCALSTENCIL, "double", OPS_RW));
    }
}
//TODO This function needs to be improved for different initialisation scheme
void InitialiseSolution() {
    UpdateFeqandBodyforce();
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        const Real zero = 0;
        ops_par_loop(
            KerSetfFixValue, "KerSetfFixValue", g_Block[blockIndex], SPACEDIM,
            iterRng, ops_arg_gbl(&zero, 1, "double", OPS_READ),
            ops_arg_dat(g_Bodyforce[0], NUMXI, LOCALSTENCIL, "double", OPS_RW));
    }
    CopyDistribution(g_feq, g_f);
}

void CopyDistribution(const ops_dat* fSrc, ops_dat* fDest) {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerCopyf, "KerCopyf", g_Block[blockIndex], SPACEDIM,
                     iterRng,
                     ops_arg_dat(fSrc[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(fDest[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_WRITE));
    }
}

void CalcResidualError() {
    for (int macroVarIdx = 0; macroVarIdx < MacroVarsNum(); macroVarIdx++) {
        for (int blockIdx = 0; blockIdx < BlockNum(); blockIdx++) {
            int* iterRng = BlockIterRng(blockIdx, IterRngWhole());
            ops_par_loop(KerCalcMacroVarSquareofDifference,
                         "KerCalcMacroVarSquareofDifference", g_Block[blockIdx],
                         SPACEDIM, iterRng,
                         ops_arg_dat(g_MacroVars[blockIdx], NUMMACROVAR,
                                     LOCALSTENCIL, "double", OPS_READ),
                         ops_arg_dat(g_MacroVarsCopy[blockIdx], NUMMACROVAR,
                                     LOCALSTENCIL, "double", OPS_READ),
                         ops_arg_gbl(&macroVarIdx, 1, "int", OPS_READ),
                         ops_arg_reduce(g_ResidualErrorHandle[macroVarIdx], 1,
                                        "double", OPS_INC));
        }
    }
    for (int macroVarIdx = 0; macroVarIdx < MacroVarsNum(); macroVarIdx++) {
        ops_reduction_result(g_ResidualErrorHandle[macroVarIdx],
                             (double*)&g_ResidualError[2 * macroVarIdx]);
    }
    for (int blockIdx = 0; blockIdx < BlockNum(); blockIdx++) {
        int* iterRng = BlockIterRng(blockIdx, IterRngWhole());
        ops_par_loop(KerCopyMacroVars, "KerCopyMacroVars", g_Block[blockIdx],
                     SPACEDIM, iterRng,
                     ops_arg_dat(g_MacroVars[blockIdx], NUMMACROVAR,
                                 LOCALSTENCIL, "double", OPS_READ),
                     ops_arg_dat(g_MacroVarsCopy[blockIdx], NUMMACROVAR,
                                 LOCALSTENCIL, "double", OPS_RW));
    }
    for (int macroVarIdx = 0; macroVarIdx < MacroVarsNum(); macroVarIdx++) {
        for (int blockIdx = 0; blockIdx < BlockNum(); blockIdx++) {
            int* iterRng = BlockIterRng(blockIdx, IterRngWhole());
            ops_par_loop(KerCalcMacroVarSquare, "KerCalcMacroVarSquare",
                         g_Block[blockIdx], SPACEDIM, iterRng,
                         ops_arg_dat(g_MacroVars[blockIdx], NUMMACROVAR,
                                     LOCALSTENCIL, "double", OPS_READ),
                         ops_arg_gbl(&macroVarIdx, 1, "int", OPS_READ),
                         ops_arg_reduce(g_ResidualErrorHandle[macroVarIdx], 1,
                                        "double", OPS_INC));
        }
    }
    for (int macroVarIdx = 0; macroVarIdx < MacroVarsNum(); macroVarIdx++) {
        ops_reduction_result(g_ResidualErrorHandle[macroVarIdx],
                             (double*)&g_ResidualError[2 * macroVarIdx + 1]);
    }
}

void ForwardEuler() {
    for (int blockIndex = 0; blockIndex < BlockNum(); blockIndex++) {
        int* iterRng = BlockIterRng(blockIndex, IterRngWhole());
        ops_par_loop(KerCutCellCVTUpwind2nd, "KerCutCellCVTUpwind2nd",
                     g_Block[blockIndex], SPACEDIM, iterRng,
                     ops_arg_dat(g_CoordinateXYZ[blockIndex], SPACEDIM,
                                 ONEPTREGULARSTENCIL, "double", OPS_READ),
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_GeometryProperty[blockIndex], 1,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_f[blockIndex], NUMXI, ONEPTREGULARSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_fStage[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_RW));
        Real schemeCoeff{1};
        ops_par_loop(KerCutCellExplicitTimeMach, "KerCutCellExplicitTimeMach",
                     g_Block[blockIndex], SPACEDIM, iterRng,
                     ops_arg_gbl(pTimeStep(), 1, "double", OPS_READ),
                     ops_arg_gbl(&schemeCoeff, 1, "double", OPS_READ),
                     ops_arg_dat(g_NodeType[blockIndex], NUMCOMPONENTS,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_GeometryProperty[blockIndex], 1,
                                 LOCALSTENCIL, "int", OPS_READ),
                     ops_arg_dat(g_fStage[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_feq[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_Tau[blockIndex], NUMCOMPONENTS, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_Bodyforce[blockIndex], NUMXI, LOCALSTENCIL,
                                 "double", OPS_READ),
                     ops_arg_dat(g_f[blockIndex], NUMXI, LOCALSTENCIL, "double",
                                 OPS_RW));
    }
}

void DispResidualError(const int iter, const Real checkPeriod) {
    ops_printf("##########Residual Error at %i time step##########\n", iter);
    for (int macroVarIdx = 0; macroVarIdx < MacroVarsNum(); macroVarIdx++) {
        Real residualError = g_ResidualError[2 * macroVarIdx] /
                             g_ResidualError[2 * macroVarIdx + 1] /
                             (checkPeriod * TimeStep());
        ops_printf("%s = %.17g\n", MacroVarName()[macroVarIdx].c_str(),
                   residualError);
    }
}


//TODO Shall we introduce debug information mechanism similar to 3D version?
void StreamCollision() {
    UpdateMacroVars();
    CopyDistribution(g_f, g_fStage);
    UpdateFeqandBodyforce();
    UpdateTau();
    Collision();
    Stream();
    ImplementBoundaryConditions();
}

void TimeMarching() {
    UpdateMacroVars();
    UpdateFeqandBodyforce();
    UpdateTau();
    ForwardEuler();
    //ops_halo_transfer(HaloGroups);
    ImplementBoundary();
}
#endif /* OPS_2D */