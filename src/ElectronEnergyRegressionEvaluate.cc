#include "EGamma/EGammaAnalysisTools/interface/ElectronEnergyRegressionEvaluate.h"
#include <cmath>
#include <cassert>

#ifndef STANDALONE
#include "RecoEcal/EgammaCoreTools/interface/EcalClusterLazyTools.h"
#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"
#include "RecoEgamma/EgammaTools/interface/EcalClusterLocal.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/EcalDetId/interface/EBDetId.h"
#include "DataFormats/EcalDetId/interface/EEDetId.h"
#endif

ElectronEnergyRegressionEvaluate::ElectronEnergyRegressionEvaluate() : 
	fIsInitialized(kFALSE),
	fVersionType(kNoTrkVar),
	forest_eb(0), 
	forest_ee(0), 
	forest_lowPt_eb(0), 
	forest_lowPt_ee(0), 
	forest_highPt_eb(0), 
	forest_highPt_ee(0) {
	}

ElectronEnergyRegressionEvaluate::~ElectronEnergyRegressionEvaluate() {}
// Destructor does nothing


void ElectronEnergyRegressionEvaluate::initialize(std::string weightsFile, 
		ElectronEnergyRegressionEvaluate::ElectronEnergyRegressionType type) {

  // Loading forest object according to different versions
  TFile file(weightsFile.c_str());

  if (type == kNoTrkVar || type == kWithTrkVar) {
    forest_eb = (GBRForest*) file.Get("EBCorrection");
    forest_ee = (GBRForest*) file.Get("EECorrection");

    // Just checking
    assert(forest_eb);
    assert(forest_ee);
  }

  else if (type == kNoTrkVarTwoPtBins || type == kWithTrkVarTwoPtBins) {
    forest_lowPt_eb = (GBRForest*) file.Get("EBCorrection_lowPt");
    forest_lowPt_ee = (GBRForest*) file.Get("EECorrection_lowPt");
    forest_highPt_eb = (GBRForest*) file.Get("EBCorrection_highPt");
    forest_highPt_ee = (GBRForest*) file.Get("EECorrection_highPt");

    // Just checking
    assert(forest_lowPt_eb);
    assert(forest_lowPt_ee);
    assert(forest_highPt_eb);
    assert(forest_highPt_ee);
  }

  // Updating type and marking as initialized
  fVersionType = type;
  fIsInitialized = kTRUE;
}



#ifndef STANDALONE
double ElectronEnergyRegressionEvaluate::calculateRegressionEnergy(const reco::GsfElectron *ele, 
                                                                   EcalClusterLazyTools &myEcalCluster, 
                                                                   const edm::EventSetup &setup,
                                                                   double rho, double nvertices, 
                                                                   bool printDebug) {
  
  if (!fIsInitialized) {
    std::cout << "Error: Electron Energy Regression has not been initialized yet. return 0. \n";
    return 0;
  }

  std::vector<float> vCov = myEcalCluster.localCovariances(*(ele->superCluster()->seed()));
  double spp = 0;
  if (!isnan(vCov[2])) spp = sqrt (vCov[2]);
  double sep;
  if (ele->sigmaIetaIeta()*spp > 0) {
    sep = vCov[1] / ele->sigmaIetaIeta()*spp;
  } else if (vCov[1] > 0) {
    sep = 1.0;
  } else {
    sep = -1.0;
  }

  //local coordinates
  EcalClusterLocal local;  
  double ietaseed = 0;
  double iphiseed = 0;
  double etacryseed = 0;
  double phicryseed = 0;

  if (ele->superCluster()->seed()->hitsAndFractions().at(0).first.subdetId()==EcalBarrel) {
    float etacry, phicry, thetatilt, phitilt;
    int ieta, iphi;
    local.localCoordsEB(*ele->superCluster()->seed(),setup,etacry,phicry,ieta,iphi,thetatilt,phitilt);
    
    EBDetId edet(ele->superCluster()->seed()->hitsAndFractions().at(0).first);
    ietaseed = edet.ieta();
    iphiseed = edet.iphi();
    etacryseed = etacry;
    phicryseed = phicry;
  }
  
  if (printDebug) {
    std::cout << "Regression Type: " << fVersionType << std::endl;
    std::cout << "Electron : " << ele->pt() << " " << ele->eta() << " " << ele->phi() << "\n";
  }

  if (fVersionType == kNoTrkVar || fVersionType == kNoTrkVarTwoPtBins) {
    return regressionValueNoTrkVar(
      ele->p(),
      ele->superCluster()->rawEnergy(),
      ele->superCluster()->eta(),
      ele->superCluster()->phi(),
      myEcalCluster.e3x3(*ele->superCluster()->seed()) / ele->superCluster()->rawEnergy(),
      ele->superCluster()->etaWidth(),
      ele->superCluster()->phiWidth(),
      ele->superCluster()->clustersSize(),
      ele->hadronicOverEm(),
      rho,
      nvertices,
      ele->superCluster()->seed()->eta(),
      ele->superCluster()->seed()->phi(),
      ele->superCluster()->seed()->energy(),
      myEcalCluster.e3x3(*ele->superCluster()->seed()),
      myEcalCluster.e5x5(*ele->superCluster()->seed()),
      ele->sigmaIetaIeta(),
      spp,
      sep,
      myEcalCluster.eMax(*ele->superCluster()->seed()),
      myEcalCluster.e2nd(*ele->superCluster()->seed()),
      myEcalCluster.eTop(*ele->superCluster()->seed()),
      myEcalCluster.eBottom(*ele->superCluster()->seed()),
      myEcalCluster.eLeft(*ele->superCluster()->seed()),
      myEcalCluster.eRight(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Max(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Top(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Bottom(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Left(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Right(*ele->superCluster()->seed()),
      ele->pt(),
      ietaseed,
      iphiseed,
      etacryseed,
      phicryseed,
      ele->superCluster()->preshowerEnergy(),
      printDebug
      );
  } else if (fVersionType == kWithTrkVar || fVersionType == kWithTrkVarTwoPtBins) {
    return regressionValueWithTrkVar(
      ele->p(),
      ele->superCluster()->rawEnergy(),
      ele->superCluster()->eta(),
      ele->superCluster()->phi(),
      myEcalCluster.e3x3(*ele->superCluster()->seed()) / ele->superCluster()->rawEnergy(),
      ele->superCluster()->etaWidth(),
      ele->superCluster()->phiWidth(),
      ele->superCluster()->clustersSize(),
      ele->hadronicOverEm(),
      rho,
      nvertices,
      ele->superCluster()->seed()->eta(),
      ele->superCluster()->seed()->phi(),
      ele->superCluster()->seed()->energy(),
      myEcalCluster.e3x3(*ele->superCluster()->seed()),
      myEcalCluster.e5x5(*ele->superCluster()->seed()),
      ele->sigmaIetaIeta(),
      spp,
      sep,
      myEcalCluster.eMax(*ele->superCluster()->seed()),
      myEcalCluster.e2nd(*ele->superCluster()->seed()),
      myEcalCluster.eTop(*ele->superCluster()->seed()),
      myEcalCluster.eBottom(*ele->superCluster()->seed()),
      myEcalCluster.eLeft(*ele->superCluster()->seed()),
      myEcalCluster.eRight(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Max(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Top(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Bottom(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Left(*ele->superCluster()->seed()),
      myEcalCluster.e2x5Right(*ele->superCluster()->seed()),
      ele->pt(),      
      ele->trackMomentumAtVtx().R(),
      ele->fbrem(),
      ele->charge(),
      ele->eSuperClusterOverP(),
      ietaseed,
      iphiseed,
      etacryseed,
      phicryseed,
      ele->superCluster()->preshowerEnergy(),
      printDebug
      );
  } else {
    std::cout << "Warning: Electron Regression Type " << fVersionType << " is not supported. Reverting to default electron momentum.\n"; 
    return ele->p();
  }


}
#endif


double ElectronEnergyRegressionEvaluate::regressionValueNoTrkVar(
                double electronP, 
		double SCRawEnergy,
		double scEta,
		double scPhi,
		double R9,
		double etawidth,
		double phiwidth,
		double NClusters,
		double HoE,
		double rho,
		double vertices,
		double EtaSeed,
		double PhiSeed,
		double ESeed,
		double E3x3Seed,
		double E5x5Seed,
		double see,
		double spp,
		double sep,
		double EMaxSeed,
		double E2ndSeed,
		double ETopSeed,
		double EBottomSeed,
		double ELeftSeed,
		double ERightSeed,
		double E2x5MaxSeed,
		double E2x5TopSeed,
		double E2x5BottomSeed,
		double E2x5LeftSeed,
		double E2x5RightSeed,
		double pt,
		double IEtaSeed,
		double IPhiSeed,
		double EtaCrySeed,
		double PhiCrySeed,
		double PreShowerOverRaw, 
                bool printDebug) 
{
  // Checking if instance has been initialized
  if (fIsInitialized == kFALSE) {
    printf("ElectronEnergyRegressionEvaluate instance not initialized !!!");
    return 0;
  }

  // Checking if type is correct
  assert(fVersionType == kNoTrkVar || fVersionType == kNoTrkVarTwoPtBins);

  // Now applying regression according to version and (endcap/barrel)
  float *vals = (scEta <= 1.479) ? new float[39] : new float[32];
  if (scEta <= 1.479) {		// Barrel
    vals[0]  = SCRawEnergy;
    vals[1]  = scEta;
    vals[2]  = scPhi;
    vals[3]  = R9;
    vals[4]  = E5x5Seed/SCRawEnergy;
    vals[5]  = etawidth;
    vals[6]  = phiwidth;
    vals[7]  = NClusters;
    vals[8]  = HoE;
    vals[9]  = rho;
    vals[10] = vertices;
    vals[11] = EtaSeed - scEta;
    vals[12] = atan2(sin(PhiSeed-scPhi),cos(PhiSeed-scPhi));
    vals[13] = ESeed/SCRawEnergy;
    vals[14] = E3x3Seed/ESeed;
    vals[15] = E5x5Seed/ESeed;
    vals[16] = see;
    vals[17] = spp;
    vals[18] = sep;
    vals[19] = EMaxSeed/ESeed;
    vals[20] = E2ndSeed/ESeed;
    vals[21] = ETopSeed/ESeed;
    vals[22] = EBottomSeed/ESeed;
    vals[23] = ELeftSeed/ESeed;
    vals[24] = ERightSeed/ESeed;
    vals[25] = E2x5MaxSeed/ESeed;
    vals[26] = E2x5TopSeed/ESeed;
    vals[27] = E2x5BottomSeed/ESeed;
    vals[28] = E2x5LeftSeed/ESeed;
    vals[29] = E2x5RightSeed/ESeed;
    vals[30] = pt;
    vals[31] = IEtaSeed;
    vals[32] = IPhiSeed;
    vals[33] = ((int) IEtaSeed)%5;
    vals[34] = ((int) IPhiSeed)%2;
    vals[35] = (abs(IEtaSeed)<=25)*(((int)IEtaSeed)%25) + (abs(IEtaSeed)>25)*(((int) (IEtaSeed-25*abs(IEtaSeed)/IEtaSeed))%20);
    vals[36] = ((int) IPhiSeed)%20;
    vals[37] = EtaCrySeed;
    vals[38] = PhiCrySeed;
  }

  else if (scEta > 1.479) {	// Endcap
    vals[0]  = SCRawEnergy;
    vals[1]  = scEta;
    vals[2]  = scPhi;
    vals[3]  = R9;
    vals[4]  = E5x5Seed/SCRawEnergy;
    vals[5]  = etawidth;
    vals[6]  = phiwidth;
    vals[7]  = NClusters;
    vals[8]  = HoE;
    vals[9]  = rho;
    vals[10] = vertices;
    vals[11] = EtaSeed - scEta;
    vals[12] = atan2(sin(PhiSeed-scPhi),cos(PhiSeed-scPhi));
    vals[13] = ESeed/SCRawEnergy;
    vals[14] = E3x3Seed/ESeed;
    vals[15] = E5x5Seed/ESeed;
    vals[16] = see;
    vals[17] = spp;
    vals[18] = sep;
    vals[19] = EMaxSeed/ESeed;
    vals[20] = E2ndSeed/ESeed;
    vals[21] = ETopSeed/ESeed;
    vals[22] = EBottomSeed/ESeed;
    vals[23] = ELeftSeed/ESeed;
    vals[24] = ERightSeed/ESeed;
    vals[25] = E2x5MaxSeed/ESeed;
    vals[26] = E2x5TopSeed/ESeed;
    vals[27] = E2x5BottomSeed/ESeed;
    vals[28] = E2x5LeftSeed/ESeed;
    vals[29] = E2x5RightSeed/ESeed;
    vals[30] = pt;
    vals[31] = PreShowerOverRaw;
  }

  // Now evaluating the regression
  double regressionResult = 0;
  Int_t BinIndex = -1;

  if (fVersionType == kNoTrkVar) {
    if (scEta <= 1.479) { 
      regressionResult = SCRawEnergy * forest_eb->GetResponse(vals); 
      BinIndex = 0;
    }
    else if (scEta > 1.479) {
      regressionResult = (SCRawEnergy*(1+PreShowerOverRaw)) * forest_ee->GetResponse(vals);
      BinIndex = 1;
    }
  }

  else if (fVersionType == kNoTrkVarTwoPtBins) {
    if (scEta <= 1.479) {
      if (pt <= 15) {
        regressionResult = electronP * forest_lowPt_eb->GetResponse(vals);
        BinIndex = 0;
      }
      else {
        regressionResult = SCRawEnergy * forest_highPt_eb->GetResponse(vals);
        BinIndex = 1;
      }
    }
    else {
      if (pt <= 15) {
        regressionResult = electronP * forest_lowPt_ee->GetResponse(vals);
        BinIndex = 2;
      }
      else {
        regressionResult = (SCRawEnergy*(1+PreShowerOverRaw)) * forest_highPt_ee->GetResponse(vals);
        BinIndex = 3;
      }
    }
  }
  
  //print debug
  if (printDebug) {    
    if (scEta <= 1.479) {
      std::cout << "Barrel :";
      for (uint v=0; v < 39; ++v) std::cout << vals[v] << ", ";
      std::cout << "\n";
    }
    else {
      std::cout << "Endcap :";
      for (uint v=0; v < 32; ++v) std::cout << vals[v] << ", ";
      std::cout << "\n";
    }
    std::cout << "BinIndex : " << BinIndex << "\n";
    std::cout << "pt = " << pt << " : SCRawEnergy = " << SCRawEnergy << " : PreShowerOverRaw = " << PreShowerOverRaw << std::endl;
    std::cout << "regression energy = " << regressionResult << std::endl;
  }
  

  // Cleaning up and returning
  delete[] vals;
  return regressionResult;
}

double ElectronEnergyRegressionEvaluate::regressionValueWithTrkVar(
                double electronP, 
		double SCRawEnergy,                
		double scEta,
		double scPhi,
		double R9,
		double etawidth,
		double phiwidth,
		double NClusters,
		double HoE,
		double rho,
		double vertices,
		double EtaSeed,
		double PhiSeed,
		double ESeed,
		double E3x3Seed,
		double E5x5Seed,
		double see,
		double spp,
		double sep,
		double EMaxSeed,
		double E2ndSeed,
		double ETopSeed,
		double EBottomSeed,
		double ELeftSeed,
		double ERightSeed,
		double E2x5MaxSeed,
		double E2x5TopSeed,
		double E2x5BottomSeed,
		double E2x5LeftSeed,
		double E2x5RightSeed,
		double pt,
		double GsfTrackPIn,
		double fbrem,
		double Charge,
		double EoP,
		double IEtaSeed,
		double IPhiSeed,
		double EtaCrySeed,
		double PhiCrySeed,
		double PreShowerOverRaw, 
                bool printDebug) 
{
  // Checking if instance has been initialized
  if (fIsInitialized == kFALSE) {
    printf("ElectronEnergyRegressionEvaluate instance not initialized !!!");
    return 0;
  }

  // Checking if fVersionType is correct
  assert(fVersionType == kWithTrkVar || fVersionType == kWithTrkVarTwoPtBins);

  float *vals = (scEta <= 1.479) ? new float[43] : new float[36];
  if (scEta <= 1.479) {		// Barrel
    vals[0]  = SCRawEnergy;
    vals[1]  = scEta;
    vals[2]  = scPhi;
    vals[3]  = R9;
    vals[4]  = E5x5Seed/SCRawEnergy;
    vals[5]  = etawidth;
    vals[6]  = phiwidth;
    vals[7]  = NClusters;
    vals[8]  = HoE;
    vals[9]  = rho;
    vals[10] = vertices;
    vals[11] = EtaSeed - scEta;
    vals[12] = atan2(sin(PhiSeed-scPhi),cos(PhiSeed-scPhi));
    vals[13] = ESeed/SCRawEnergy;
    vals[14] = E3x3Seed/ESeed;
    vals[15] = E5x5Seed/ESeed;
    vals[16] = see;
    vals[17] = spp;
    vals[18] = sep;
    vals[19] = EMaxSeed/ESeed;
    vals[20] = E2ndSeed/ESeed;
    vals[21] = ETopSeed/ESeed;
    vals[22] = EBottomSeed/ESeed;
    vals[23] = ELeftSeed/ESeed;
    vals[24] = ERightSeed/ESeed;
    vals[25] = E2x5MaxSeed/ESeed;
    vals[26] = E2x5TopSeed/ESeed;
    vals[27] = E2x5BottomSeed/ESeed;
    vals[28] = E2x5LeftSeed/ESeed;
    vals[29] = E2x5RightSeed/ESeed;
    vals[30] = pt;
    vals[31] = GsfTrackPIn;
    vals[32] = fbrem;
    vals[33] = Charge;
    vals[34] = EoP;
    vals[35] = IEtaSeed;
    vals[36] = IPhiSeed;
    vals[37] = ((int) IEtaSeed)%5;
    vals[38] = ((int) IPhiSeed)%2;
    vals[39] = (abs(IEtaSeed)<=25)*(((int)IEtaSeed)%25) + (abs(IEtaSeed)>25)*(((int) (IEtaSeed-25*abs(IEtaSeed)/IEtaSeed))%20);
    vals[40] = ((int) IPhiSeed)%20;
    vals[41] = EtaCrySeed;
    vals[42] = PhiCrySeed;
  }

  else if (scEta > 1.479) {	// Endcap
    vals[0]  = SCRawEnergy;
    vals[1]  = scEta;
    vals[2]  = scPhi;
    vals[3]  = R9;
    vals[4]  = E5x5Seed/SCRawEnergy;
    vals[5]  = etawidth;
    vals[6]  = phiwidth;
    vals[7]  = NClusters;
    vals[8]  = HoE;
    vals[9]  = rho;
    vals[10] = vertices;
    vals[11] = EtaSeed - scEta;
    vals[12] = atan2(sin(PhiSeed-scPhi),cos(PhiSeed-scPhi));
    vals[13] = ESeed/SCRawEnergy;
    vals[14] = E3x3Seed/ESeed;
    vals[15] = E5x5Seed/ESeed;
    vals[16] = see;
    vals[17] = spp;
    vals[18] = sep;
    vals[19] = EMaxSeed/ESeed;
    vals[20] = E2ndSeed/ESeed;
    vals[21] = ETopSeed/ESeed;
    vals[22] = EBottomSeed/ESeed;
    vals[23] = ELeftSeed/ESeed;
    vals[24] = ERightSeed/ESeed;
    vals[25] = E2x5MaxSeed/ESeed;
    vals[26] = E2x5TopSeed/ESeed;
    vals[27] = E2x5BottomSeed/ESeed;
    vals[28] = E2x5LeftSeed/ESeed;
    vals[29] = E2x5RightSeed/ESeed;
    vals[30] = pt;
    vals[31] = GsfTrackPIn;
    vals[32] = fbrem;
    vals[33] = Charge;
    vals[34] = EoP;
    vals[35] = PreShowerOverRaw;
  }

  // Now evaluating the regression
  double regressionResult = 0;

  if (fVersionType == kWithTrkVar) {
    if (scEta <= 1.479) regressionResult = SCRawEnergy * forest_eb->GetResponse(vals);
    else if (scEta > 1.479) regressionResult = (SCRawEnergy*(1+PreShowerOverRaw)) * forest_ee->GetResponse(vals);
  }

  else if (fVersionType == kWithTrkVarTwoPtBins) {
    if (scEta <= 1.479) {
      if (pt <= 15) regressionResult = electronP * forest_lowPt_eb->GetResponse(vals);
      if (pt > 15) regressionResult = SCRawEnergy * forest_highPt_eb->GetResponse(vals);
    }
    if (scEta > 1.479) {
      if (pt <= 15) regressionResult = electronP * forest_lowPt_ee->GetResponse(vals);
      if (pt > 15) regressionResult = (SCRawEnergy*(1+PreShowerOverRaw)) * forest_highPt_ee->GetResponse(vals);
    }
  }

  //print debug
  if (printDebug) {
    if (scEta <= 1.479) {
      std::cout << "Barrel :";
      for (uint v=0; v < 43; ++v) std::cout << vals[v] << ", ";
      std::cout << "\n";
    }
    else {
      std::cout << "Endcap :";
      for (uint v=0; v < 36; ++v) std::cout << vals[v] << ", ";
      std::cout << "\n";
    }
    std::cout << "pt = " << pt << " : SCRawEnergy = " << SCRawEnergy << " : PreShowerOverRaw = " << PreShowerOverRaw << std::endl;
    std::cout << "regression energy = " << regressionResult << std::endl;
  }


  // Cleaning up and returning
  delete[] vals;
  return regressionResult;
}