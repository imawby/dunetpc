#ifndef PANDIZZLEALG_H_SEEN
#define PANDIZZLEALG_H_SEEN

///////////////////////////////////////////////
// PandizzleAlg.h
//
///////////////////////////////////////////////

// framework
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "canvas/Persistency/Common/Ptr.h" 
#include "canvas/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "art/Framework/Services/Optional/TFileService.h" 
#include "art/Framework/Services/Optional/TFileDirectory.h" 
#include "messagefacility/MessageLogger/MessageLogger.h" 
#include "canvas/Persistency/Common/FindManyP.h"

// LArSoft
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/AnalysisBase/MVAPIDResult.h"
#include "lardataobj/AnalysisBase/ParticleID.h"
#include "larsim/MCCheater/BackTrackerService.h"
#include "larsim/MCCheater/ParticleInventoryService.h"
#include "larpandora/LArPandoraInterface/LArPandoraHelper.h"
#include "larreco/RecoAlg/ShowerEnergyAlg.h"

// c++
#include <vector>
#include <map>

// ROOT
#include "TTree.h"

//Custom
#include "FDSelectionUtils.h"

//constexpr int kMaxObjects = 999;

namespace FDSelection {
  class PandizzleAlg;
}

class FDSelection::PandizzleAlg {
 public:

  PandizzleAlg(const fhicl::ParameterSet& pset);

  void Run(const art::Event& evt);

 private:

  /// Initialise the tree
  void InitialiseTrees();

  std::vector<art::Ptr<recob::PFParticle> > SelectChildPFParticles(const art::Ptr<recob::PFParticle> parent_pfp, const lar_pandora::PFParticleMap & pfp_map);

  void ProcessPFParticle(const art::Ptr<recob::PFParticle> pfp, const art::Event& evt);
  int CountPFPWithPDG(const std::vector<art::Ptr<recob::PFParticle> > & pfps, int pdg);

  std::vector<art::Ptr<recob::Hit> > GetPFPHits(const art::Ptr<recob::PFParticle> pfp, const art::Event& evt);

  void ResetTreeVariables();

  void BookTreeInt(TTree *tree, std::string branch_name);

  void BookTreeFloat(TTree *tree, std::string branch_name);

  void FillTree();

  void FillMichelElectronVariables(const art::Ptr<recob::PFParticle> mu_pfp, const std::vector<art::Ptr<recob::PFParticle> > & child_pfps, const art::Event& evt);

  void FillTrackVariables(const art::Ptr<recob::PFParticle> pfp, const art::Event& evt);
  void CalculateTrackDeflection(const art::Ptr<recob::Track> track);

  void CalculateTrackLengthRatio(const art::Ptr<recob::Track> track, const art::Event& evt);


  // module labels
  std::string fTrackModuleLabel;
  std::string fShowerModuleLabel;
  std::string fPIDModuleLabel;
  std::string fPFParticleModuleLabel;
  std::string fSpacePointModuleLabel;

  // tree
  TTree* fSignalTrackTree;
  TTree* fSignalShowerTree;
  TTree *fBackgroundTrackTree;
  TTree *fBackgroundShowerTree;

  //Algs
  shower::ShowerEnergyAlg fShowerEnergyAlg;

  struct VarHolder{ //This thing holds all variables to be handed to the trees
    std::map<std::string, int> IntVars;
    std::map<std::string, float> FloatVars;
  };

  VarHolder fVarHolder;
  /*
  int fRun;
  int fSubRun;
  int fEvent;
  int fPFPPDG;
  int fPFPNChildren;
  int fPFPNShowerChildren;
  int fPFPNTrackChildren;
  int fPFPNHits;
  */

  // services
  art::ServiceHandle<cheat::BackTrackerService> bt_serv;
  art::ServiceHandle<cheat::ParticleInventoryService> pi_serv;
  art::ServiceHandle<art::TFileService> tfs;

};

#endif
