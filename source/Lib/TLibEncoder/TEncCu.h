/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.  
 *
 * Copyright (c) 2010-2013, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TEncCu.h
    \brief    Coding Unit (CU) encoder class (header)
*/

#ifndef __TENCCU__
#define __TENCCU__

// Include files
#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComPrediction.h"
#include "TLibCommon/TComTrQuant.h"
#include "TLibCommon/TComBitCounter.h"
#include "TLibCommon/TComDataCU.h"

#include "TEncEntropy.h"
#include "TEncSearch.h"
#include "TEncRateCtrl.h"
//! \ingroup TLibEncoder
//! \{

class TEncTop;
class TEncSbac;
class TEncCavlc;
class TEncSlice;

// ====================================================================================================================
// Class definition
// ====================================================================================================================

/// CU encoder class
class TEncCu
{
  struct DATA{
    TComDataCU* bestCU;
    TComDataCU* tempCU;

    TComYuv*    predYuvBest; ///< Best Prediction Yuv for each depth
    TComYuv*    resiYuvBest; ///< Best Residual Yuv for each depth
    TComYuv*    recoYuvBest; ///< Best Reconstruction Yuv for each depth
    TComYuv*    predYuvTemp; ///< Temporary Prediction Yuv for each depth
    TComYuv*    resiYuvTemp; ///< Temporary Residual Yuv for each depth
    TComYuv*    recoYuvTemp; ///< Temporary Reconstruction Yuv for each depth
    TComYuv*    origYuv;     ///< Original Yuv for each depth

    Bool dQPFlag;
  };
  typedef struct DATA DATA;

private:
  
//  TComDataCU**            m_ppcBestCU;      ///< Best CUs in each depth
//  TComDataCU**            m_ppcTempCU;      ///< Temporary CUs in each depth
  UChar                   m_uhTotalDepth;
  UInt                    m_uiMaxWidth;
  UInt                    m_uiMaxHeight;
  
//  TComYuv**               m_ppcPredYuvBest; ///< Best Prediction Yuv for each depth
//  TComYuv**               m_ppcResiYuvBest; ///< Best Residual Yuv for each depth
//  TComYuv**               m_ppcRecoYuvBest; ///< Best Reconstruction Yuv for each depth
//  TComYuv**               m_ppcPredYuvTemp; ///< Temporary Prediction Yuv for each depth
//  TComYuv**               m_ppcResiYuvTemp; ///< Temporary Residual Yuv for each depth
//  TComYuv**               m_ppcRecoYuvTemp; ///< Temporary Reconstruction Yuv for each depth
//  TComYuv**               m_ppcOrigYuv;     ///< Original Yuv for each depth
  
  //  Data : encoder control
  Bool                    m_bEncodeDQP;
  
  //  Access channel
  TEncCfg*                m_pcEncCfg;
  TEncSearch*             m_pcPredSearch;
  TComTrQuant*            m_pcTrQuant;
  TComBitCounter*         m_pcBitCounter;
  TComRdCost*             m_pcRdCost;
  
  TEncEntropy*            m_pcEntropyCoder;
//  TEncCavlc*              m_pcCavlcCoder;
//  TEncSbac*               m_pcSbacCoder;
//  TEncBinCABAC*           m_pcBinCABAC;
  
  // SBAC RD
  TEncSbac***             m_pppcRDSbacCoder;
  TEncSbac*               m_pcRDGoOnSbacCoder;
  Bool                    m_bUseSBACRD;
  TEncRateCtrl*           m_pcRateCtrl;
#if RATE_CONTROL_LAMBDA_DOMAIN && !M0036_RC_IMPROVEMENT
  UInt                    m_LCUPredictionSAD;
  Int                     m_addSADDepth;
  Int                     m_temporalSAD;
#endif
public:
  /// copy parameters from encoder class
  Void  init                ( TEncTop* pcEncTop );
  
  /// create internal buffers
  Void  create              ( UChar uhTotalDepth, UInt iMaxWidth, UInt iMaxHeight );
  
  /// destroy internal buffers
  Void  destroy             ();
 
  Void init_predSearch      (TEncSearch *search);
  /// CU analysis function
  Void  compressCU          ( TEncSlice *slice, TComDataCU*&  rpcCU );
  
  /// CU encoding function
  Void  encodeCU            ( TComDataCU*    pcCU );
  
  Void setBitCounter        ( TComBitCounter* pcBitCounter ) { m_pcBitCounter = pcBitCounter; }
#if RATE_CONTROL_LAMBDA_DOMAIN && !M0036_RC_IMPROVEMENT
  UInt getLCUPredictionSAD() { return m_LCUPredictionSAD; }
#endif
#if RATE_CONTROL_INTRA
  Int   updateLCUDataISlice ( TComDataCU* pcCU, Int LCUIdx, Int width, Int height );
#endif
protected:
  Void  create_DATA         ( DATA& data, UInt depth );
  Void  destroy_DATA        ( DATA& data );

  Void  finishCU            ( TComDataCU*  pcCU, UInt uiAbsPartIdx,           UInt uiDepth        );

  Void  xCompressCUPart     ( TEncSearch *search, DATA *data, DATA *subData, TComSlice* pcSlice, UInt uiPartUnitIdx, UInt iQP, UInt uiDepth, UInt uhNextDepth, TEncSbac* pppcRDSbacCoder_curr_best);

#if AMP_ENC_SPEEDUP
  Void  xCompressCU         ( TEncSearch *search, DATA &data, UInt uiDepth, TEncSbac* curr_sbac, PartSize eParentPartSize = SIZE_NONE);
#else
  Void  xCompressCU         ( TEncSearch *search, TComDataCU*& rpcBestCU, TComDataCU*& rpcTempCU, UInt uiDepth        );
#endif
  Void  xEncodeCU           ( TComDataCU*  pcCU, UInt uiAbsPartIdx,           UInt uiDepth        );
  
  Int   xComputeQP          ( TComDataCU* pcCU, UInt uiDepth );
  Void  xCheckBestMode      ( TEncSearch *search, DATA &data, UInt uiDepth        );
  
  Void  xCheckRDCostMerge2Nx2N( TEncSearch *search, DATA &data, Bool *earlyDetectionSkipMode);

#if AMP_MRG
  Void  xCheckRDCostInter   ( TEncSearch *search, DATA &data, PartSize ePartSize, Bool bUseMRG = false  );
#else
  Void  xCheckRDCostInter   ( TEncSearch *search, DATA &data, PartSize ePartSize  );
#endif
  Void  xCheckRDCostIntra   ( TEncSearch *search, DATA &data, PartSize ePartSize  );
  Void  xCheckDQP           ( TEncSearch *search, TComDataCU*  pcCU );
  
  Void  xCheckIntraPCM      ( TEncSearch *search, DATA &data                     );
  Void  xCopyAMVPInfo       ( AMVPInfo* pSrc, AMVPInfo* pDst );
  Void  xCopyYuv2Pic        ( TComPic* rpcPic, UInt uiCUAddr, UInt uiAbsPartIdx, UInt uiDepth, UInt uiSrcDepth, TComDataCU* pcCU, UInt uiLPelX, UInt uiTPelY, DATA &data );
  Void  xCopyYuv2Tmp        ( DATA &data, DATA &subData, UInt uhPartUnitIdx, UInt uiDepth );

  Bool getdQPFlag           ()                        { return m_bEncodeDQP;        }
  Void setdQPFlag           ( Bool b )                { m_bEncodeDQP = b;           }

#if ADAPTIVE_QP_SELECTION
  // Adaptive reconstruction level (ARL) statistics collection functions
  Void xLcuCollectARLStats(TComDataCU* rpcCU);
  Int  xTuCollectARLStats(TCoeff* rpcCoeff, Int* rpcArlCoeff, Int NumCoeffInCU, Double* cSum, UInt* numSamples );
#endif

#if AMP_ENC_SPEEDUP 
#if AMP_MRG
  Void deriveTestModeAMP (TComDataCU *&rpcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver, Bool &bTestMergeAMP_Hor, Bool &bTestMergeAMP_Ver);
#else
  Void deriveTestModeAMP (TComDataCU *&rpcBestCU, PartSize eParentPartSize, Bool &bTestAMP_Hor, Bool &bTestAMP_Ver);
#endif
#endif

  Void  xFillPCMBuffer     ( TComDataCU*& pCU, TComYuv* pOrgYuv ); 
};

//! \}

#endif // __TENCMB__
