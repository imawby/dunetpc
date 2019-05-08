///////////////////////////////////////////////
// PandizzleAlg.cxx
//
// Reco and true PID stuff up
// D Brailsford & M Wallbank, June 2017
///////////////////////////////////////////////

#include "PandizzleAlg.h"

FDSelection::PandizzleAlg::PandizzleAlg(const fhicl::ParameterSet& pset) :
  fShowerEnergyAlg(pset.get<fhicl::ParameterSet>("ShowerEnergyAlg"))
{
  fTrackModuleLabel  = pset.get<std::string>("ModuleLabels.TrackModuleLabel");
  fShowerModuleLabel = pset.get<std::string>("ModuleLabels.ShowerModuleLabel");
  fPIDModuleLabel    = pset.get<std::string>("ModuleLabels.PIDModuleLabel");
  fPFParticleModuleLabel = pset.get<std::string>("ModuleLabels.PFParticleModuleLabel");
  fSpacePointModuleLabel = pset.get<std::string>("ModuleLabels.SpacePointModuleLabel");
  InitialiseTrees();
  ResetTreeVariables();
}

void FDSelection::PandizzleAlg::InitialiseTrees() {
  fSignalTrackTree = tfs->make<TTree>("DizzleSigTrackTree","Pandizzle Signal Track Tree");
  fBackgroundTrackTree = tfs->make<TTree>("DizzleBgTrackTree","Pandizzle Background Track Tree");
  fSignalShowerTree = tfs->make<TTree>("DizzleSigShowerTree","Pandizzle Signal Shower Tree");
  fBackgroundShowerTree = tfs->make<TTree>("DizzleBgShowerTree","Pandizzle Background Shower Tree");

  //I am lazy.  Sue me.
  std::map<std::string, TTree*> treeMap;
  treeMap["signalTrack"] = fSignalTrackTree;
  treeMap["backgroundTrack"] = fBackgroundTrackTree;
  treeMap["signalShower"] = fSignalShowerTree;
  treeMap["backgroundShower"] = fBackgroundShowerTree;
  for (std::map<std::string, TTree*>::iterator mapIt = treeMap.begin(); mapIt != treeMap.end(); mapIt++){
    TTree *tree = mapIt->second;
    BookTreeInt(tree, "Event");
    BookTreeInt(tree, "Run");
    BookTreeInt(tree, "SubRun");
    BookTreeInt(tree, "PFPPDG");
    BookTreeInt(tree, "PFPNHits");
    BookTreeInt(tree, "PFPTrueID");
    BookTreeInt(tree, "PFPTruePDG");
    BookTreeInt(tree, "PFPTrueMotherID");
    BookTreeFloat(tree, "PFPTrueMomT");
    BookTreeFloat(tree, "PFPTrueMomX");
    BookTreeFloat(tree, "PFPTrueMomY");
    BookTreeFloat(tree, "PFPTrueMomZ");
    BookTreeInt(tree, "PFPNChildren");
    BookTreeInt(tree, "PFPNShowerChildren");
    BookTreeInt(tree, "PFPNTrackChildren");
    BookTreeFloat(tree, "PFPMichelDist");
    BookTreeInt(tree, "PFPMichelNHits");
    BookTreeInt(tree, "PFPMichelTrueID");
    BookTreeInt(tree, "PFPMichelTrueMotherID");
    BookTreeInt(tree, "PFPMichelTruePDG");
    BookTreeFloat(tree, "PFPMichelElectronMVA");
    BookTreeFloat(tree, "PFPMichelRecoEnergyPlane0");
    BookTreeFloat(tree, "PFPMichelRecoEnergyPlane1");
    BookTreeFloat(tree, "PFPMichelRecoEnergyPlane2");
    BookTreeFloat(tree,"PFPTrackDeflecAngleMean");
    BookTreeFloat(tree,"PFPTrackDeflecAngleSD");
    BookTreeFloat(tree,"PFPTrackMuonMVA");
    BookTreeFloat(tree,"PFPTrackProtonMVA");
    BookTreeFloat(tree,"PFPTrackPionMVA");
    BookTreeFloat(tree,"PFPTrackPhotonMVA");
    BookTreeFloat(tree,"PFPTrackElectronMVA");
    BookTreeFloat(tree,"PFPTrackLengthRatio");
  }
}

void FDSelection::PandizzleAlg::Run(const art::Event& evt) {

  //Get the PFPs out of the event
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  if (!(evt.getByLabel(fPFParticleModuleLabel, pfparticleListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::PFParticle> with module label: " << fPFParticleModuleLabel;
    return;
  }
  std::vector<art::Ptr<recob::PFParticle> > pfparticleList;
  art::fill_ptr_vector(pfparticleList, pfparticleListHandle);

  //Ceate the full PFP map
  lar_pandora::PFParticleMap pfparticleMap;
  lar_pandora::LArPandoraHelper::BuildPFParticleMap(pfparticleList, pfparticleMap);

  //Grab the primary PFPs (the neutrinos) from the 
  std::vector<art::Ptr<recob::PFParticle> > nu_pfps;
  lar_pandora::LArPandoraHelper::SelectNeutrinoPFParticles(pfparticleList, nu_pfps);

  //Now grab the primary children of these PFP
  for (unsigned int i_nupfp = 0; i_nupfp < nu_pfps.size(); i_nupfp++){
    art::Ptr<recob::PFParticle> nu_pfp = nu_pfps[i_nupfp];
    //std::cout<<"Nu ID: " << nu_pfp->Self() << std::endl;
    std::vector<art::Ptr<recob::PFParticle> > child_pfps = SelectChildPFParticles(nu_pfp, pfparticleMap);
    //Assess each child pfp
    for (unsigned int i_childpfp = 0; i_childpfp < child_pfps.size(); i_childpfp++){
      art::Ptr<recob::PFParticle> child_pfp = child_pfps[i_childpfp];
      //Process the child PFP
      ProcessPFParticle(child_pfp, evt);
    }
  }
  //mf::LogWarning("PandizzleAlg") << "No tracks";

  return;

}

std::vector<art::Ptr<recob::PFParticle> > FDSelection::PandizzleAlg::SelectChildPFParticles(const art::Ptr<recob::PFParticle> parent_pfp, const lar_pandora::PFParticleMap & pfp_map){
  std::vector<art::Ptr<recob::PFParticle> > child_pfps;
  for (int i_child = 0; i_child < parent_pfp->NumDaughters(); i_child++){
    int child_id = parent_pfp->Daughter(i_child);
    art::Ptr<recob::PFParticle> child_pfp = pfp_map.at(child_id);
    child_pfps.push_back(child_pfp);
  }
  return child_pfps;
}

void FDSelection::PandizzleAlg::ProcessPFParticle(const art::Ptr<recob::PFParticle> pfp, const art::Event& evt){
  //Get event,subrun,run
  fVarHolder.IntVars["Run"] = evt.run();
  fVarHolder.IntVars["SubRun"] = evt.subRun();
  fVarHolder.IntVars["Event"] = evt.event();

  //Get the PFPs out of the event
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  if (!(evt.getByLabel(fPFParticleModuleLabel, pfparticleListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::PFParticle> with module label: " << fPFParticleModuleLabel;
    return;
  }
  std::vector<art::Ptr<recob::PFParticle> > pfparticleList;
  art::fill_ptr_vector(pfparticleList, pfparticleListHandle);

  //Ceate the full PFP map
  lar_pandora::PFParticleMap pfparticleMap;
  lar_pandora::LArPandoraHelper::BuildPFParticleMap(pfparticleList, pfparticleMap);

  //PDG
  fVarHolder.IntVars["PFPPDG"] = pfp->PdgCode();


  //Get the PFP hits
  std::vector<art::Ptr<recob::Hit> > pfp_hits = GetPFPHits(pfp, evt);
  fVarHolder.IntVars["PFPNHits"] = pfp_hits.size();
  //if (pfp_hits.size() == 0) std::cout<<"PDG: " << pfp->PdgCode() << "  NHits: " << pfp_hits.size() << "  ID: " << pfp->Self() << "  NDau" << pfp->NumDaughters() << "  Parent " << pfp->Parent()  << std::endl;

  //Get the true PDG
  int g4id = FDSelectionUtils::TrueParticleIDFromTotalRecoHits(pfp_hits); 
  if (g4id > 0){
    art::ServiceHandle<cheat::ParticleInventoryService> pi_serv;
    const simb::MCParticle* matched_mcparticle = pi_serv->ParticleList().at(g4id);

    if (matched_mcparticle){
      fVarHolder.IntVars["PFPTrueID"] = g4id;
      fVarHolder.IntVars["PFPTruePDG"] = matched_mcparticle->PdgCode();
      fVarHolder.IntVars["PFPTrueMotherID"] = matched_mcparticle->Mother();
      fVarHolder.FloatVars["PFPTrueMomX"] = matched_mcparticle->Momentum(0).X();
      fVarHolder.FloatVars["PFPTrueMomY"] = matched_mcparticle->Momentum(0).Y();
      fVarHolder.FloatVars["PFPTrueMomZ"] = matched_mcparticle->Momentum(0).Z();
      fVarHolder.FloatVars["PFPTrueMomT"] = matched_mcparticle->Momentum(0).T();
    }
  }

  //Count all of the children
  std::vector<art::Ptr<recob::PFParticle> > child_pfps = SelectChildPFParticles(pfp, pfparticleMap);
  fVarHolder.IntVars["PFPNChildren"] = pfp->NumDaughters();
  fVarHolder.IntVars["PFPNShowerChildren"] = CountPFPWithPDG(child_pfps, 11);
  fVarHolder.IntVars["PFPNTrackChildren"] = CountPFPWithPDG(child_pfps, 13);
  
  FillMichelElectronVariables(pfp, child_pfps, evt);
  FillTrackVariables(pfp, evt);


  FillTree();
  ResetTreeVariables();
  return;
}

void FDSelection::PandizzleAlg::ResetTreeVariables(){
  for (std::map<std::string, int>::iterator mapIt = fVarHolder.IntVars.begin(); mapIt != fVarHolder.IntVars.end(); mapIt++){
    mapIt->second = -9999;
  }
  for (std::map<std::string, float>::iterator mapIt = fVarHolder.FloatVars.begin(); mapIt != fVarHolder.FloatVars.end(); mapIt++){
    mapIt->second = -9999.;
  }
  return;
}

int FDSelection::PandizzleAlg::CountPFPWithPDG(const std::vector<art::Ptr<recob::PFParticle> > & pfps, int pdg){
  int NPFP = 0;
  for (unsigned int i_pfp = 0; i_pfp < pfps.size(); i_pfp++){
    int pfp_pdg = pfps[i_pfp]->PdgCode();
    if (pfp_pdg == pdg) NPFP++;
  }
  return NPFP;
}

std::vector<art::Ptr<recob::Hit> > FDSelection::PandizzleAlg::GetPFPHits(const art::Ptr<recob::PFParticle> pfp, const art::Event& evt){
  std::vector<art::Ptr<recob::Hit> > pfp_hits;
  //Get the PFP handle out of the event
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  if (!(evt.getByLabel(fPFParticleModuleLabel, pfparticleListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::PFParticle> with module label: " << fPFParticleModuleLabel;
    return pfp_hits; //empty
  }
  //Get the spacepoint handle out of the event
  art::Handle< std::vector<recob::SpacePoint> > spacePointListHandle;
  if (!(evt.getByLabel(fSpacePointModuleLabel, spacePointListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::SpacePoint> with module label: " << fSpacePointModuleLabel;
    return pfp_hits; //empty
  }

  art::FindManyP<recob::SpacePoint> fmsppfp(pfparticleListHandle, evt, fPFParticleModuleLabel);
  const std::vector<art::Ptr<recob::SpacePoint> > sel_pfp_spacepoints = fmsppfp.at(pfp.key());
  //std::cout<<"NSpacePoints: " << sel_pfp_spacepoints.size() << std::endl;
  art::FindManyP<recob::Hit> fmhsp(spacePointListHandle, evt, fSpacePointModuleLabel);
  //Loop over the space points and retrieve the hits
  for (unsigned i_sp = 0; i_sp < sel_pfp_spacepoints.size(); i_sp++){
    art::Ptr<recob::SpacePoint> sp = sel_pfp_spacepoints[i_sp];
    const std::vector<art::Ptr<recob::Hit> > sel_pfp_hits = fmhsp.at(sp.key());
    for (unsigned i_hit = 0; i_hit < sel_pfp_hits.size(); i_hit++){
      pfp_hits.push_back(sel_pfp_hits[i_hit]);
    }
  }
  return pfp_hits;
}

void FDSelection::PandizzleAlg::BookTreeInt(TTree *tree, std::string branch_name){
  fVarHolder.IntVars[branch_name] = -9999;
  tree->Branch(branch_name.c_str(), &(fVarHolder.IntVars[branch_name]));
  return;
}

void FDSelection::PandizzleAlg::BookTreeFloat(TTree *tree, std::string branch_name){
  fVarHolder.FloatVars[branch_name] = -9999.;
  tree->Branch(branch_name.c_str(), &(fVarHolder.FloatVars[branch_name]));
  return;
}
void FDSelection::PandizzleAlg::FillTree(){
  if (fVarHolder.IntVars["PFPTruePDG"] == 13){ //signal
    if (fVarHolder.IntVars["PFPPDG"] == 13){ //track
      fSignalTrackTree->Fill();
    }
    else if (fVarHolder.IntVars["PFPPDG"] == 11){ //shower
      fSignalShowerTree->Fill();
    }
    else{ //Don't know
      mf::LogWarning("PandizzleAlg") << "Unknown PFP PDG when filling tree"<< fVarHolder.IntVars["PFPPDG"];
    }
  }
  else { //background
    if (fVarHolder.IntVars["PFPPDG"] == 13){ //track
      fBackgroundTrackTree->Fill();
    }
    else if (fVarHolder.IntVars["PFPPDG"] == 11){ //shower
      fBackgroundShowerTree->Fill();
    }
    else{ //Don't know
      mf::LogWarning("PandizzleAlg") << "Unknown PFP PDG when filling tree"<< fVarHolder.IntVars["PFPPDG"];

    }
  }
  return;
}


void FDSelection::PandizzleAlg::FillMichelElectronVariables(const art::Ptr<recob::PFParticle> mu_pfp, const std::vector<art::Ptr<recob::PFParticle> > & child_pfps, const art::Event& evt){
  //Find closest PFP to end of mu
  //If we don't have a track, sack everything off
  if (mu_pfp->PdgCode() != 13){
    return;
  }
  //Assign the branch value to a new default value to indicate that we have a track, but don't necessarily have a Michel candidate (rather than a global default value)
  fVarHolder.FloatVars["PFPMichelDist"] = -100.;
  fVarHolder.IntVars["PFPMichelNHits"] = -100;
  fVarHolder.FloatVars["PFPMichelElectronMVA"] = -100.;
  fVarHolder.FloatVars["PFPMichelRecoEnergyPlane0"] = -100.;
  fVarHolder.FloatVars["PFPMichelRecoEnergyPlane1"] = -100.;
  fVarHolder.FloatVars["PFPMichelRecoEnergyPlane2"] = -100.;

  //If no child PFPs, sack everything off
  if (child_pfps.size() == 0){
    return;
  }
  //Get the PFP handle
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  if (!(evt.getByLabel(fPFParticleModuleLabel, pfparticleListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::PFParticle> with module label: " << fPFParticleModuleLabel;
    return;
  }

  //Get the shower handle
  art::Handle< std::vector<recob::Shower> > showerListHandle;
  if (!(evt.getByLabel(fShowerModuleLabel, showerListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::Shower> with module label: " << fShowerModuleLabel;
    return;
  }

  //Get the track handle
  art::Handle< std::vector<recob::Track> > trackListHandle;
  if (!(evt.getByLabel(fTrackModuleLabel, trackListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::Track> with module label: " << fTrackModuleLabel;
    return;
  }



  art::FindManyP<recob::Track> fmtpfp(pfparticleListHandle, evt, fTrackModuleLabel);
  art::FindManyP<recob::Shower> fmspfp(pfparticleListHandle, evt, fShowerModuleLabel);
  art::FindManyP<anab::MVAPIDResult> fmpidt(trackListHandle, evt, fPIDModuleLabel);
  art::FindManyP<anab::MVAPIDResult> fmpids(showerListHandle, evt, fPIDModuleLabel);

  const std::vector<art::Ptr<recob::Track> > sel_pfp_tracks = fmtpfp.at(mu_pfp.key());
  if (sel_pfp_tracks.size() != 1){
    return;
  }
  art::Ptr<recob::Track> mu_track = sel_pfp_tracks[0];
  TVector3 mu_end_position = mu_track->End();

  art::Ptr<recob::PFParticle> closest_child_pfp;
  art::Ptr<anab::MVAPIDResult> closest_child_pfp_mva_pid_result;
  double closest_distance = 99999999999;
  for (unsigned int i_child = 0; i_child < child_pfps.size(); i_child++){
    art::Ptr<recob::PFParticle> child_pfp = child_pfps[i_child];
    art::Ptr<anab::MVAPIDResult> child_pfp_mva_pid_result;
    TVector3 child_start_pos;
    if (child_pfp->PdgCode()==13){
      const std::vector<art::Ptr<recob::Track> > child_pfp_tracks = fmtpfp.at(child_pfp.key());
      if (child_pfp_tracks.size() != 1) return;
      art::Ptr<recob::Track> child_track = child_pfp_tracks[0];
      child_start_pos.SetXYZ(child_track->Start().X(),child_track->Start().Y(),child_track->Start().Z());
    std::vector<art::Ptr<anab::MVAPIDResult> > pids = fmpidt.at(child_track.key());
    child_pfp_mva_pid_result = pids.at(0);

    }
    else if (child_pfp->PdgCode()==11){
      const std::vector<art::Ptr<recob::Shower> > child_pfp_showers = fmspfp.at(child_pfp.key());
      if (child_pfp_showers.size() != 1) return;
      art::Ptr<recob::Shower> child_shower = child_pfp_showers[0];
      child_start_pos.SetXYZ(child_shower->ShowerStart().X(),child_shower->ShowerStart().Y(),child_shower->ShowerStart().Z());
      std::vector<art::Ptr<anab::MVAPIDResult> > pids = fmpids.at(child_shower.key());
      child_pfp_mva_pid_result = pids.at(0);

    }
    else {
      mf::LogWarning("PandizzleAlg") << "Unknown PFP PDG when finding Michel candidate"<< child_pfp->PdgCode();

    }
    double curr_distance = (child_start_pos-mu_end_position).Mag();
    if (curr_distance < closest_distance){
      closest_distance = curr_distance;
      closest_child_pfp = child_pfp;
      closest_child_pfp_mva_pid_result = child_pfp_mva_pid_result;
    }
  }

  fVarHolder.FloatVars["PFPMichelDist"] = closest_distance;

  std::vector<art::Ptr<recob::Hit> > michel_hits = GetPFPHits(closest_child_pfp, evt);
  fVarHolder.IntVars["PFPMichelNHits"] = michel_hits.size();
  int g4id = FDSelectionUtils::TrueParticleIDFromTotalRecoHits(michel_hits); 
  if (g4id > 0){
    art::ServiceHandle<cheat::ParticleInventoryService> pi_serv;
    const simb::MCParticle* matched_mcparticle = pi_serv->ParticleList().at(g4id);
    if (matched_mcparticle){
      fVarHolder.IntVars["PFPMichelTrueID"] = matched_mcparticle->TrackId();
      fVarHolder.IntVars["PFPMichelTrueMotherID"] = matched_mcparticle->Mother();
      fVarHolder.IntVars["PFPMichelTruePDG"] = matched_mcparticle->PdgCode();
    }
  }

  std::map<std::string,double> mvaOutMap = closest_child_pfp_mva_pid_result->mvaOutput;
  fVarHolder.FloatVars["PFPMichelElectronMVA"] = mvaOutMap["electron"];

  //Reco energy
  fVarHolder.FloatVars["PFPMichelRecoEnergyPlane0"] = fShowerEnergyAlg.ShowerEnergy(michel_hits, 0);
  fVarHolder.FloatVars["PFPMichelRecoEnergyPlane1"] = fShowerEnergyAlg.ShowerEnergy(michel_hits, 1);
  fVarHolder.FloatVars["PFPMichelRecoEnergyPlane2"] = fShowerEnergyAlg.ShowerEnergy(michel_hits, 2);

  return;
}

void FDSelection::PandizzleAlg::FillTrackVariables(const art::Ptr<recob::PFParticle> pfp, const art::Event& evt){
  if (pfp->PdgCode() != 13) return;

  //Get the PFP handle
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  if (!(evt.getByLabel(fPFParticleModuleLabel, pfparticleListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::PFParticle> with module label: " << fPFParticleModuleLabel;
    return;
  }
  //Get the track handle
  art::Handle< std::vector<recob::Track> > trackListHandle;
  if (!(evt.getByLabel(fTrackModuleLabel, trackListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::Track> with module label: " << fTrackModuleLabel;
    return;
  }


  art::FindManyP<recob::Track> fmtpfp(pfparticleListHandle, evt, fTrackModuleLabel);
  std::vector<art::Ptr<recob::Track> > pfp_tracks = fmtpfp.at(pfp.key());
  if (pfp_tracks.size() > 1){
    mf::LogWarning("PandizzleAlg") << "Found more than one recob::Track matched to PFP: " << pfp_tracks.size();
    return;
  }
  if (pfp_tracks.size() == 0){
    return; //Nothing to do
  }

  art::Ptr<recob::Track> pfp_track = pfp_tracks.at(0);
  CalculateTrackDeflection(pfp_track);
  CalculateTrackLengthRatio(pfp_track, evt);
  //PID
  art::FindManyP<anab::MVAPIDResult> fmpidt(trackListHandle, evt, fPIDModuleLabel);
  std::vector<art::Ptr<anab::MVAPIDResult> > pids = fmpidt.at(pfp_track.key());
  art::Ptr<anab::MVAPIDResult> pid = pids[0];
  std::map<std::string,double> mvaOutMap = pid->mvaOutput;
  fVarHolder.FloatVars["PFPTrackMuonMVA"] = mvaOutMap["muon"];
  fVarHolder.FloatVars["PFPTrackProtonMVA"] = mvaOutMap["proton"];
  fVarHolder.FloatVars["PFPTrackPionMVA"] = mvaOutMap["pion"];
  fVarHolder.FloatVars["PFPTrackPhotonMVA"] = mvaOutMap["photon"];
  fVarHolder.FloatVars["PFPTrackElectronMVA"] = mvaOutMap["electron"];






  //fVarHolder.FloatVars["PFPMichelElectronMVA"]

  return;
}

void FDSelection::PandizzleAlg::CalculateTrackDeflection(const art::Ptr<recob::Track> track){
  //Loop over the trajectory points in a track and compare adjacent pairs
  //Get the number of points
  size_t NPoints = track->NumberTrajectoryPoints();
  //Store the directions between adjacent points on a vector
  std::vector<TVector3> directions;
  for (size_t i_point = 0; i_point < NPoints-1; i_point++){
    TVector3 position_i(track->TrajectoryPoint(i_point).position.X(), track->TrajectoryPoint(i_point).position.Y(), track->TrajectoryPoint(i_point).position.Z());
    TVector3 position_iplus1(track->TrajectoryPoint(i_point+1).position.X(), track->TrajectoryPoint(i_point+1).position.Y(), track->TrajectoryPoint(i_point+1).position.Z());
    TVector3 direction = (position_iplus1-position_i).Unit();
    directions.push_back(direction);
  }

  //Now let's loop through the direction and compare adjacent elements (wow!)
  std::vector<double> deflection_angles;
  for (size_t i_dir = 0; i_dir < directions.size()-1; i_dir++){
    //double angle = directions[i_dir].Angle(directions[i_dir+1]) * 180 / 3.142;
    //Aim: rotate both direction so that the first direction is parallel to the z-axis.  Then take the x-projection of scattered track and calculate the angle between that and the first direction
    TVector3 z_axis(0,0,1);
    TVector3 direction_first = directions[i_dir];
    TVector3 direction_second = directions[i_dir+1];

    //Ignore if either direction is 0 (i.e. not 1)
    if (direction_first.Mag() < 0.999 || direction_second.Mag() < 0.999){
      continue;
    }
    double angle_dir_first_z_axis = direction_first.Angle(z_axis);
    TVector3 orthogonal_vector = direction_first.Cross(z_axis);
    direction_first.Rotate(angle_dir_first_z_axis, orthogonal_vector);
    direction_second.Rotate(angle_dir_first_z_axis, orthogonal_vector);
    //Now work out the angle between the vectors in th x-z plane
    //double angle = atan(direction_second.X()/direction_first.Z());
    direction_first.SetY(0);
    direction_second.SetY(0);
    double dot_product = direction_first.Dot(direction_second);
    double angle = acos(dot_product) * 180/3.142;
    //define +x as a +angle
    if (direction_second.X() < 0) angle*=-1;
    deflection_angles.push_back(angle);
  }
  double angle_mean = 0;
  for (size_t i_angle = 0; i_angle < deflection_angles.size(); i_angle++){
    angle_mean += deflection_angles[i_angle];
  }
  angle_mean/=deflection_angles.size();
  double angle_sd = 0;
  for (size_t i_angle = 0; i_angle < deflection_angles.size(); i_angle++){
    angle_sd = (deflection_angles[i_angle] - angle_mean)*(deflection_angles[i_angle] - angle_mean);
  }
  angle_sd/=(deflection_angles.size()-1);
  fVarHolder.FloatVars["PFPTrackDeflecAngleMean"] = angle_mean;
  fVarHolder.FloatVars["PFPTrackDeflecAngleSD"] = angle_sd;
  return;
}

void FDSelection::PandizzleAlg::CalculateTrackLengthRatio(const art::Ptr<recob::Track> track, const art::Event& evt){
  //Get the PFP handle
  art::Handle< std::vector<recob::PFParticle> > pfparticleListHandle;
  if (!(evt.getByLabel(fPFParticleModuleLabel, pfparticleListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::PFParticle> with module label: " << fPFParticleModuleLabel;
    return;
  }
  //Get the track handle
  art::Handle< std::vector<recob::Track> > trackListHandle;
  if (!(evt.getByLabel(fTrackModuleLabel, trackListHandle))){
    mf::LogWarning("PandizzleAlg") << "Unable to find std::vector<recob::Track> with module label: " << fTrackModuleLabel;
    return;
  }
  //Need to get the PFP back from the track
  art::FindManyP<recob::PFParticle> fmpfptrk(trackListHandle, evt, fTrackModuleLabel);
  std::vector<art::Ptr<recob::PFParticle> > matched_pfps = fmpfptrk.at(track.key());
  if (matched_pfps.size() > 1){
    mf::LogWarning("PandizzleAlg") << "Found more than one recob::PFParticle matched to the track: " << matched_pfps.size();
    return;
  }
  if (matched_pfps.size() == 0){
    return; //Nothing to do
  }
  art::Ptr<recob::PFParticle> track_pfp = matched_pfps[0];

  std::vector<art::Ptr<recob::PFParticle> > pfparticleList;
  art::fill_ptr_vector(pfparticleList, pfparticleListHandle);

  //Ceate the full PFP map
  lar_pandora::PFParticleMap pfparticleMap;
  lar_pandora::LArPandoraHelper::BuildPFParticleMap(pfparticleList, pfparticleMap);

  //Grab the primary PFPs (the neutrinos) from the 
  std::vector<art::Ptr<recob::PFParticle> > nu_pfps;
  lar_pandora::LArPandoraHelper::SelectNeutrinoPFParticles(pfparticleList, nu_pfps);

  art::FindManyP<recob::Track> fmptrkpfp(pfparticleListHandle, evt, fTrackModuleLabel);

  double average_length = 0;
  int ntracks = 0;
  //Now grab the primary children of these PFP
  for (unsigned int i_nupfp = 0; i_nupfp < nu_pfps.size(); i_nupfp++){
    art::Ptr<recob::PFParticle> nu_pfp = nu_pfps[i_nupfp];
    //std::cout<<"Nu ID: " << nu_pfp->Self() << std::endl;
    std::vector<art::Ptr<recob::PFParticle> > child_pfps = SelectChildPFParticles(nu_pfp, pfparticleMap);
    //Assess each child pfp
    for (unsigned int i_childpfp = 0; i_childpfp < child_pfps.size(); i_childpfp++){
      art::Ptr<recob::PFParticle> child_pfp = child_pfps[i_childpfp];
      //Don't assess the particle we care about in the average
      if (child_pfp->Self() == track_pfp->Self()) continue;
      std::vector<art::Ptr<recob::Track> > matched_tracks = fmptrkpfp.at(child_pfp.key());
      if (matched_tracks.size() > 1){
        mf::LogWarning("PandizzleAlg") << "Found more than one recob::Track matched to PFP: " << matched_tracks.size();
        continue;
      }
      if (matched_tracks.size() == 0){
        continue;
      }
      art::Ptr<recob::Track> matched_track = matched_tracks[0];
      average_length += matched_track->Length();
      ntracks++;
    }
  }

  average_length/=ntracks;
  double track_length = track->Length(); 
  fVarHolder.FloatVars["PFPTrackLengthRatio"] = track_length/average_length;

}