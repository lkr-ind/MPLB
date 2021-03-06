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

/*！ @brief Define kernels for numerical schemes.
 *  @author Jianping Meng
 *  @details Including various space and time scheme.
 *  For the stream-collision, we plan only to support the standard lattice,
 *  That is the speed is only 1, not multi-speed lattice
 *  For the finite-difference scheme, only the cut-cell mesh technique is
 *  supported currently
 **/
#ifndef SCHEME_KERNEL_H
#define SCHEME_KERNEL_H
#include "scheme.h"
#ifdef OPS_2D  // two dimensional code
void KerCollide(const Real* dt, const int* nodeType, const Real* f,
                const Real* feq, const Real* relaxationTime,
                const Real* bodyForce, Real* fStage) {
    VertexTypes vt = (VertexTypes)nodeType[OPS_ACC1(0, 0)];
    // collisionRequired: means if collision is required at boundary
    // e.g., the ZouHe boundary condition explicitly requires collision
    bool collisionRequired =
        (vt == Vertex_Fluid ||
         vt == Vertex_ZouHeVelocity ||
         vt == Vertex_EQMDiffuseRefl ||
         vt == Vertex_ExtrapolPressure1ST ||
         vt == Vertex_ExtrapolPressure2ND
         );
    if (collisionRequired) {
        for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
            Real tau = relaxationTime[OPS_ACC_MD4(compoIndex, 0, 0)];
            Real dtOvertauPlusdt = (*dt) / (tau + 0.5 * (*dt));
            for (int xiIndex = COMPOINDEX[2 * compoIndex];
                 xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
                fStage[OPS_ACC_MD6(xiIndex, 0, 0)] =
                    f[OPS_ACC_MD2(xiIndex, 0, 0)] -
                    dtOvertauPlusdt * (f[OPS_ACC_MD2(xiIndex, 0, 0)] -
                                       feq[OPS_ACC_MD3(xiIndex, 0, 0)]) +
                    tau * dtOvertauPlusdt *
                        bodyForce[OPS_ACC_MD5(xiIndex, 0, 0)];
            }
        }
    }
}
void KerStream(const int* nodeType, const int* geometry, const Real* fStage,
               Real* f) {
    //ops_printf("Inside stream kernel!!!!!. \n");
    VertexTypes vt = (VertexTypes)nodeType[OPS_ACC0(0, 0)];
    VertexGeometryTypes vg = (VertexGeometryTypes)geometry[OPS_ACC1(0, 0)];
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        for (int xiIndex = COMPOINDEX[2 * compoIndex];
             xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
            int cx = (int)XI[xiIndex * LATTDIM];
            int cy = (int)XI[xiIndex * LATTDIM + 1];
            if ((vt >= Vertex_Fluid) && (vt < Vertex_Boundary)) {
                //ops_printf("I am inside if vt = %d. \n", (int)vt);
                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
            }
            if (vt >= Vertex_Boundary) {
                // streamRequired: means if the particles with velocity parallel
                // needs to be streamed at the boundary
                bool streamRequired =
                    (vt == Vertex_ZouHeVelocity ||
                     vt == Vertex_EQMDiffuseRefl ||
                     vt == Vertex_ExtrapolPressure1ST ||
                     vt == Vertex_ExtrapolPressure2ND
                     );
                if (streamRequired) {
                    if ((cx == 0) && (cy == 0)) {
                        f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                            fStage[OPS_ACC_MD2(xiIndex, 0, 0)];
                        continue;
                    }
                }
                switch (vg) {
                    case VG_IP:
                        // (cx=0 means stream is implemented at i=0,so here we
                        //  disable the step at boundary)
                        if (streamRequired) {
                            if (cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_IM:
                        if (streamRequired) {
                            if (cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_JP:
                        if (streamRequired) {
                            if (cy <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cy < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_JM:
                        if (streamRequired) {
                            if (cy >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cy > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_IPJP_I:
                        if (streamRequired) {
                            if (cy <= 0 && cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cy < 0 && cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_IPJM_I:
                        if (streamRequired) {
                            if (cy >= 0 && cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cy > 0 && cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_IMJP_I:
                        if (streamRequired) {
                            if (cy <= 0 && cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cy < 0 && cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_IMJM_I:
                        if (streamRequired) {
                            if (cy >= 0 && cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        } else {
                            if (cy > 0 && cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                            }
                        }
                        break;
                    case VG_IPJP_O:
                        if (cy < 0 || cx < 0) {
                            f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                        }
                        break;
                    case VG_IPJM_O:
                        if (cy > 0 || cx < 0) {
                            f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                        }
                        break;
                    case VG_IMJP_O:
                        if (cy < 0 || cx > 0) {
                            f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                        }
                        break;
                    case VG_IMJM_O:
                        if (cy > 0 || cx > 0) {
                            f[OPS_ACC_MD3(xiIndex, 0, 0)] =
                                fStage[OPS_ACC_MD2(xiIndex, -cx, -cy)];
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void KerCutCellCVTUpwind1st(const Real* coordinateXYZ, const int* nodeType,
                            const int* geometry, const Real* f,
                            Real* fGradient) {
    VertexTypes vt = (VertexTypes)nodeType[OPS_ACC1(0, 0)];
    VertexGeometryTypes vg = (VertexGeometryTypes)geometry[OPS_ACC2(0, 0)];
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        for (int xiIndex = COMPOINDEX[2 * compoIndex];
             xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
            Real cx{XI[xiIndex * LATTDIM]};
            Real cy{XI[xiIndex * LATTDIM + 1]};
            // setting a initial value
            fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] = 0;
            // make sure no calculation occurring at boundary when unnecessary
            // this can avoid access undefined memory if the halo point is set
            // incorrectly
            bool needCalc{true};
            if (vt == Vertex_ImmersedSolid) needCalc = false;
            if (vt >= Vertex_Boundary) {
                switch (vg) {
                    case VG_IP:
                        if (cx > 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IM:
                        if (cx < 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_JP:
                        if (cy > 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_JM:
                        if (cy < 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IPJP_I:
                        if (cy > 0 || cx > 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IPJM_I:
                        if (cy < 0 || cx > 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IMJP_I:
                        if (cy > 0 || cx < 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IMJM_I:
                        if (cy < 0 || cx < 0) {
                            needCalc = false;
                        }
                        break;
                    // It appears that the information is well defined for the
                    // outer corners
                    // which is quite different from the inner corners
                    case VG_IPJP_O:
                        if (cy >= 0 && cx >= 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IPJM_O:
                        if (cy <= 0 && cx >= 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IMJP_O:
                        if (cy >= 0 && cx <= 0) {
                            needCalc = false;
                        }
                        break;
                    case VG_IMJM_O:
                        if (cy <= 0 && cx <= 0) {
                            needCalc = false;
                        }
                        break;
                    default:
                        break;
                }
            }
            if (needCalc) {
                if (cx > 0) {
                    fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                        CS * cx *
                        ((f[OPS_ACC_MD3(xiIndex, 0, 0)] -
                          f[OPS_ACC_MD3(xiIndex, -1, 0)]) /
                         (coordinateXYZ[OPS_ACC_MD0(0, 0, 0)] -
                          coordinateXYZ[OPS_ACC_MD0(0, -1, 0)]));
                }
                if (cx < 0) {
                    fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                        CS * cx *
                        ((f[OPS_ACC_MD3(xiIndex, 1, 0)] -
                          f[OPS_ACC_MD3(xiIndex, 0, 0)]) /
                         (coordinateXYZ[OPS_ACC_MD0(0, 1, 0)] -
                          coordinateXYZ[OPS_ACC_MD0(0, 0, 0)]));
                }
                if (cy > 0) {
                    fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                        CS * cy *
                        ((f[OPS_ACC_MD3(xiIndex, 0, 0)] -
                          f[OPS_ACC_MD3(xiIndex, 0, -1)]) /
                         (coordinateXYZ[OPS_ACC_MD0(1, 0, 0)] -
                          coordinateXYZ[OPS_ACC_MD0(1, 0, -1)]));
                }
                if (cy < 0) {
                    fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                        CS * cy *
                        ((f[OPS_ACC_MD3(xiIndex, 0, 1)] -
                          f[OPS_ACC_MD3(xiIndex, 0, 0)]) /
                         (coordinateXYZ[OPS_ACC_MD0(1, 0, 1)] -
                          coordinateXYZ[OPS_ACC_MD0(1, 0, 0)]));
                }
            }
        }
    }
}
void KerCutCellCVTUpwind2nd(const Real* coordinateXYZ, const int* nodeType,
                            const int* geometry, const Real* f,
                            Real* fGradient) {
    VertexTypes vt = (VertexTypes)nodeType[OPS_ACC1(0, 0)];
    VertexGeometryTypes vg = (VertexGeometryTypes)geometry[OPS_ACC2(0, 0)];
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        for (int xiIndex = COMPOINDEX[2 * compoIndex];
             xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
            Real cx{XI[xiIndex * LATTDIM]};
            Real cy{XI[xiIndex * LATTDIM + 1]};
            bool reduceOrderX{false};
            bool reduceOrderY{false};
            bool needCalc{true};
            // setting a initial value
            fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] = 0;
            if (vt == Vertex_ImmersedSolid) needCalc = false;
            // make sure no calculation occurring at boundary when unnecessary
            // this can avoid access undefined memory if the halo point is set
            // incorrectly
            if (vt >= Vertex_Fluid && vt < Vertex_Boundary) {
                // current node is a fluid point
                if (cx > 0) {
                    VertexTypes vtUpwind =
                        (VertexTypes)nodeType[OPS_ACC1(-1, 0)];
                    VertexGeometryTypes vgUpwind =
                        (VertexGeometryTypes)geometry[OPS_ACC2(-1, 0)];
                    if (vtUpwind >= Vertex_Boundary && vgUpwind == VG_IP) {
                        // if the upwind node is boundary and VG_IP point
                        reduceOrderX = true;
                    }
                }  // cx >0
                if (cx < 0) {
                    VertexTypes vtUpwind =
                        (VertexTypes)nodeType[OPS_ACC1(1, 0)];
                    VertexGeometryTypes vgUpwind =
                        (VertexGeometryTypes)geometry[OPS_ACC2(1, 0)];
                    if (vtUpwind >= Vertex_Boundary && vgUpwind == VG_IM) {
                        // if the upwind node is boundary and VG_IM point
                        reduceOrderX = true;
                    }
                }  // Cx<0
                if (cy > 0) {
                    VertexTypes vtUpwind =
                        (VertexTypes)nodeType[OPS_ACC1(0, -1)];
                    VertexGeometryTypes vgUpwind =
                        (VertexGeometryTypes)geometry[OPS_ACC2(0, -1)];
                    if (vtUpwind >= Vertex_Boundary && vgUpwind == VG_JP) {
                        reduceOrderY = true;
                    }
                }  // cy >0
                if (cy < 0) {
                    VertexTypes vtUpwind =
                        (VertexTypes)nodeType[OPS_ACC1(0, 1)];
                    VertexGeometryTypes vgUpwind =
                        (VertexGeometryTypes)geometry[OPS_ACC2(0, 1)];
                    if (vtUpwind >= Vertex_Boundary && vgUpwind == VG_JM) {
                        reduceOrderY = true;
                    }
                }  // cy < 0
            }      // current node is a fluid point
            if (vt >= Vertex_Boundary) {
                switch (vg) {
                    case VG_IP:
                        if (cx > 0) {
                            needCalc = false;
                        } else {
                            if (cy > 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(0, -1)];
                                if (((vgUpwind == VG_IPJM_O ||
                                      vgUpwind == VG_IPJP_O) &&
                                     (((VertexTypes)
                                           nodeType[OPS_ACC2(0, -2)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IPJM_I) ||
                                    (vgUpwind == VG_IPJP_I)) {
                                    reduceOrderY = true;
                                }
                            }  // cy > 0 VG_IP
                            if (cy < 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(0, 1)];
                                if (((vgUpwind == VG_IPJM_O ||
                                      vgUpwind == VG_IPJP_O) &&
                                     (((VertexTypes)nodeType[OPS_ACC2(0, 2)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IPJM_I) ||
                                    (vgUpwind == VG_IPJP_I)) {
                                    reduceOrderY = true;
                                }
                            }  // Cy < 0 VG_IP
                        }
                        break;
                    case VG_IM:
                        if (cx < 0) {
                            needCalc = false;
                        } else {
                            if (cy > 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(0, -1)];
                                if (((vgUpwind == VG_IMJM_O ||
                                      vgUpwind == VG_IMJP_O) &&
                                     (((VertexTypes)
                                           nodeType[OPS_ACC2(0, -2)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IMJM_I) ||
                                    (vgUpwind == VG_IMJP_I)) {
                                    reduceOrderY = true;
                                }
                            }  // cy > 0 VG_IM
                            if (cy < 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(0, 1)];
                                if (((vgUpwind == VG_IMJM_O ||
                                      vgUpwind == VG_IMJP_O) &&
                                     (((VertexTypes)nodeType[OPS_ACC2(0, 2)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IMJM_I) ||
                                    (vgUpwind == VG_IMJP_I)) {
                                    reduceOrderY = true;
                                }
                            }  // cy < 0 VG_IM
                        }
                        break;
                    case VG_JP:
                        if (cy > 0) {
                            needCalc = false;
                        } else {
                            if (cx > 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(-1, 0)];
                                if (((vgUpwind == VG_IMJP_O ||
                                      vgUpwind == VG_IPJP_O) &&
                                     (((VertexTypes)
                                           nodeType[OPS_ACC2(-2, 0)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IMJP_I) ||
                                    (vgUpwind == VG_IPJP_I)) {
                                    reduceOrderX = true;
                                }
                            }  // cx > 0  VG_JP
                            if (cx < 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(1, 0)];
                                if (((vgUpwind == VG_IMJP_O ||
                                      vgUpwind == VG_IPJP_O) &&
                                     (((VertexTypes)nodeType[OPS_ACC2(2, 0)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IMJP_I) ||
                                    (vgUpwind == VG_IPJP_I)) {
                                    reduceOrderX = true;
                                }
                            }  // cx <0 VG_JP
                        }
                        break;
                    case VG_JM:
                        if (cy < 0) {
                            needCalc = false;
                        } else {
                            if (cx > 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(-1, 0)];
                                if (((vgUpwind == VG_IMJM_O ||
                                      vgUpwind == VG_IPJM_O) &&
                                     (((VertexTypes)
                                           nodeType[OPS_ACC2(-2, 0)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IMJM_I) ||
                                    (vgUpwind == VG_IPJM_I)) {
                                    reduceOrderX = true;
                                }
                            }  // cx > 0 VG_JM
                            if (cx < 0) {
                                VertexGeometryTypes vgUpwind =
                                    (VertexGeometryTypes)
                                        geometry[OPS_ACC2(1, 0)];
                                if (((vgUpwind == VG_IPJM_O ||
                                      vgUpwind == VG_IMJM_O) &&
                                     (((VertexTypes)nodeType[OPS_ACC2(2, 0)]) ==
                                      Vertex_ImmersedSolid)) ||
                                    (vgUpwind == VG_IMJM_I) ||
                                    (vgUpwind == VG_IPJM_I)) {
                                    reduceOrderX = true;
                                }
                            }  // cx > 0 VG_JM
                        }
                        break;
                    case VG_IPJP_I:
                        if (cy > 0 || cx > 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(1, 0)];
                            if (vgUpwind == VG_IMJP_I) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, 1)];
                            if (vgUpwind == VG_IPJM_I) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IPJM_I:
                        if (cy < 0 || cx > 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(1, 0)];
                            if (vgUpwind == VG_IMJM_I) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, -1)];
                            if (vgUpwind == VG_IPJP_I) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IMJP_I:
                        if (cy > 0 || cx < 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(-1, 0)];
                            if (vgUpwind == VG_IPJP_I) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, 1)];
                            if (vgUpwind == VG_IMJM_I) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IMJM_I:
                        if (cy < 0 || cx < 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(-1, 0)];
                            if (vgUpwind == VG_IPJM_I) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, -1)];
                            if (vgUpwind == VG_IMJP_I) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IPJP_O:
                        if (cy >= 0 && cx >= 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(1, 0)];
                            if (vgUpwind == VG_IM) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, 1)];
                            if (vgUpwind == VG_JM) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IPJM_O:
                        if (cy <= 0 && cx >= 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(1, 0)];
                            if (vgUpwind == VG_IM) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, -1)];
                            if (vgUpwind == VG_JP) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IMJP_O:
                        if (cy >= 0 && cx <= 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(-1, 0)];
                            if (vgUpwind == VG_IP) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, 1)];
                            if (vgUpwind == VG_JM) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    case VG_IMJM_O:
                        if (cy <= 0 && cx <= 0) {
                            needCalc = false;
                        } else {
                            VertexGeometryTypes vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(-1, 0)];
                            if (vgUpwind == VG_IP) {
                                reduceOrderX = true;
                            }
                            vgUpwind =
                                (VertexGeometryTypes)geometry[OPS_ACC2(0, -1)];
                            if (vgUpwind == VG_JP) {
                                reduceOrderY = true;
                            }
                        }
                        break;
                    default:
                        break;
                }
            }  // if the current node is a boundary node
            if (needCalc) {
                if (cx > 0) {
                    if (reduceOrderX) {
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cx *
                            ((f[OPS_ACC_MD3(xiIndex, 0, 0)] -
                              f[OPS_ACC_MD3(xiIndex, -1, 0)]) /
                             (coordinateXYZ[OPS_ACC_MD0(0, 0, 0)] -
                              coordinateXYZ[OPS_ACC_MD0(0, -1, 0)]));
                    } else {
                        Real x_2{coordinateXYZ[OPS_ACC_MD0(0, -2, 0)]};
                        Real x_1{coordinateXYZ[OPS_ACC_MD0(0, -1, 0)]};
                        Real x_0{coordinateXYZ[OPS_ACC_MD0(0, 0, 0)]};
                        // ops_printf("x_2=%f x_1=%f x_0=%f\n",x_2,x_1,x_0);
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cx *
                            (f[OPS_ACC_MD3(xiIndex, -2, 0)] *
                                 (1 / (x_2 - x_0) - 1 / (x_2 - x_1)) +
                             f[OPS_ACC_MD3(xiIndex, -1, 0)] *
                                 (1 / (x_1 - x_0) - 1 / (x_1 - x_2)) +
                             f[OPS_ACC_MD3(xiIndex, 0, 0)] *
                                 (1 / (x_0 - x_1) + 1 / (x_0 - x_2)));
                    }
                }
                if (cx < 0) {
                    if (reduceOrderX) {
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cx *
                            ((f[OPS_ACC_MD3(xiIndex, 1, 0)] -
                              f[OPS_ACC_MD3(xiIndex, 0, 0)]) /
                             (coordinateXYZ[OPS_ACC_MD0(0, 1, 0)] -
                              coordinateXYZ[OPS_ACC_MD0(0, 0, 0)]));
                    } else {
                        Real xp2{coordinateXYZ[OPS_ACC_MD0(0, 2, 0)]};
                        Real xp1{coordinateXYZ[OPS_ACC_MD0(0, 1, 0)]};
                        Real x0{coordinateXYZ[OPS_ACC_MD0(0, 0, 0)]};
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cx *
                            (f[OPS_ACC_MD3(xiIndex, 0, 0)] *
                                 (1 / (x0 - xp2) + 1 / (x0 - xp1)) +
                             f[OPS_ACC_MD3(xiIndex, 1, 0)] *
                                 (1 / (xp2 - xp1) + 1 / (xp1 - x0)) +
                             f[OPS_ACC_MD3(xiIndex, 2, 0)] *
                                 (1 / (xp2 - x0) + 1 / (xp1 - xp2)));
                    }
                }
                if (cy > 0) {
                    if (reduceOrderY) {
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cy *
                            ((f[OPS_ACC_MD3(xiIndex, 0, 0)] -
                              f[OPS_ACC_MD3(xiIndex, 0, -1)]) /
                             (coordinateXYZ[OPS_ACC_MD0(1, 0, 0)] -
                              coordinateXYZ[OPS_ACC_MD0(1, 0, -1)]));
                    } else {
                        Real y_2{coordinateXYZ[OPS_ACC_MD0(1, 0, -2)]};
                        Real y_1{coordinateXYZ[OPS_ACC_MD0(1, 0, -1)]};
                        Real y_0{coordinateXYZ[OPS_ACC_MD0(1, 0, 0)]};
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cy *
                            (f[OPS_ACC_MD3(xiIndex, 0, -2)] *
                                 (1 / (y_2 - y_0) - 1 / (y_2 - y_1)) +
                             f[OPS_ACC_MD3(xiIndex, 0, -1)] *
                                 (1 / (y_1 - y_0) - 1 / (y_1 - y_2)) +
                             f[OPS_ACC_MD3(xiIndex, 0, 0)] *
                                 (1 / (y_0 - y_1) + 1 / (y_0 - y_2)));
                    }
                }
                if (cy < 0) {
                    if (reduceOrderY) {
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cy *
                            ((f[OPS_ACC_MD3(xiIndex, 0, 1)] -
                              f[OPS_ACC_MD3(xiIndex, 0, 0)]) /
                             (coordinateXYZ[OPS_ACC_MD0(1, 0, 1)] -
                              coordinateXYZ[OPS_ACC_MD0(1, 0, 0)]));
                    } else {
                        Real yp2{coordinateXYZ[OPS_ACC_MD0(1, 0, 2)]};
                        Real yp1{coordinateXYZ[OPS_ACC_MD0(1, 0, 1)]};
                        Real y0{coordinateXYZ[OPS_ACC_MD0(1, 0, 0)]};
                        fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +=
                            CS * cy *
                            (f[OPS_ACC_MD3(xiIndex, 0, 0)] *
                                 (1 / (y0 - yp2) + 1 / (y0 - yp1)) +
                             f[OPS_ACC_MD3(xiIndex, 0, 1)] *
                                 (1 / (yp2 - yp1) + 1 / (yp1 - y0)) +
                             f[OPS_ACC_MD3(xiIndex, 0, 2)] *
                                 (1 / (yp2 - y0) + 1 / (yp1 - yp2)));
                    }
                }
            }
        }
    }
}
void KerCutCellSemiImplicitTimeMach(const Real* dt, const Real* schemeCoeff,
                                    const int* nodeType, const int* geometry,
                                    const Real* fGradient, const Real* feq,
                                    const Real* relaxationTime,
                                    const Real* bodyForce, Real* f) {
    /*
    dt(0), schemeCoeff(1),nodeType(2),geometry(3), fGradient (4),
    feq(5),
    relaxationTime(6), bodyForce(7), f(8)
    */
    VertexTypes vt = (VertexTypes)nodeType[OPS_ACC2(0, 0)];
    VertexGeometryTypes vg = (VertexGeometryTypes)geometry[OPS_ACC3(0, 0)];
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        Real tau = relaxationTime[OPS_ACC_MD6(compoIndex, 0, 0)];
        for (int xiIndex = COMPOINDEX[2 * compoIndex];
             xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
            Real cx{XI[xiIndex * LATTDIM]};
            Real cy{XI[xiIndex * LATTDIM + 1]};
            bool needMarch{true};
            if (vt == Vertex_ImmersedSolid) needMarch = false;
            if (vt >= Vertex_Boundary) {
                switch (vg) {
                    case VG_IP:
                        if (cx > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IM:
                        if (cx < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_JP:
                        if (cy > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_JM:
                        if (cy < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJP_I:
                        if (cy > 0 || cx > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJM_I:
                        if (cy < 0 || cx > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJP_I:
                        if (cy > 0 || cx < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJM_I:
                        if (cy < 0 || cx < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJP_O:
                        if (cy >= 0 && cx >= 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJM_O:
                        if (cy <= 0 && cx >= 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJP_O:
                        if (cy >= 0 && cx <= 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJM_O:
                        if (cy <= 0 && cx <= 0) {
                            needMarch = false;
                        }
                        break;
                    default:
                        break;
                }
            }
            if (needMarch) {
                f[OPS_ACC_MD8(xiIndex, 0, 0)] =
                    (feq[OPS_ACC_MD5(xiIndex, 0, 0)] * (*dt) +
                     bodyForce[OPS_ACC_MD7(xiIndex, 0, 0)] * (*dt) * tau -
                     fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] * (*dt) * tau +
                     f[OPS_ACC_MD8(xiIndex, 0, 0)] * tau) /
                    ((*dt) + tau);
            }
        }
    }
}
void KerCutCellExplicitTimeMach(const Real* dt, const Real* schemeCoeff,
                                const int* nodeType, const int* geometry,
                                const Real* fGradient, const Real* feq,
                                const Real* relaxationTime,
                                const Real* bodyForce, Real* f) {
    /*
    dt(0), schemeCoeff(1),nodeType(2),geometry(3), fGradient (4),
    feq(5),
    relaxationTime(6), bodyForce(7), f(8)
    */
    VertexTypes vt = (VertexTypes)nodeType[OPS_ACC2(0, 0)];
    VertexGeometryTypes vg = (VertexGeometryTypes)geometry[OPS_ACC3(0, 0)];
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        Real tau = relaxationTime[OPS_ACC_MD6(compoIndex, 0, 0)];
        for (int xiIndex = COMPOINDEX[2 * compoIndex];
             xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
            Real cx{XI[xiIndex * LATTDIM]};
            Real cy{XI[xiIndex * LATTDIM + 1]};
            bool needMarch{true};
            if (vt == Vertex_ImmersedSolid) needMarch = false;
            if (vt >= Vertex_Boundary) {
                switch (vg) {
                    case VG_IP:
                        if (cx > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IM:
                        if (cx < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_JP:
                        if (cy > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_JM:
                        if (cy < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJP_I:
                        if (cy > 0 || cx > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJM_I:
                        if (cy < 0 || cx > 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJP_I:
                        if (cy > 0 || cx < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJM_I:
                        if (cy < 0 || cx < 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJP_O:
                        if (cy >= 0 && cx >= 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IPJM_O:
                        if (cy <= 0 && cx >= 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJP_O:
                        if (cy >= 0 && cx <= 0) {
                            needMarch = false;
                        }
                        break;
                    case VG_IMJM_O:
                        if (cy <= 0 && cx <= 0) {
                            needMarch = false;
                        }
                        break;
                    default:
                        break;
                }
            }
            if (needMarch) {
                f[OPS_ACC_MD8(xiIndex, 0, 0)] =
                    f[OPS_ACC_MD8(xiIndex, 0, 0)] +
                    (*dt) * (*schemeCoeff) *
                        (bodyForce[OPS_ACC_MD7(xiIndex, 0, 0)] -
                         fGradient[OPS_ACC_MD4(xiIndex, 0, 0)] +
                         (feq[OPS_ACC_MD5(xiIndex, 0, 0)] -
                          f[OPS_ACC_MD8(xiIndex, 0, 0)]) /
                             tau);
            }
        }
    }
}
#endif
#ifdef OPS_3D  // three dimensional code

void KerCollide3D(const Real* dt, const int* nodeType, const Real* f,
                  const Real* feq, const Real* relaxationTime,
                  const Real* bodyForce, Real* fStage) {
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        // collisionRequired: means if collision is required at boundary
        // e.g., the ZouHe boundary condition explicitly requires collision
        VertexTypes vt =
            (VertexTypes)nodeType[OPS_ACC_MD1(compoIndex, 0, 0, 0)];
        bool collisionRequired =
            (vt == Vertex_Fluid ||
             vt == Vertex_ZouHeVelocity ||
             vt == Vertex_EQMDiffuseRefl ||
             vt == Vertex_ExtrapolPressure1ST ||
              vt == Vertex_Periodic
              );
        if (collisionRequired) {
            Real tau = relaxationTime[OPS_ACC_MD4(compoIndex, 0, 0, 0)];
            Real dtOvertauPlusdt = (*dt) / (tau + 0.5 * (*dt));
            for (int xiIndex = COMPOINDEX[2 * compoIndex];
                 xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
                fStage[OPS_ACC_MD6(xiIndex, 0, 0, 0)] =
                    f[OPS_ACC_MD2(xiIndex, 0, 0, 0)] -
                    dtOvertauPlusdt * (f[OPS_ACC_MD2(xiIndex, 0, 0, 0)] -
                                       feq[OPS_ACC_MD3(xiIndex, 0, 0, 0)]) +
                    tau * dtOvertauPlusdt *
                        bodyForce[OPS_ACC_MD5(xiIndex, 0, 0, 0)];
#ifdef CPU
                const Real res{fStage[OPS_ACC_MD6(xiIndex, 0, 0, 0)]};
                if (isnan(res) || res <= 0 || isinf(res)) {
                    ops_printf(
                        "Error! Distribution function %f becomes "
                        "invalid for the component %i at  the lattice "
                        "%i\n",
                        res, compoIndex, xiIndex);
                    assert(!(isnan(res) || res <= 0 || isinf(res)));
                }
#endif
            }
        }
    }
}

void KerStream3D(const int* nodeType, const int* geometry, const Real* fStage,
                 Real* f) {
    VertexGeometryTypes vg = (VertexGeometryTypes)geometry[OPS_ACC1(0, 0, 0)];
    for (int compoIndex = 0; compoIndex < NUMCOMPONENTS; compoIndex++) {
        VertexTypes vt = (VertexTypes)nodeType[OPS_ACC_MD0(compoIndex,0, 0, 0)];
        for (int xiIndex = COMPOINDEX[2 * compoIndex];
             xiIndex <= COMPOINDEX[2 * compoIndex + 1]; xiIndex++) {
            int cx = (int)XI[xiIndex * LATTDIM];
            int cy = (int)XI[xiIndex * LATTDIM + 1];
            int cz = (int)XI[xiIndex * LATTDIM + 2];
            if ((vt >= Vertex_Fluid) && (vt < Vertex_Boundary)) {
                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
            }
            if (vt >= Vertex_Boundary) {
                // streamRequired: means if the particles with velocity parallel
                // needs to be streamed at the boundary
                bool streamRequired =
                    (
                     vt == Vertex_EQMDiffuseRefl ||
                     vt == Vertex_ExtrapolPressure1ST ||
                     vt == Vertex_Periodic
                     );
                if (streamRequired) {
                    if ((cx == 0) && (cy == 0) && (cz == 0)) {
                        f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                            fStage[OPS_ACC_MD2(xiIndex, 0, 0, 0)];
                        continue;
                    }
                }
                switch (vg) {
                        // faces six types
                    case VG_IP:
                        // (cx=0 means stream is implemented at i=0,so here we
                        //  disable the step at boundary)
                        if (streamRequired) {
                            if (cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IM:
                        if (streamRequired) {
                            if (cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JP:
                        if (streamRequired) {
                            if (cy <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JM:
                        if (streamRequired) {
                            if (cy >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_KP:
                        // (cx=0 means stream is implemented at i=0,so here we
                        //  disable the step at boundary)
                        if (streamRequired) {
                            if (cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_KM:
                        if (streamRequired) {
                            if (cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }

                        } else {
                            if (cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    // faces six types end
                    // 12 edges
                    case VG_IPJP_I:
                        if (streamRequired) {
                            if (cy <= 0 && cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy < 0 && cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJM_I:
                        if (streamRequired) {
                            if (cy >= 0 && cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy > 0 && cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJP_I:
                        if (streamRequired) {
                            if (cy <= 0 && cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy < 0 && cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJM_I:
                        if (streamRequired) {
                            if (cy >= 0 && cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy > 0 && cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    // k
                    case VG_IPKP_I:
                        if (streamRequired) {
                            if (cz <= 0 && cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 && cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPKM_I:
                        if (streamRequired) {
                            if (cz >= 0 && cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 && cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMKP_I:
                        if (streamRequired) {
                            if (cz <= 0 && cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 && cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMKM_I:
                        if (streamRequired) {
                            if (cz >= 0 && cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 && cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JPKP_I:
                        if (streamRequired) {
                            if (cz <= 0 && cy <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 && cy < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JPKM_I:
                        if (streamRequired) {
                            if (cz >= 0 && cy <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 && cy < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JMKP_I:
                        if (streamRequired) {
                            if (cz <= 0 && cy >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 && cy > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JMKM_I:
                        if (streamRequired) {
                            if (cz >= 0 && cy >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 && cy > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    // K
                    // k_out
                    case VG_IPJP_O:
                        if (streamRequired) {
                            if (cy <= 0 || cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy < 0 || cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJM_O:
                        if (streamRequired) {
                            if (cy >= 0 || cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy > 0 || cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJP_O:
                        if (streamRequired) {
                            if (cy <= 0 || cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy < 0 || cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJM_O:
                        if (streamRequired) {
                            if (cy >= 0 || cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cy > 0 || cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                        // IK
                    case VG_IPKP_O:
                        if (streamRequired) {
                            if (cz <= 0 || cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 || cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPKM_O:
                        if (streamRequired) {
                            if (cz >= 0 || cx <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 || cx < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMKP_O:
                        if (streamRequired) {
                            if (cz <= 0 || cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 || cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMKM_O:
                        if (streamRequired) {
                            if (cz >= 0 || cx >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 || cx > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                        // JK
                    case VG_JPKP_O:
                        if (streamRequired) {
                            if (cz <= 0 || cy <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 || cy < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JPKM_O:
                        if (streamRequired) {
                            if (cz >= 0 || cy <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 || cy < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JMKP_O:
                        if (streamRequired) {
                            if (cz <= 0 || cy >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz < 0 || cy > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_JMKM_O:
                        if (streamRequired) {
                            if (cz >= 0 || cy >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cz > 0 || cy > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    // k_out end
                    // 12 edges end
                    // 8 corners
                    // inner corners
                    case VG_IPJPKP_I:
                        if (streamRequired) {
                            if (cx <= 0 && cy <= 0 && cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 && cy < 0 && cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJPKM_I:
                        if (streamRequired) {
                            if (cx <= 0 && cy <= 0 && cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 && cy < 0 && cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJMKP_I:
                        if (streamRequired) {
                            if (cx <= 0 && cy >= 0 && cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 && cy > 0 && cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJMKM_I:
                        if (streamRequired) {
                            if (cx <= 0 && cy >= 0 && cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 && cy > 0 && cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJPKP_I:
                        if (streamRequired) {
                            if (cx >= 0 && cy <= 0 && cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 && cy < 0 && cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJPKM_I:
                        if (streamRequired) {
                            if (cx >= 0 && cy <= 0 && cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 && cy < 0 && cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJMKP_I:
                        if (streamRequired) {
                            if (cx >= 0 && cy >= 0 && cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 && cy > 0 && cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJMKM_I:
                        if (streamRequired) {
                            if (cx >= 0 && cy >= 0 && cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 && cy > 0 && cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    // out corner
                    case VG_IPJPKP_O:
                        if (streamRequired) {
                            if (cx <= 0 || cy <= 0 || cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 || cy < 0 || cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJPKM_O:
                        if (streamRequired) {
                            if (cx <= 0 || cy <= 0 || cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 || cy < 0 || cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJMKP_O:
                        if (streamRequired) {
                            if (cx <= 0 || cy >= 0 || cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 || cy > 0 || cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IPJMKM_O:
                        if (streamRequired) {
                            if (cx <= 0 || cy >= 0 || cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx < 0 || cy > 0 || cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJPKP_O:
                        if (streamRequired) {
                            if (cx >= 0 || cy <= 0 || cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 || cy < 0 || cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJPKM_O:
                        if (streamRequired) {
                            if (cx >= 0 || cy <= 0 || cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 || cy < 0 || cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJMKP_O:
                        if (streamRequired) {
                            if (cx >= 0 || cy >= 0 || cz <= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 || cy > 0 || cz < 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    case VG_IMJMKM_O:
                        if (streamRequired) {
                            if (cx >= 0 || cy >= 0 || cz >= 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        } else {
                            if (cx > 0 || cy > 0 || cz > 0) {
                                f[OPS_ACC_MD3(xiIndex, 0, 0, 0)] =
                                    fStage[OPS_ACC_MD2(xiIndex, -cx, -cy, -cz)];
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

#endif  // OPS_3D
void KerSetGeometryProperty(const int* value, int* var) {
#ifdef OPS_2D
    var[OPS_ACC1(0, 0)] = (*value);
#endif
#ifdef OPS_3D
    var[OPS_ACC1(0, 0, 0)] = (*value);
#endif
}

void KerSetNodeType(const int* value, int* var, const int* compoId) {
#ifdef OPS_2D
    var[OPS_ACC_MD1((*compoId), 0, 0)] = (*value);
#endif
#ifdef OPS_3D
    var[OPS_ACC_MD1((*compoId), 0, 0, 0)] = (*value);
#endif
}

void KerSetMacroVarToConst(const Real* value, Real* macroVar) {
    for (int macroVarIndex = 0; macroVarIndex < NUMMACROVAR; macroVarIndex++) {
#ifdef OPS_2D
        macroVar[OPS_ACC_MD1(macroVarIndex, 0, 0)] = value[macroVarIndex];
#endif
#ifdef OPS_3D
        macroVar[OPS_ACC_MD1(macroVarIndex, 0, 0, 0)] = value[macroVarIndex];
#endif
    }
}
void KerCopyf(const Real* src, Real* dest) {
    for (int xiIndex = 0; xiIndex < NUMXI; xiIndex++) {
#ifdef OPS_2D
        dest[OPS_ACC_MD1(xiIndex, 0, 0)] = src[OPS_ACC_MD0(xiIndex, 0, 0)];
#endif
#ifdef OPS_3D
        dest[OPS_ACC_MD1(xiIndex, 0, 0, 0)] =
            src[OPS_ACC_MD0(xiIndex, 0, 0, 0)];
#endif
    }
}
void KerCopyProperty(const int* src, int* dest) {
#ifdef OPS_2D
    dest[OPS_ACC1(0, 0)] = src[OPS_ACC0(0, 0)];
#endif
#ifdef OPS_3D
    dest[OPS_ACC1(0, 0, 0)] = src[OPS_ACC0(0, 0, 0)];
#endif
}
void KerCopyMacroVars(const Real* src, Real* dest) {
    for (int idx = 0; idx < NUMMACROVAR; idx++) {
#ifdef OPS_2D
        dest[OPS_ACC_MD1(idx, 0, 0)] = src[OPS_ACC_MD0(idx, 0, 0)];
#endif
#ifdef OPS_3D
        dest[OPS_ACC_MD1(idx, 0, 0, 0)] = src[OPS_ACC_MD0(idx, 0, 0, 0)];
#endif
    }
}
void KerCopyCoordinateXYZ(const Real* src, Real* dest) {
    for (int idx = 0; idx < SPACEDIM; idx++) {
#ifdef OPS_2D
        dest[OPS_ACC_MD1(idx, 0, 0)] = src[OPS_ACC_MD0(idx, 0, 0)];
#endif
#ifdef OPS_3D
        dest[OPS_ACC_MD1(idx, 0, 0, 0)] = src[OPS_ACC_MD0(idx, 0, 0, 0)];
#endif
    }
}
void KerCopyDispf(const Real* src, Real* dest, const int* disp) {
    for (int xiIndex = 0; xiIndex < NUMXI; xiIndex++) {
#ifdef OPS_2D
        dest[OPS_ACC_MD1(xiIndex, disp[0], disp[1])] =
            src[OPS_ACC_MD0(xiIndex, 0, 0)];
#endif
#ifdef OPS_3D
        dest[OPS_ACC_MD1(xiIndex, disp[0], disp[1], disp[2])] =
            src[OPS_ACC_MD0(xiIndex, 0, 0, 0)];
#endif
    }
}

void KerCalcMacroVarSquareofDifference(const Real* macroVars,
                                       const Real* macroVarsCopy,
                                       const int* varId,
                                       double* sumSquareDiff) {
#ifdef OPS_2D
    *sumSquareDiff =
        *sumSquareDiff + (macroVars[OPS_ACC_MD0(*varId, 0, 0)] -
                          macroVarsCopy[OPS_ACC_MD1(*varId, 0, 0)]) *
                             (macroVars[OPS_ACC_MD0(*varId, 0, 0)] -
                              macroVarsCopy[OPS_ACC_MD1(*varId, 0, 0)]);
#endif
#ifdef OPS_3D
    *sumSquareDiff =
        *sumSquareDiff + (macroVars[OPS_ACC_MD0(*varId, 0, 0, 0)] -
                          macroVarsCopy[OPS_ACC_MD1(*varId, 0, 0, 0)]) *
                             (macroVars[OPS_ACC_MD0(*varId, 0, 0, 0)] -
                              macroVarsCopy[OPS_ACC_MD1(*varId, 0, 0, 0)]);
#endif
}
void KerCalcMacroVarSquare(const Real* macroVars, const int* varId,
                           double* sumSquare) {
#ifdef OPS_2D
    *sumSquare = *sumSquare + (macroVars[OPS_ACC_MD0(*varId, 0, 0)]) *
                                  (macroVars[OPS_ACC_MD0(*varId, 0, 0)]);
#endif
#ifdef OPS_3D
    *sumSquare = *sumSquare + (macroVars[OPS_ACC_MD0(*varId, 0, 0, 0)]) *
                                  (macroVars[OPS_ACC_MD0(*varId, 0, 0, 0)]);
#endif
}

void KerSetfFixValue(const Real* value, Real* f) {
    for (int xiIndex = 0; xiIndex < NUMXI; xiIndex++) {
#ifdef OPS_2D
        f[OPS_ACC_MD1(xiIndex, 0, 0)] = (*value);
#endif
#ifdef OPS_3D
        f[OPS_ACC_MD1(xiIndex, 0, 0, 0)] = (*value);
#endif
    }
}

void KerGetPointMacroVarValue(const Real* macroVars, Real* pointValue) {
#ifdef OPS_2D
    *pointValue = *pointValue + macroVars[OPS_ACC_MD0(1, 0, 0)] /
                                    macroVars[OPS_ACC_MD0(0, 0, 0)];
#endif
#ifdef OPS_3D
    *pointValue = *pointValue + macroVars[OPS_ACC_MD0(1, 0, 0, 0)] /
                                    macroVars[OPS_ACC_MD0(0, 0, 0, 0)];
#endif
}
#endif  // SCHEME_KERNEL_H