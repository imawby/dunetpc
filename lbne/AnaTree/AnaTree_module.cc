////////////////////////////////////////////////////////////////////////
// Class:       AnaTree
// Module Type: analyzer
// File:        AnaTree_module.cc
//
// Generated at Sun Mar 24 09:05:02 2013 by Tingjun Yang using artmod
// from art v1_02_06.
//
//  ** modified by Muhammad Elnimr to access track information and clusters as well
//  mmelnimr@as.ua.edu
//  August 2014
//
////////////////////////////////////////////////////////////////////////
// Framework includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h" 
#include "art/Framework/Principal/Event.h" 
#include "fhiclcpp/ParameterSet.h" 
#include "art/Framework/Principal/Handle.h" 
#include "art/Persistency/Common/Ptr.h" 
#include "art/Persistency/Common/PtrVector.h" 
#include "art/Framework/Services/Registry/ServiceHandle.h" 
#include "art/Framework/Services/Optional/TFileService.h" 
#include "art/Framework/Services/Optional/TFileDirectory.h" 
#include "messagefacility/MessageLogger/MessageLogger.h" 

// LArSoft includes
#include "Geometry/Geometry.h"
#include "Geometry/PlaneGeo.h"
#include "Geometry/WireGeo.h"
#include "RecoBase/Hit.h"
#include "RecoBase/Cluster.h"
#include "RecoBase/Track.h"
#include "RecoBase/SpacePoint.h"
#include "Utilities/LArProperties.h"
#include "Utilities/DetectorProperties.h"
#include "Utilities/AssociationUtil.h"
#include "RawData/ExternalTrigger.h"
#include "MCCheater/BackTracker.h"


#include "SimulationBase/MCParticle.h"
#include "SimulationBase/MCTruth.h"

// ROOT includes
#include "TTree.h"
#include "TTimeStamp.h"
#include "TLorentzVector.h"
#include "TH2F.h"
#include "TFile.h"

//standard library includes
#include <map>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <memory>

const int kMaxTrack      = 1000;  //maximum number of tracks
const int kMaxHits       = 10000; //maximum number of hits
const int kMaxClust       = 10000; //maximum number of clusters
const int kMaxTrackHits  = 1000;  //maximum number of space points

namespace AnaTree {
  class AnaTree;
}
namespace {

  // Local functions.

  // Calculate distance to boundary.

  //----------------------------------------------------------------------------

  double bdist(const TVector3& pos, unsigned int tpc = 0, unsigned int /*cstat*/ = 0)

  {

    // Get geometry.

    double d3,d4;

    art::ServiceHandle<geo::Geometry> geom;

      

    if(tpc==2 || tpc==3 || tpc==4 || tpc==5)

      {

        d3 = pos.Y() - 1.25;     // 1.25cm APA 2/3 distance to horizontal.

        //    double d3 = pos.Y() + 85.0;     // Distance to bottom.

        d4 = 113.0 - pos.Y();     // Distance to top.

        //    double d4 = 113.0 - pos.Y();     // Distance to top.

      }

    else  //tpc 0 1  6  7

      {

        d3 = pos.Y() + geom->DetHalfHeight(tpc)-15.0;     // Distance to bottom.

        //    double d3 = pos.Y() + 85.0;     // Distance to bottom.

        d4 = geom->DetHalfHeight(tpc)+15.0 - pos.Y();     // Distance to top.

        //    double d4 = 113.0 - pos.Y();     // Distance to top.

      }

    

    //    mf::LogVerbatim("output") <<"d3" << d3;

    //    mf::LogVerbatim("output") <<"d4" << d4;

    double d1 = abs(pos.X());          // Distance to right side (wires).

    double d2=2.*geom->DetHalfWidth(tpc)- abs(pos.X());

    //    mf::LogVerbatim("output") <<"d1" << d1;

    //    mf::LogVerbatim("output") <<"d2" << d2;

    //    double d2 = 226.539 - pos.X();   // Distance to left side (cathode).

    double d5 = 0.;

    double d6 = 0.;

    

    if(tpc==0 || tpc==1)

      {

        d5 = pos.Z()+1.0;                             // Distance to front.

        d6 = geom->DetLength(tpc) -1.0- pos.Z();         // Distance to back.

      }

    else if (tpc==2||tpc==3 || tpc==4 || tpc==5)

      {

        d5 = pos.Z()-51.0;                             // Distance to front.     

        d6 = geom->DetLength(tpc) +51.0- pos.Z();         // Distance to back.   

      }

    else if (tpc==6 || tpc==7)

      {

        d5 = pos.Z()-103.0;                             // Distance to front.     

        d6 = geom->DetLength(tpc) +103.0- pos.Z();         // Distance to back.   

        

      }

    if(d6<0){

      //      mf::LogVerbatim("output")<< "z"  <<pos.Z();

      //      mf::LogVerbatim("output")<< "Tpc" <<tpc;

      //      mf::LogVerbatim("output")<< "DetLength" <<geom->DetLength(tpc);

      

    }

    //    mf::LogVerbatim("output") <<"d5" << d5;

    //    mf::LogVerbatim("output") <<"d6" << d6;

    double result = std::min(std::min(std::min(std::min(std::min(d1, d2), d3), d4), d5), d6);

    //    mf::LogVerbatim("output")<< "bdist" << result;

    //    mf::LogVerbatim("output")<< "Height" << geom->DetHalfHeight(tpc);

    //    mf::LogVerbatim("output")<< "Width" << geom->DetHalfWidth(tpc);

    if(result<0) result=0;

    return result;
  }
  // Length of reconstructed track.
  //----------------------------------------------------------------------------
  double length(const recob::Track& track)
  {
    double result = 0.;
    TVector3 disp = track.LocationAtPoint(0);
    int n = track.NumberTrajectoryPoints();
    for(int i = 1; i < n; ++i) {
      const TVector3& pos = track.LocationAtPoint(i);

      disp -= pos;

      result += disp.Mag();

      disp = pos;
    }
    //    mf::LogVerbatim("output") << " length (track) " << result;
    return result;
  }
  // Length of MC particle.
  //----------------------------------------------------------------------------
  double length(const simb::MCParticle& part, double dx,
                TVector3& start, TVector3& end, TVector3& startmom, TVector3& endmom,
                unsigned int /*tpc*/ = 0, unsigned int /*cstat*/ = 0)
  {

    // Get services.

    art::ServiceHandle<geo::Geometry> geom;

    art::ServiceHandle<util::DetectorProperties> detprop;

    double xmin,xmax,ymin,ymax,zmin,zmax;

    /*  if(tpc==0 || tpc==2 || tpc==4 || tpc==6 ) //short tpcs

      {

        

        xmin=-0.9;

        xmax=xmin-2.*geom->DetHalfWidth(tpc);

      }

    else  //long tpcs

      {

        xmin=-0.9;

        xmax=2*geom->DetHalfWidth(tpc)+xmin;

      }





    if(tpc==2 || tpc==3 )

      {

        ymin = 1.25;  //35t

        ymax = ymin+2*geom->DetHalfHeight(tpc);   //35t

      }

    else if( tpc==4 || tpc==5)

      {

        ymin=-1.25;

        ymax=ymin-2*geom->DetHalfHeight(tpc);


      }

    else // tpcs 0 ,1 ,6 ,7 

      {

        ymin=geom->DetHalfHeight(tpc)-1.25;

        ymax=2* geom->DetHalfHeight(tpc)+ymin;

      }




  if(tpc==0 || tpc==1)

    {

      zmin=-1.0;

      zmax=geom->DetLength(tpc)+zmin;


    }    

  else if (tpc==2||tpc==3 || tpc==4 || tpc==5)

    {

      zmin=51;

      zmax=zmin+geom->DetLength(tpc);

      

    }

   else if (tpc==6 || tpc==7)

     {

       zmin=103;

       zmax=zmin+geom->DetLength(tpc);

     }


 mf::LogVerbatim("output") << " xmin " << xmin;

 mf::LogVerbatim("output") << " xmax " << xmax;

 mf::LogVerbatim("output") << " ymin " << ymin;

 mf::LogVerbatim("output") << " ymax " << ymax;

 mf::LogVerbatim("output") << " zmin " << zmin;

 mf::LogVerbatim("output") << " zmax " << zmax;

    */

    //

    // The MC should be independent of the tpc

    //

    // It cares only about the Cryostat bounds.

    //

    //  However, to be able to use the ConvertXToTicks function

    //  we need to know the tpc number, will predict it 

    //  based on the coordinates of this specific point

    //

    /*

    double origin[3] = {0.};

    double world[3] = {0.};

    const int cc=0;

    geom->Cryostat(cc).LocalToWorld(origin, world);

    xmin=world[0] - geom->Cryostat(cc).HalfWidth();

    xmax=world[0] + geom->Cryostat(cc).HalfWidth();


    ymin= world[1] - geom->Cryostat(cc).HalfHeight();

    ymax=world[1] + geom->Cryostat(cc).HalfHeight();


    zmin=world[2] - geom->Cryostat(cc).Length()/2;

    zmax=world[2] + geom->Cryostat(cc).Length()/2;

    //

    //

mf::LogVerbatim("output") << " xmin " << xmin;

 mf::LogVerbatim("output") << " xmax " << xmax;

 mf::LogVerbatim("output") << " ymin " << ymin;

 mf::LogVerbatim("output") << " ymax " << ymax;

 mf::LogVerbatim("output") << " zmin " << zmin;

 mf::LogVerbatim("output") << " zmax " << zmax;*/

    xmin=-50;

    xmax=230;

    ymin=-85;

    ymax=113;

    zmin=-10;

    zmax=153;

    double result = 0.;

    TVector3 disp;

    int n = part.NumberTrajectoryPoints();

    bool first = true;

    for(int i = 0; i < n; ++i) {

      TVector3 pos = part.Position(i).Vect();

      // Make fiducial cuts.  Require the particle to be within the physical volume of

      // the tpc, and also require the apparent x position to be within the expanded

      // readout frame.

      // predict the tpc number

      int whichTPC=0;

      if(pos.Z() >=-1.0 && pos.Z()<=49.0 && pos.Y() >=-85.0 && pos.Y() <= 113.0)   

        {

          if(pos.X() >=-1.0) whichTPC=1;

          else if (pos.X() <-1.0) whichTPC=0; 

          else whichTPC=-999;

        }

      if(pos.Z() >=103.0 && pos.Z()<=153.0 && pos.Y() >=-85.0 && pos.Y() <= 113.0)

         {

          if(pos.X() >=-1.0) whichTPC=7;

          else if (pos.X() <-1.0) whichTPC=6; 

          else whichTPC=-999;

         }

      if(pos.Z() >=51.0 && pos.Z()<=101.0 && pos.Y() >= 1.25 && pos.Y() <= 113.0)

         {

          if(pos.X() >=-1.0) whichTPC=3;

          else if (pos.X() <-1.0) whichTPC=2; 

          else whichTPC=-999;

         }

      if(pos.Z() >=51.0 && pos.Z()<=101.0 && pos.Y() >= - 85.0 && pos.Y() <= 1.25)

         {

          if(pos.X() >=-1.0) whichTPC=5;

          else if (pos.X() <-1.0) whichTPC=4; 

          else whichTPC=-999;

         }

      

      //      mf::LogVerbatim("output") << " x " << pos.X();

      //      mf::LogVerbatim("output") << " y " << pos.Y();

      //      mf::LogVerbatim("output") << " z " << pos.Z();

      if(pos.X() >= xmin &&

         pos.X() <= xmax &&

         pos.Y() >= ymin &&

         pos.Y() <= ymax &&

         pos.Z() >= zmin &&

         pos.Z() <= zmax) {

        pos[0] += dx;

        //        double ticks = detprop->ConvertXToTicks(pos[0], 0, 0, 0);

        double ticks = detprop->ConvertXToTicks(pos[0], 0, whichTPC, 0);

        if(ticks >= 0. && ticks < detprop->ReadOutWindowSize()) {

          if(first) {

            start = pos;

            startmom = part.Momentum(i).Vect();

          }

          else {

            disp -= pos;

            result += disp.Mag();

          }

          first = false;

          disp = pos;

          end = pos;

          endmom = part.Momentum(i).Vect();

        }

      }

    }

    //    mf::LogVerbatim("output") << " length (MCParticle) " << result;

    return result;

  }

  // Fill efficiency histogram assuming binomial errors.

  void effcalc(const TH1* hnum, const TH1* hden, TH1* heff)

  {

    int nbins = hnum->GetNbinsX();

    if (nbins != hden->GetNbinsX())

      throw cet::exception("TrackAnaCT") << "effcalc[" __FILE__ "]: incompatible histograms (I)\n";

    if (nbins != heff->GetNbinsX())

      throw cet::exception("TrackAnaCT") << "effcalc[" __FILE__ "]: incompatible histograms (II)\n";

    // Loop over bins, including underflow and overflow.

    for(int ibin = 0; ibin <= nbins+1; ++ibin) {

      double num = hnum->GetBinContent(ibin);

      double den = hden->GetBinContent(ibin);

      if(den == 0.) {

        heff->SetBinContent(ibin, 0.);

        heff->SetBinError(ibin, 0.);

      }

      else {

        double eff = num / den;

        if(eff < 0.)

          eff = 0.;

        if(eff > 1.)

          eff = 1.;

        double err = std::sqrt(eff * (1.-eff) / den);

        heff->SetBinContent(ibin, eff);

        heff->SetBinError(ibin, err);

      }

    }

    heff->SetMinimum(0.);

    heff->SetMaximum(1.05);

    heff->SetMarkerStyle(20);

  }

}
class AnaTree::AnaTree : public art::EDAnalyzer {
public:
  explicit AnaTree(fhicl::ParameterSet const & p);
  virtual ~AnaTree();

  void analyze(art::Event const & e) override;

  void beginJob() override;
  void endJob();
  //void reconfigure(fhicl::ParameterSet const & p) override;

private:

  void ResetVars();
  
  TTree* fTree;
  //run information
  int run;
  int subrun;
  int event;
  double evttime;
  double efield[3];
  int t0;
  int trigtime[16];
  int ntracks_reco;         //number of reconstructed tracks
  double trkstartx[kMaxTrack];
  double trkstarty[kMaxTrack];
  double trkstartz[kMaxTrack];
  double trkendx[kMaxTrack];
  double trkendy[kMaxTrack];
  double trkendz[kMaxTrack];

 double trkstartx_MC[kMaxTrack];
  double trkstarty_MC[kMaxTrack];
  double trkstartz_MC[kMaxTrack];
  double trkendx_MC[kMaxTrack];
  double trkendy_MC[kMaxTrack];
  double trkendz_MC[kMaxTrack];
  double trklen_MC[kMaxTrack];
  double trklen_cut_MC[kMaxTrack];

  double trkmom[kMaxTrack];
  double trkpdg[kMaxTrack];
  double trkd2[kMaxTrack];
  double trkcolin[kMaxTrack];
  double trklen[kMaxTrack];
  double trklen_L[kMaxTrack];
  double trkid[kMaxTrack];
 double trktheta_xz[kMaxTrack];
  double trktheta_yz[kMaxTrack];
  double trktheta[kMaxTrack];
  double trkphi[kMaxTrack];
  double mcang_x;
  double mcang_y;
  double mcang_z;
  double mcpos_x;
  double mcpos_y;
  double mcpos_z;

  double trkang[kMaxTrack];
  double trkcolinearity[kMaxTrack];
  double trkmatchdisp[kMaxTrack];
  double trkwmatchdisp[kMaxTrack];
  double trklenratio[kMaxTrack];
  double trkstartdcosx[kMaxTrack];
  double trkstartdcosy[kMaxTrack];
  double trkstartdcosz[kMaxTrack];
  double trkenddcosx[kMaxTrack];
  double trkenddcosy[kMaxTrack];
  double trkenddcosz[kMaxTrack];
  int    ntrkhits[kMaxTrack];
  double trkx[kMaxTrack][kMaxTrackHits];
  double trky[kMaxTrack][kMaxTrackHits];
  double trkz[kMaxTrack][kMaxTrackHits];
  double trkpitch[kMaxTrack][3];
  int    nhits;

  std::string fTrigModuleLabel;
  std::string fHitsModuleLabel;
  std::string fTrackModuleLabel;
  std::string fClusterModuleLabel;
  std::string fTrkSpptAssocModuleLabel;
  std::string fHitSpptAssocModuleLabel;
  
  int fDump;                 // Number of events to dump to debug message facility.
  double fMinMCKE;           // Minimum MC particle kinetic energy (GeV).
  double fMinMCLen;          // Minimum MC particle length in tpc (cm).
  double fMatchColinearity;  // Minimum matching colinearity.
  double fMatchDisp;         // Maximum matching displacement.
  double fWMatchDisp;        // Maximum matching displacement in the w direction.
  bool fIgnoreSign;          // Ignore sign of mc particle if true.
  bool fStitchedAnalysis;    // if true, do the whole drill-down from stitched track to assd hits

  struct RecoHists
    {
      // Constructors.
      RecoHists();
      //      ~RecoHists();
      RecoHists(const std::string& subdir);
      // Pure reco track histograms.
      TH1F* fHstartx;      // Starting x position.
      TH1F* fHstarty;      // Starting y position.
      TH1F* fHstartz;      // Starting z position.
      TH1F* fHstartd;      // Starting distance to boundary.
      TH1F* fHendx;        // Ending x position.
      TH1F* fHendy;        // Ending y position.
      TH1F* fHendz;        // Ending z position.
      TH1F* fHendd;        // Ending distance to boundary.
      TH1F* fHtheta;       // Theta.
      TH1F* fHphi;         // Phi.
      TH1F* fHtheta_xz;    // Theta_xz.
      TH1F* fHtheta_yz;    // Theta_yz.
      TH1F* fHmom;         // Momentum.
      TH1F* fHlen;         // Length.
      // Histograms for the consituent Hits
      TH1F* fHHitChg;       // hit charge
      TH1F* fHHitWidth;     // hit width
      TH1F* fHHitPdg;       // Pdg primarily responsible.
      TH1F* fHHitTrkId;     // TrkId
      TH1F* fModeFrac;       // mode fraction
      TH1F* fNTrkIdTrks;    // # of stitched tracks in which unique TrkId appears
      TH2F* fNTrkIdTrks2;   
      TH2F* fNTrkIdTrks3;   
    };

    // Struct for mc particles and mc-matched tracks.
    struct MCHists
    {
      // Constructors.
      MCHists();
      MCHists(const std::string& subdir);
      // Reco-MC matching.
      TH2F* fHduvcosth;    // 2D mc vs. data matching, duv vs. cos(theta).
      TH1F* fHcosth;       // 1D direction matching, cos(theta).
      TH1F* fHmcu;         // 1D endpoint truth u.
      TH1F* fHmcv;         // 1D endpoint truth v.
      TH1F* fHmcw;         // 1D endpoint truth w.
      TH1F* fHupull;       // 1D endpoint u pull.
      TH1F* fHvpull;       // 1D endpoint v pull.
      TH1F* fHmcdudw;      // Truth du/dw.
      TH1F* fHmcdvdw;      // Truth dv/dw.
      TH1F* fHdudwpull;    // du/dw pull.
      TH1F* fHdvdwpull;    // dv/dw pull.
      // Histograms for matched tracks.
      TH1F* fHstartdx;     // Start dx.
      TH1F* fHstartdy;     // Start dy.
      TH1F* fHstartdz;     // Start dz.
      TH1F* fHenddx;       // End dx.
      TH1F* fHenddy;       // End dy.
      TH1F* fHenddz;       // End dz.
      TH2F* fHlvsl;        // MC vs. reco length.
      TH1F* fHdl;          // Delta(length).
      TH2F* fHpvsp;        // MC vs. reco momentum.
      TH2F* fHpvspc;       // MC vs. reco momentum (contained tracks).
      TH1F* fHdp;          // Momentum difference.
      TH1F* fHdpc;         // Momentum difference (contained tracks).
      TH1F* fHppull;       // Momentum pull.
      TH1F* fHppullc;      // Momentum pull (contained tracks).
      // Pure MC particle histograms (efficiency denominator).
      TH1F* fHmcstartx;    // Starting x position.
      TH1F* fHmcstarty;    // Starting y position.
      TH1F* fHmcstartz;    // Starting z position.
      TH1F* fHmcendx;      // Ending x position.
      TH1F* fHmcendy;      // Ending y position.
      TH1F* fHmcendz;      // Ending z position.
      TH1F* fHmctheta;     // Theta.
      TH1F* fHmcphi;       // Phi.
      TH1F* fHmctheta_xz;  // Theta_xz.
      TH1F* fHmctheta_yz;  // Theta_yz.
      TH1F* fHmcmom;       // Momentum.
      TH1F* fHmclen;       // Length.
      // Histograms for well-reconstructed matched tracks (efficiency numerator).
      TH1F* fHgstartx;     // Starting x position.
      TH1F* fHgstarty;     // Starting y position.
      TH1F* fHgstartz;     // Starting z position.
      TH1F* fHgendx;       // Ending x position.
      TH1F* fHgendy;       // Ending y position.
      TH1F* fHgendz;       // Ending z position.
      TH1F* fHgtheta;      // Theta.
      TH1F* fHgphi;        // Phi.
      TH1F* fHgtheta_xz;   // Theta_xz.
      TH1F* fHgtheta_yz;   // Theta_yz.
      TH1F* fHgmom;        // Momentum.
      TH1F* fHglen;        // Length.
      // Efficiency histograms.
      TH1F* fHestartx;     // Starting x position.
      TH1F* fHestarty;     // Starting y position.
      TH1F* fHestartz;     // Starting z position.
      TH1F* fHeendx;       // Ending x position.
      TH1F* fHeendy;       // Ending y position.
      TH1F* fHeendz;       // Ending z position.
      TH1F* fHetheta;      // Theta.
      TH1F* fHephi;        // Phi.
      TH1F* fHetheta_xz;   // Theta_xz.
      TH1F* fHetheta_yz;   // Theta_yz.
      TH1F* fHemom;        // Momentum.
      TH1F* fHelen;        // Length.
    };
    std::map<int, MCHists> fMCHistMap;       // Indexed by pdg id.
    std::map<int, RecoHists> fRecoHistMap;   // Indexed by pdg id.

};


AnaTree::AnaTree::AnaTree(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , fTrigModuleLabel       (pset.get< std::string >("TrigModuleLabel"))
  , fHitsModuleLabel       (pset.get< std::string >("HitsModuleLabel"))
  , fTrackModuleLabel       (pset.get< std::string >("TrackModuleLabel"))
  , fClusterModuleLabel       (pset.get< std::string >("ClusterModuleLabel"))
  , fTrkSpptAssocModuleLabel    (pset.get< std::string >("TrkSpptAssocModuleLabel"))
  , fHitSpptAssocModuleLabel    (pset.get< std::string >("HitSpptAssocModuleLabel"))
  , fDump              (pset.get<int>("Dump"))
  , fMinMCKE            (pset.get<double>("MinMCKE"))
  , fMinMCLen           (pset.get<double>("MinMCLen"))
  , fMatchColinearity       (pset.get<double>("MatchColinearity"))
  , fMatchDisp             (pset.get<double>("MatchDisp"))
  , fWMatchDisp             (pset.get<double>("WMatchDisp"))
  , fIgnoreSign             (pset.get<bool>("IgnoreSign"))
  , fStitchedAnalysis       (pset.get<bool>("StitchedAnalysis",false))
{

}

AnaTree::AnaTree::~AnaTree()
{
  // Clean up dynamic memory and other resources here.

}

void AnaTree::AnaTree::analyze(art::Event const & evt)
{
  // Implementation of required member function here.
  ResetVars();

  art::ServiceHandle<geo::Geometry> geom;
  art::ServiceHandle<util::LArProperties> larprop;
  art::ServiceHandle<util::DetectorProperties> detprop;
  
  run = evt.run();
  subrun = evt.subRun();
  event = evt.id().event();
  art::Timestamp ts = evt.time();
  TTimeStamp tts(ts.timeHigh(), ts.timeLow());
  evttime = tts.AsDouble();


  efield[0] = larprop->Efield(0);
  efield[1] = larprop->Efield(1);
  efield[2] = larprop->Efield(2);

  t0 = detprop->TriggerOffset();

  art::Handle< std::vector<raw::ExternalTrigger> > trigListHandle;
  std::vector<art::Ptr<raw::ExternalTrigger> > triglist;
  if (evt.getByLabel(fTrigModuleLabel,trigListHandle))
    art::fill_ptr_vector(triglist, trigListHandle);

  for (size_t i = 0; i<triglist.size(); ++i){
    trigtime[i] = triglist[i]->GetTrigTime();
  }

  art::Handle< std::vector<recob::Track> > trackListHandle;
  std::vector<art::Ptr<recob::Track> > tracklist;
  if (evt.getByLabel(fTrackModuleLabel,trackListHandle))
    art::fill_ptr_vector(tracklist, trackListHandle);    

  art::Handle< std::vector<recob::Hit> > hitListHandle;
  std::vector<art::Ptr<recob::Hit> > hitlist;
  if (evt.getByLabel(fHitsModuleLabel,hitListHandle))
    art::fill_ptr_vector(hitlist, hitListHandle);

  //  Event.getByLabel(fSimulationProducerLabel, particleHandle);
  //  art::Handle< std::vector<simb::MCParticle> > particleHandle;

  art::ServiceHandle<cheat::BackTracker> bt;
  //  int trkid=1;
  const sim::ParticleList& plist = bt->ParticleList();

  simb::MCParticle *particle=0;
  //  const simb::MCParticle *particle = bt->TrackIDToParticle(trkid);
  for ( sim::ParticleList::const_iterator ipar = plist.begin(); ipar!=plist.end(); ++ipar){
    particle = ipar->second;
    
    int fPDG=13;
    if(!(particle->Process()=="primary" && particle->PdgCode()== fPDG)) continue;

  //  size_t numberTrajectoryPoints = particle->NumberTrajectoryPoints();
  //  int last = numberTrajectoryPoints - 1;
  //  const TLorentzVector& momentumStart = particle->Momentum(0);
  //  const TLorentzVector& momentumEnd   = particle->Momentum(last);
  //    TLorentzVector tmpVec= momentumEnd-momentumStart;

  //  TLorentzVector tmpVec= momentumStart;
  
    TLorentzVector tmpVec;
    /*
  for ( auto const& particle : (*particleHandle) )
   {
     // we know it is only one MC Particle for now
     size_t numberTrajectoryPoints = particle.NumberTrajectoryPoints();
     int last = numberTrajectoryPoints - 1;
     const TLorentzVector& momentumStart = particle.Momentum(0);
     const TLorentzVector& momentumEnd   = particle.Momentum(last);
     tmpVec= momentumEnd-momentumStart;
   }
  */
  art::FindManyP<recob::SpacePoint> fmsp(trackListHandle, evt, fTrackModuleLabel);
  art::FindMany<recob::Track>       fmtk(hitListHandle, evt, fTrackModuleLabel);

 


  //track information
  ntracks_reco=tracklist.size();

  TLorentzVector vectx(1,0,0,0);
  TLorentzVector vecty(0,1,0,0);
  TLorentzVector vectz(0,0,1,0);
  //mcang_x=(1./TMath::DegToRad())*tmpVec.Angle(vectx.Vect());
  //mcang_x=(1./TMath::DegToRad())*(1.-tmpVec.Phi());
  //  mcang_y=(1./TMath::DegToRad())*tmpVec.Phi();
  //  mcang_z=(1./TMath::DegToRad())*tmpVec.Theta();
  mcang_x=particle->Px();
  mcang_y=particle->Py();
  mcang_z=particle->Pz();


  //  mcpos_x=particle->Vx();
  //  mcpos_y=particle->Vy();
  //  mcpos_z=particle->Vz();
  tmpVec=particle->Position();
  mcpos_x=(1./TMath::DegToRad())*tmpVec.Angle(vectx.Vect());
  mcpos_y=(1./TMath::DegToRad())*tmpVec.Angle(vecty.Vect());
  mcpos_z=(1./TMath::DegToRad())*tmpVec.Angle(vectz.Vect());

  double larStart[3];
  double larEnd[3];
  std::vector<double> trackStart;
  std::vector<double> trackEnd;

  /////////////////////
  /////////////  Extra Test
  /////////
  ////////////////////

  art::Handle< std::vector<simb::MCParticle> > particleHandle2;
    evt.getByLabel("largeant", particleHandle2);

    // put it in a more easily usable form
    std::vector< art::Ptr<simb::MCParticle> > particles2;
    art::fill_ptr_vector(particles2, particleHandle2);

    art::Handle< std::vector<sim::SimChannel> > simChannelHandle2;
    evt.getByLabel("largeant", simChannelHandle2);    

    //    int fMC_Ntrack = particles2.size();

    float fMC_startXYZT[1000][4];
    float fMC_endXYZT[1000][4];

    int i=0; // track index
    for (auto const& particle: particles2 ) {
      //        int Ndaughters = particle->NumberDaughters();
      //        vector<int> daughters;
      //        for (int i=0; i<Ndaughters; i++) {
      //            daughters.push_back(particle->Daughter(i));
      //        }
      //        fMC_daughters.push_back(daughters);
      size_t numberTrajectoryPoints = particle->NumberTrajectoryPoints();
      trkpdg[i]=particle->PdgCode();
      if(!(particle->Process()=="primary" && particle->PdgCode()==13))
	continue;
      double origin[3] = {0.};
      double world[3] = {0.};
      double xyztArray[4];
      double cryoBound_pos[3];
      double cryoBound_neg[3];
      int c=0;//only one cryo
      
      geom->Cryostat(c).LocalToWorld(origin, world);
      cryoBound_neg[0]=world[0] - geom->Cryostat(c).HalfWidth();
      cryoBound_neg[1]=world[1] - geom->Cryostat(c).HalfWidth();
      cryoBound_neg[2]=world[2] - geom->Cryostat(c).HalfWidth();

      cryoBound_pos[0]=world[0] + geom->Cryostat(c).HalfWidth();
      cryoBound_pos[1]=world[1] + geom->Cryostat(c).HalfWidth();
      cryoBound_pos[2]=world[2] + geom->Cryostat(c).HalfWidth();


      const TLorentzVector& positionStart = particle->Position(0);
      int last = numberTrajectoryPoints - 1;
      TLorentzVector& positionEnd  =( TLorentzVector&)particle->Position(last);

      bool insideActiveVolume=true;
      for(size_t ii=0;ii<numberTrajectoryPoints;ii++)
	  {
	    const TLorentzVector& tmpPosition=particle->Position(ii);
	    tmpPosition.GetXYZT(xyztArray);
	    for(int p=0;p<3;p++)
	      {
		if((xyztArray[p]>cryoBound_pos[p]) || (xyztArray[p]<cryoBound_neg[p]))
		  {
		    insideActiveVolume=false;
		    break;
		  }
	      }
	    
	    if(!insideActiveVolume) 
	      {
		positionEnd=(const TLorentzVector&)particle->Position(ii-1);
		break;
	      }
	  }
     
	//        const TLorentzVector& momentumStart = particle->Momentum(0);
	//        const TLorentzVector& momentumEnd   = particle->Momentum(last);
      TLorentzVector& momentumStart  =( TLorentzVector&)particle->Momentum(0);
      trkmom[i]=momentumStart.P();
      positionStart.GetXYZT(fMC_startXYZT[i]);
      positionEnd.GetXYZT(fMC_endXYZT[i]);
      trkstartx_MC[i]=fMC_startXYZT[i][0];
      trkstarty_MC[i]=fMC_startXYZT[i][1];
      trkstartz_MC[i]=fMC_startXYZT[i][2];
      trkendx_MC[i]=fMC_endXYZT[i][0];
      trkendy_MC[i]=fMC_endXYZT[i][1];
      trkendz_MC[i]=fMC_endXYZT[i][2];
      tmpVec= positionEnd-positionStart;
      trklen_MC[i]=(positionEnd-positionStart).Rho();
      double mctime = particle->T();                                 // nsec
      double mcdx = mctime * 1.e-3 * larprop->DriftVelocity();   // cm
      // Calculate the points where this mc particle enters and leaves the
      // fiducial volume, and the length in the fiducial volume.
      TVector3 mcstart;
      TVector3 mcend;
      TVector3 mcstartmom;
      TVector3 mcendmom;
      double plen = length(*particle, mcdx, mcstart, mcend, mcstartmom, mcendmom);
      trklen_cut_MC[i]=plen;
      //        momentumStart.GetXYZT(fMC_startMomentum[i]);
      //        momentumEnd.GetXYZT(fMC_endMomentum[i]);
      i++;
    } // particle loop done 
    //////////////////////
    /////////////  End of Extra Test
    //////
    ////////////////////



    // **********************
    // **********************
    //
    //  Tracks:
    //
    //
    //
    // *********************
    // *********************

       art::Handle< std::vector<recob::Track> > trackh;
    evt.getByLabel(fTrackModuleLabel, trackh);
    
    for(size_t i=0; i<tracklist.size();++i){
    trkid[i]=i;
    trackStart.clear();
    trackEnd.clear();
    memset(larStart, 0, 3);
    memset(larEnd, 0, 3);
    tracklist[i]->Extent(trackStart,trackEnd); 
    tracklist[i]->Direction(larStart,larEnd);
    trkstartx[i]        = trackStart[0];
    trkstarty[i]        = trackStart[1];
    trkstartz[i]        = trackStart[2];
    trkendx[i]        = trackEnd[0];
    trkendy[i]        = trackEnd[1];
    trkendz[i]        = trackEnd[2];
    trkstartdcosx[i]  = larStart[0];
    trkstartdcosy[i]  = larStart[1];
    trkstartdcosz[i]  = larStart[2];
    TLorentzVector v1(trackStart[0],trackStart[1],trackStart[2],0);
    TLorentzVector v2(trackEnd[0],trackEnd[1],trackEnd[2],0);
    trklen[i]=(v2-v1).Rho();
  
    //    trkang[i]=TMath::Cos((v2-v1).Angle(tmpVec.Vect()));
    trkang[i]=TMath::Cos((v2-v1).Angle(tmpVec.Vect()));
    //    trklen[i]=v.Mag();
    trkenddcosx[i]    = larEnd[0];
    trkenddcosy[i]    = larEnd[1];
    trkenddcosz[i]    = larEnd[2];
    ntrkhits[i] = fmsp.at(i).size();
    std::vector<art::Ptr<recob::SpacePoint> > spts = fmsp.at(i);
    TVector3 V1(trackStart[0],trackStart[1],trackStart[2]);
    TVector3 V2(trackEnd[0],trackEnd[1],trackEnd[2]);
    TVector3 vOrth=(V2-V1).Orthogonal();

    TVector3 pointVector=V1;
    double distance_squared=0;
    double distance=0;

    for (size_t j = 0; j<spts.size(); ++j){

      TVector3 sptVector(spts[j]->XYZ()[0],spts[j]->XYZ()[1],spts[j]->XYZ()[2]);
      TVector3 vToPoint=sptVector-pointVector;
      distance=(vOrth.Dot(vToPoint))/vOrth.Mag();
      distance_squared+=distance *distance;
      trkx[i][j] = spts[j]->XYZ()[0];
      trky[i][j] = spts[j]->XYZ()[1];
      trkz[i][j] = spts[j]->XYZ()[2];
    }

    // *********************
    //   Cuts specific quantities
    //  
    // *********************
    //
    //
    TMatrixD rot(3,3);
    int start_point =0;
    tracklist[i]->GlobalToLocalRotationAtPoint(start_point, rot);
    //  int ntraj = tracklist[i]->NumberTrajectoryPoints();
  // if(ntraj > 0) {
  TVector3 pos = tracklist[i]->Vertex();
  art::ServiceHandle<cheat::BackTracker> bktrk;
  const   sim::ParticleList& ppplist=bktrk->ParticleList();
  std::vector<const simb::MCParticle*> plist2;
  plist2.reserve(ppplist.size());
  double mctime = particle->T();                                 // nsec
  double mcdx = mctime * 1.e-3 * larprop->DriftVelocity();   // cm
  // Calculate the points where this mc particle enters and leaves the
  // fiducial volume, and the length in the fiducial volume.
  TVector3 mcstart;
  TVector3 mcend;
  TVector3 mcstartmom;
  TVector3 mcendmom;
  double plen = length(*particle, mcdx, mcstart, mcend, mcstartmom, mcendmom);
  // Get the displacement of this mc particle in the global coordinate system.
  TVector3 mcpos = mcstart - pos;
  // Rotate the momentum and position to the
  // track-local coordinate system.
  TVector3 mcmoml = rot * mcstartmom;
  TVector3 mcposl = rot * mcpos;
  trktheta_xz[i] = std::atan2(mcstartmom.X(), mcstartmom.Z());
  trktheta_yz[i] = std::atan2(mcstartmom.Y(), mcstartmom.Z());
  trktheta[i]=mcstartmom.Theta();
  trkphi[i]=mcstartmom.Phi();

  trkcolinearity[i] = mcmoml.Z() / mcmoml.Mag();
  double u = mcposl.X();
  double v = mcposl.Y();
  double w = mcposl.Z();
  trkwmatchdisp[i]=w;
  double pu = mcmoml.X();
  double pv = mcmoml.Y();
  double pw = mcmoml.Z();
  double dudw = pu / pw;
  double dvdw = pv / pw;
  double u0 = u - w * dudw;
  double v0 = v - w * dvdw;
  trkmatchdisp[i]=abs( std::sqrt(u0*u0 + v0*v0));
  art::Ptr<recob::Track> ptrack(trackh, i);
  const recob::Track& track = *ptrack;
  trklenratio[i] = length(track)/plen;
  trklen_L[i]=length(track);
  //
  //**********************
  //
  // End Cut specific quantities
  //  
  //***********************
  //
  //
  //
  //
  //


  distance_squared=distance_squared/spts.size();
  trkd2[i]=distance_squared;
  
  for (int j = 0; j<3; ++j){
    try{
      if (j==0)
	trkpitch[i][j] = tracklist[i]->PitchInView(geo::kU);
      else if (j==1)
	trkpitch[i][j] = tracklist[i]->PitchInView(geo::kV);
      else if (j==2)
	trkpitch[i][j] = tracklist[i]->PitchInView(geo::kZ);
    }
    catch( cet::exception &e){
      mf::LogWarning("AnaTree")<<"caught exeption "<<e<<"\n setting pitch to 0";
      trkpitch[i][j] = 0;
    }
  }

  simb::MCParticle* particle=0;
  for ( sim::ParticleList::const_iterator ipar = plist.begin(); ipar!=plist.end(); ++ipar){
    particle = ipar->second;
  }
  int pdg = particle->PdgCode();
  if (pdg!=13) continue;
  TVector3 startmom;
  startmom=particle->Momentum(0).Vect();
  TVector3 mcmomltemp=rot * startmom;
  trkcolin[i]=mcmomltemp.Z()/mcmomltemp.Mag();
  }


  art::Handle< std::vector<recob::Cluster> > clusterListHandle;
  std::vector<art::Ptr<recob::Cluster> > clusterlist;
  if (evt.getByLabel(fClusterModuleLabel,clusterListHandle))
    art::fill_ptr_vector(clusterlist, clusterListHandle);
  
  // **********************
  // **********************
  //   Fill Tree:
  // **********************
  // **********************
  
  fTree->Fill();
  // *********************
  // *********************



  // **********************
  // **********************
  //
  //  Histograms:
  //
  //
  //
  // *********************
  // *********************
 
  std::unique_ptr<mf::LogInfo> pdump;
  if(fDump > 0) {
    --fDump;
    pdump = std::unique_ptr<mf::LogInfo>(new mf::LogInfo("TrackAnaCT"));
  }
  // Histograms.

  art::ServiceHandle<cheat::BackTracker> bt2;
  const   sim::ParticleList& pplist=bt2->ParticleList();
  std::vector<const simb::MCParticle*> plist2;
  plist2.reserve(pplist.size());
  if(particle->E() >= 0.001*particle->Mass() + fMinMCKE) {
    // Calculate the x offset due to nonzero mc particle time.
    double mctime = particle->T();                                // nsec
    double mcdx = mctime * 1.e-3 * larprop->DriftVelocity();  // cm
    // Calculate the length of this mc particle inside the fiducial volume.
    TVector3 mcstart;
    TVector3 mcend;
    TVector3 mcstartmom;
    TVector3 mcendmom;
    double plen = length(*particle, mcdx, mcstart, mcend, mcstartmom, mcendmom);
    // Apply minimum fiducial length cut.  Always reject particles that have
    // zero length in the tpc regardless of the configured cut.
    if(plen > 0. && plen > fMinMCLen) {
      // This is a good mc particle (capable of making a track).
      plist2.push_back(particle);
      // Dump MC particle information here.
      if(pdump) {
	//do nothing
      }
      // Fill histograms.
      int pdg=13;
      if(fMCHistMap.count(pdg) == 0) {
	std::ostringstream ostr;
	ostr << "MC" << (fIgnoreSign ? "All" : (pdg > 0 ? "Pos" : "Neg")) << std::abs(pdg);
	fMCHistMap[pdg] = MCHists(ostr.str());
      }

      const MCHists& mchists = fMCHistMap[pdg];
      double mctheta_xz = std::atan2(mcstartmom.X(), mcstartmom.Z());
      double mctheta_yz = std::atan2(mcstartmom.Y(), mcstartmom.Z());

      mchists.fHmcstartx->Fill(mcstart.X());
      mchists.fHmcstarty->Fill(mcstart.Y());
      mchists.fHmcstartz->Fill(mcstart.Z());
      mchists.fHmcendx->Fill(mcend.X());
      mchists.fHmcendy->Fill(mcend.Y());
      mchists.fHmcendz->Fill(mcend.Z());
      mchists.fHmctheta->Fill(mcstartmom.Theta());
      mchists.fHmcphi->Fill(mcstartmom.Phi());
      mchists.fHmctheta_xz->Fill(mctheta_xz);
      mchists.fHmctheta_yz->Fill(mctheta_yz);
      mchists.fHmcmom->Fill(mcstartmom.Mag());
      mchists.fHmclen->Fill(plen);
    }
  } // ...+minMCKE

  //  art::Handle< std::vector<recob::Track> > trackh;
  //  evt.getByLabel(fTrackModuleLabel, trackh);
  if(!trackh.isValid()) continue;
  unsigned int ntrack = trackh->size();
  for(unsigned int i = 0; i < ntrack; ++i) {

    art::Ptr<recob::Track> ptrack(trackh, i);
    art::FindMany<recob::Hit>       fmhit(trackListHandle, evt, fTrackModuleLabel);
    std::vector<const recob::Hit*> hits = fmhit.at(i);


    const recob::Track& track = *ptrack;
    /*auto pcoll{ptrack};
	art::FindManyP<recob::SpacePoint> fs(pcoll, evt, fTrkSpptAssocModuleLabel);
	auto sppt = fs.at(0);
	art::FindManyP<recob::Hit> fh(sppt, evt, fHitSpptAssocModuleLabel);*/
        ////
        ///              figuring out which TPC
        ///
        ///
        //
        //        auto pcoll { ptrack };
        //art::FindManyP<recob::SpacePoint> fs( pcoll, evt, fTrkSpptAssocModuleLabel);
        //        auto sppt = fs.at(0);//.at(is);
        //        art::FindManyP<recob::Hit> fh( sppt, evt, fHitSpptAssocModuleLabel);
	//	auto hit = fh.at(0).at(0);
                                 	//auto hit = fmhit.at(0).at(0);
	/*	for(int ii=0;ii<hitlist->size())
	  {

	  }
	hit_trkid[i] = fmtk.at(i)[0]->ID();
	int hit_tpc=-1;
	if(hits.size()!=0)
	  {
	    geo::WireID tmpWireid=hits.at(0)->WireID();
	    hit_tpc=tmpWireid.TPC;
	    }
	  else hit_tpc=1; */
	int hit_tpc=1;


         ///
        ///
        //
        //
        //
        //
        //
        //
        /*        art::Handle< std::vector<recob::Hit> > hitListHandle;
        std::vector<art::Ptr<recob::Hit> > hitlist;
        if (evt.getByLabel(fHitsModuleLabel,hitListHandle))
        art::fill_ptr_vector(hitlist, hitListHandle);*/
        // Calculate the x offset due to nonzero reconstructed time.
        //double recotime = track.Time() * detprop->SamplingRate();       // nsec
        double recotime = 0.;
        double trackdx = recotime * 1.e-3 * larprop->DriftVelocity();  // cm
        // Fill histograms involving reco tracks only.
        int ntraj = track.NumberTrajectoryPoints();
        if(ntraj > 0) {
          TVector3 pos = track.Vertex();
          TVector3 dir = track.VertexDirection();
          TVector3 end = track.End();
          pos[0] += trackdx;
          end[0] += trackdx;
          double dpos = bdist(pos,hit_tpc);
          double dend = bdist(end,hit_tpc);
          double tlen = length(track);
          double theta_xz = std::atan2(dir.X(), dir.Z());
          double theta_yz = std::atan2(dir.Y(), dir.Z());
          if(fRecoHistMap.count(0) == 0)
            fRecoHistMap[0] = RecoHists("Reco");
          const RecoHists& rhists = fRecoHistMap[0];
          rhists.fHstartx->Fill(pos.X());
          rhists.fHstarty->Fill(pos.Y());
          rhists.fHstartz->Fill(pos.Z());
          rhists.fHstartd->Fill(dpos);
          rhists.fHendx->Fill(end.X());
          rhists.fHendy->Fill(end.Y());
          rhists.fHendz->Fill(end.Z());
          rhists.fHendd->Fill(dend);
          rhists.fHtheta->Fill(dir.Theta());
          rhists.fHphi->Fill(dir.Phi());
          rhists.fHtheta_xz->Fill(theta_xz);
          rhists.fHtheta_yz->Fill(theta_yz);
          double mom = 0.;
          if(track.NumberFitMomentum() > 0)
            mom = track.VertexMomentum();
          rhists.fHmom->Fill(mom);
          rhists.fHlen->Fill(tlen);
          // Id of matching mc particle.
          int mcid = -1;
          // Loop over direction.  
          for(int swap=0; swap<2; ++swap) {
            // Analyze reversed tracks only if start momentum = end momentum.
            if(swap != 0 && track.NumberFitMomentum() > 0 &&
               std::abs(track.VertexMomentum() - track.EndMomentum()) > 1.e-3)
              continue;
            // Calculate the global-to-local rotation matrix.
            TMatrixD rot(3,3);
            int start_point = (swap == 0 ? 0 : ntraj-1);
            track.GlobalToLocalRotationAtPoint(start_point, rot);
            // Update track data for reversed case.
            if(swap != 0) {
              rot(1, 0) = -rot(1, 0);
              rot(2, 0) = -rot(2, 0);
              rot(1, 1) = -rot(1, 1);
              rot(2, 1) = -rot(2, 1);
              rot(1, 2) = -rot(1, 2);
              rot(2, 2) = -rot(2, 2);
              pos = track.End();
              dir = -track.EndDirection();
              end = track.Vertex();
              pos[0] += trackdx;
              end[0] += trackdx;
              dpos = bdist(pos,hit_tpc);
              dend = bdist(end,hit_tpc);
              theta_xz = std::atan2(dir.X(), dir.Z());
              theta_yz = std::atan2(dir.Y(), dir.Z());
              if(track.NumberFitMomentum() > 0)
                mom = track.EndMomentum();
            }
	    
            // Get covariance matrix.
            //            const TMatrixD& cov = (swap == 0 ? track.VertexCovariance() : track.EndCovariance());
            // Loop over track-like mc particles.
            for(auto ipart = plist2.begin(); ipart != plist2.end(); ++ipart) {
              const simb::MCParticle* part = *ipart;
              if (!part)
                throw cet::exception("SeedAna") << "no particle! [II]\n";
              int pdg = part->PdgCode();
              if(fIgnoreSign) pdg = std::abs(pdg);
              auto iMCHistMap = fMCHistMap.find(pdg);
              if (iMCHistMap == fMCHistMap.end())
                throw cet::exception("SeedAna") << "no particle with ID=" << pdg << "\n";
              const MCHists& mchists = iMCHistMap->second;
              // Calculate the x offset due to nonzero mc particle time.
              double mctime = part->T();                                 // nsec
              double mcdx = mctime * 1.e-3 * larprop->DriftVelocity();   // cm
              // Calculate the points where this mc particle enters and leaves the
              // fiducial volume, and the length in the fiducial volume.
              TVector3 mcstart;
              TVector3 mcend;
              TVector3 mcstartmom;
              TVector3 mcendmom;
              double plen = length(*part, mcdx, mcstart, mcend, mcstartmom, mcendmom);
              // Get the displacement of this mc particle in the global coordinate system.
              TVector3 mcpos = mcstart - pos;
              // Rotate the momentum and position to the
              // track-local coordinate system.
              TVector3 mcmoml = rot * mcstartmom;
              TVector3 mcposl = rot * mcpos;
              double colinearity = mcmoml.Z() / mcmoml.Mag();
              double u = mcposl.X();
              double v = mcposl.Y();
              double w = mcposl.Z();
              double pu = mcmoml.X();
              double pv = mcmoml.Y();
              double pw = mcmoml.Z();
              double dudw = pu / pw;
              double dvdw = pv / pw;
              double u0 = u - w * dudw;
              double v0 = v - w * dvdw;
              double uv0 = std::sqrt(u0*u0 + v0*v0);
              mchists.fHduvcosth->Fill(colinearity, uv0);
              if(std::abs(uv0) < fMatchDisp) {
                // Fill slope matching histograms.
                mchists.fHmcdudw->Fill(dudw);
                mchists.fHmcdvdw->Fill(dvdw);
                //                mchists.fHdudwpull->Fill(dudw / std::sqrt(cov(2,2)));
                //                mchists.fHdvdwpull->Fill(dvdw / std::sqrt(cov(3,3)));
              }
              mchists.fHcosth->Fill(colinearity);
              if(colinearity > fMatchColinearity) {
                // Fill displacement matching histograms.
                mchists.fHmcu->Fill(u0);
                mchists.fHmcv->Fill(v0);
                mchists.fHmcw->Fill(w);
                //                mchists.fHupull->Fill(u0 / std::sqrt(cov(0,0)));
                //                mchists.fHvpull->Fill(v0 / std::sqrt(cov(1,1)));

                if(std::abs(uv0) < fMatchDisp) {
                  // Fill matching histograms.
                  double mctheta_xz = std::atan2(mcstartmom.X(), mcstartmom.Z());
                  double mctheta_yz = std::atan2(mcstartmom.Y(), mcstartmom.Z());
                  mchists.fHstartdx->Fill(pos.X() - mcstart.X());
                  mchists.fHstartdy->Fill(pos.Y() - mcstart.Y());
                  mchists.fHstartdz->Fill(pos.Z() - mcstart.Z());
                  mchists.fHenddx->Fill(end.X() - mcend.X());
                  mchists.fHenddy->Fill(end.Y() - mcend.Y());
                  mchists.fHenddz->Fill(end.Z() - mcend.Z());
                  mchists.fHlvsl->Fill(plen, tlen);
                  mchists.fHdl->Fill(tlen - plen);
                  mchists.fHpvsp->Fill(mcstartmom.Mag(), mom);
                  double dp = mom - mcstartmom.Mag();
                  mchists.fHdp->Fill(dp);
                  //                  mchists.fHppull->Fill(dp / std::sqrt(cov(4,4)));
                  if(std::abs(dpos) >= 5. && std::abs(dend) >= 5.) {
                    mchists.fHpvspc->Fill(mcstartmom.Mag(), mom);
                    mchists.fHdpc->Fill(dp);
                    //                    mchists.fHppullc->Fill(dp / std::sqrt(cov(4,4)));
                  }
                  // Count this track as well-reconstructed if it is matched to an
                  // mc particle (which it is if get here), and if in addition the
                  // starting position (w) matches and the reconstructed track length
                  // is more than 0.5 of the mc particle trajectory length.
                  bool good = std::abs(w) <= fWMatchDisp &&
                    tlen > 0.5 * plen;
                  if(good) {
                    mcid = part->TrackId();
                    mchists.fHgstartx->Fill(mcstart.X());
                    mchists.fHgstarty->Fill(mcstart.Y());
                    mchists.fHgstartz->Fill(mcstart.Z());
                    mchists.fHgendx->Fill(mcend.X());
                    mchists.fHgendy->Fill(mcend.Y());
                    mchists.fHgendz->Fill(mcend.Z());
                    mchists.fHgtheta->Fill(mcstartmom.Theta());
                    mchists.fHgphi->Fill(mcstartmom.Phi());
                    mchists.fHgtheta_xz->Fill(mctheta_xz);
                    mchists.fHgtheta_yz->Fill(mctheta_yz);
                    mchists.fHgmom->Fill(mcstartmom.Mag());
                    mchists.fHglen->Fill(plen);
                  }
                }
              }
            }

          }
          // Dump track information here.
          if(pdump) {
            TVector3 pos = track.Vertex();
            TVector3 dir = track.VertexDirection();
            TVector3 end = track.End();
            pos[0] += trackdx;
            end[0] += trackdx;
            TVector3 enddir = track.EndDirection();
            double pstart = track.VertexMomentum();
            double pend = track.EndMomentum();
            *pdump << "\nOffset"
                   << std::setw(3) << track.ID()
                   << std::setw(6) << mcid
                   << "  "
                   << std::fixed << std::setprecision(2) 
                   << std::setw(10) << trackdx
                   << "\nStart " 
                   << std::setw(3) << track.ID()
                   << std::setw(6) << mcid
                   << "  "
                   << std::fixed << std::setprecision(2) 
                   << std::setw(10) << pos[0]
                   << std::setw(10) << pos[1]
                   << std::setw(10) << pos[2];
            if(pstart > 0.) {
              *pdump << "  "
                     << std::fixed << std::setprecision(3) 
                     << std::setw(10) << dir[0]
                     << std::setw(10) << dir[1]
                     << std::setw(10) << dir[2];
            }
            else
              *pdump << std::setw(32) << " ";
            *pdump << std::setw(12) << std::fixed << std::setprecision(3) << pstart;
            *pdump << "\nEnd   " 
                   << std::setw(3) << track.ID()
                   << std::setw(6) << mcid
                   << "  "
                   << std::fixed << std::setprecision(2)
                   << std::setw(10) << end[0]
                   << std::setw(10) << end[1]
                   << std::setw(10) << end[2];
            if(pend > 0.01) {
              *pdump << "  " 
                     << std::fixed << std::setprecision(3) 
                     << std::setw(10) << enddir[0]
                     << std::setw(10) << enddir[1]
                     << std::setw(10) << enddir[2];
            }
            else 
              *pdump << std::setw(32)<< " ";
            *pdump << std::setw(12) << std::fixed << std::setprecision(3) << pend << "\n";
          }
        }
      }
  }// ipar
  
}


void AnaTree::AnaTree::beginJob()
{
  // Implementation of optional member function here.
  art::ServiceHandle<art::TFileService> tfs;
  fTree = tfs->make<TTree>("anatree","analysis tree");
  fTree->Branch("run",&run,"run/I");
  fTree->Branch("subrun",&subrun,"subrun/I");
  fTree->Branch("event",&event,"event/I");
  fTree->Branch("evttime",&evttime,"evttime/D");
  fTree->Branch("efield",efield,"efield[3]/D");
  fTree->Branch("t0",&t0,"t0/I");
  fTree->Branch("trigtime",trigtime,"trigtime[16]/I");
  fTree->Branch("ntracks_reco",&ntracks_reco,"ntracks_reco/I");
  fTree->Branch("trkstartx",trkstartx,"trkstartx[ntracks_reco]/D");
  fTree->Branch("trkstarty",trkstarty,"trkstarty[ntracks_reco]/D");
  fTree->Branch("trkstartz",trkstartz,"trkstartz[ntracks_reco]/D");
  fTree->Branch("trkendx",trkendx,"trkendx[ntracks_reco]/D");
  fTree->Branch("trkendy",trkendy,"trkendy[ntracks_reco]/D");
  fTree->Branch("trkendz",trkendz,"trkendz[ntracks_reco]/D");
fTree->Branch("trkstartx_MC",trkstartx_MC,"trkstartx_MC[ntracks_reco]/D");
  fTree->Branch("trkstarty_MC",trkstarty_MC,"trkstarty_MC[ntracks_reco]/D");
  fTree->Branch("trkstartz_MC",trkstartz_MC,"trkstartz_MC[ntracks_reco]/D");
  fTree->Branch("trkendx_MC",trkendx_MC,"trkendx_MC[ntracks_reco]/D");
  fTree->Branch("trkendy_MC",trkendy_MC,"trkendy_MC[ntracks_reco]/D");
  fTree->Branch("trkendz_MC",trkendz_MC,"trkendz_MC[ntracks_reco]/D");
  fTree->Branch("trklen_MC",trklen_MC,"trklen_MC[ntracks_reco]/D");
  fTree->Branch("trklen_cut_MC",trklen_cut_MC,"trklen_cut_MC[ntracks_reco]/D");
  fTree->Branch("trkmom",trkmom,"trkmom[ntracks_reco]/D");
  fTree->Branch("trkpdg",trkpdg,"trkpdg[ntracks_reco]/D");
  fTree->Branch("trkd2",trkd2,"trkd2[ntracks_reco]/D");
  fTree->Branch("trkcolin",trkcolin,"trkcolin[ntracks_reco]/D");
 fTree->Branch("trktheta_xz",trktheta_xz,"trktheta_xz[ntracks_reco]/D");
  fTree->Branch("trktheta_yz",trktheta_yz,"trktheta_yz[ntracks_reco]/D");
  fTree->Branch("trktheta",trktheta,"trktheta[ntracks_reco]/D");
  fTree->Branch("trkphi",trkphi,"trkphi[ntracks_reco]/D");


  fTree->Branch("trkstartdcosx",trkstartdcosx,"trkstartdcosx[ntracks_reco]/D");
  fTree->Branch("trkstartdcosy",trkstartdcosy,"trkstartdcosy[ntracks_reco]/D");
  fTree->Branch("trkstartdcosz",trkstartdcosz,"trkstartdcosz[ntracks_reco]/D");
  fTree->Branch("trklen",trklen,"trklen[ntracks_reco]/D");
  fTree->Branch("trklen_L",trklen_L,"trklen_L[ntracks_reco]/D");
  fTree->Branch("trkid",trkid,"trkid[ntracks_reco]/D");
  fTree->Branch("mcang_x",&mcang_x,"mcang_x/D");
  fTree->Branch("mcang_y",&mcang_y,"mcang_y/D");
  fTree->Branch("mcang_z",&mcang_z,"mcang_z/D");

  fTree->Branch("mcpos_x",&mcpos_x,"mcpos_x/D");
  fTree->Branch("mcpos_y",&mcpos_y,"mcpos_y/D");
  fTree->Branch("mcpos_z",&mcpos_z,"mcpos_z/D");

  fTree->Branch("trkang",trkang,"trkang[ntracks_reco]/D");
  fTree->Branch("trkcolinearity",trkcolinearity,"trkcolinearity[ntracks_reco]/D");
  fTree->Branch("trkmatchdisp",trkmatchdisp,"trkmatchdisp[ntracks_reco]/D");
  fTree->Branch("trkwmatchdisp",trkwmatchdisp,"trkwmatchdisp[ntracks_reco]/D");
  fTree->Branch("trklenratio",trklenratio,"trklenratio[ntracks_reco]/D");
  fTree->Branch("trkenddcosx",trkenddcosx,"trkenddcosx[ntracks_reco]/D");
  fTree->Branch("trkenddcosy",trkenddcosy,"trkenddcosy[ntracks_reco]/D");
  fTree->Branch("trkenddcosz",trkenddcosz,"trkenddcosz[ntracks_reco]/D");
  fTree->Branch("ntrkhits",ntrkhits,"ntrkhits[ntracks_reco]/I");
  fTree->Branch("trkx",trkx,"trkx[ntracks_reco][1000]/D");
  fTree->Branch("trky",trky,"trky[ntracks_reco][1000]/D");
  fTree->Branch("trkz",trkz,"trkz[ntracks_reco][1000]/D");
  fTree->Branch("trkpitch",trkpitch,"trkpitch[ntracks_reco][3]/D");
  fTree->Branch("nhits",&nhits,"nhits/I");
}

AnaTree::AnaTree::RecoHists::RecoHists():
    //
    // Purpose: Default constructor.
    //
    fHstartx(0),
    fHstarty(0),
    fHstartz(0),
    fHstartd(0),
    fHendx(0),
    fHendy(0),
    fHendz(0),
    fHendd(0),
    fHtheta(0),
    fHphi(0),
    fHtheta_xz(0),
    fHtheta_yz(0),
    fHmom(0),
    fHlen(0)
    ,fHHitChg(0)
    ,fHHitWidth(0)
    ,fHHitPdg(0)
    ,fHHitTrkId(0)
    ,fModeFrac(0)
    ,fNTrkIdTrks(0)
    ,fNTrkIdTrks2(0)
    ,fNTrkIdTrks3(0)
  {}
AnaTree::AnaTree::RecoHists::RecoHists(const std::string& subdir)
  //
  // Purpose: Initializing constructor.
  //
  {
    // Make sure all histogram pointers are initially zero.
    *this = RecoHists();
    // Get services.
    art::ServiceHandle<geo::Geometry> geom;
    art::ServiceHandle<art::TFileService> tfs;
    // Make histogram directory.
    art::TFileDirectory topdir = tfs->mkdir("trkana", "TrackAnaCT histograms");
    art::TFileDirectory dir = topdir.mkdir(subdir);
    // Book histograms.
    fHstartx = dir.make<TH1F>("xstart", "X Start Position",
                              100, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHstarty = dir.make<TH1F>("ystart", "Y Start Position",
                              100, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHstartz = dir.make<TH1F>("zstart", "Z Start Position",
                              100, 0., geom->Cryostat(0).Length());
    fHstartd = dir.make<TH1F>("dstart", "Start Position Distance to Boundary",
                              100, -10., geom->Cryostat(0).HalfWidth());
    fHendx = dir.make<TH1F>("xend", "X End Position",
                            100, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHendy = dir.make<TH1F>("yend", "Y End Position",
                            100, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHendz = dir.make<TH1F>("zend", "Z End Position",
                            100, 0., geom->Cryostat(0).Length());
    fHendd = dir.make<TH1F>("dend", "End Position Distance to Boundary",
                            100, -10., geom->Cryostat(0).HalfWidth());
    fHtheta = dir.make<TH1F>("theta", "Theta", 100, 0., 3.142);
    fHphi = dir.make<TH1F>("phi", "Phi", 100, -3.142, 3.142);
    fHtheta_xz = dir.make<TH1F>("theta_xz", "Theta_xz", 100, -3.142, 3.142);
    fHtheta_yz = dir.make<TH1F>("theta_yz", "Theta_yz", 100, -3.142, 3.142);
    fHmom = dir.make<TH1F>("mom", "Momentum", 100, 0., 10.);
    fHlen = dir.make<TH1F>("len", "Track Length", 100, 0., 3.0 * geom->Cryostat(0).Length());
    fHHitChg = dir.make<TH1F>("hchg", "Hit Charge (ADC counts)", 100, 0., 4000.);
    fHHitWidth = dir.make<TH1F>("hwid", "Hit Width (ticks)", 40, 0., 20.);
    fHHitPdg = dir.make<TH1F>("hpdg", "Hit Pdg code",5001, -2500.5, +2500.5);
    fHHitTrkId = dir.make<TH1F>("htrkid", "Hit Track ID", 401, -200.5, +200.5);
    fModeFrac = dir.make<TH1F>("hmodefrac", "quasi-Purity: Fraction of component tracks with the Track mode value", 20, 0.01, 1.01);
    fNTrkIdTrks = dir.make<TH1F>("hntrkids", "quasi-Efficiency: Number of stitched tracks in which TrkId appears", 20, 0., +10.0);
    fNTrkIdTrks2 = dir.make<TH2F>("hntrkids2", "Number of stitched tracks in which TrkId appears vs KE [GeV]", 20, 0., +10.0, 20, 0.0, 1.5);
    fNTrkIdTrks3 = dir.make<TH2F>("hntrkids3", "MC Track vs Reco Track, wtd by nhits on Collection Plane", 10, -0.5, 9.5, 10, -0.5, 9.5);
    fNTrkIdTrks3->GetXaxis()->SetTitle("Sorted-by-Descending-CPlane-Hits outer Track Number");
    fNTrkIdTrks3->GetYaxis()->SetTitle("Sorted-by-Descending-True-Length G4Track");
    
  }
  // MCHists methods.
  AnaTree::AnaTree::MCHists::MCHists() :
    //
    // Purpose: Default constructor.
    //
    fHduvcosth(0),
    fHcosth(0),
    fHmcu(0),
    fHmcv(0),
    fHmcw(0),
    fHupull(0),
    fHvpull(0),
    fHmcdudw(0),
    fHmcdvdw(0),
    fHdudwpull(0),
    fHdvdwpull(0),
    fHstartdx(0),
    fHstartdy(0),
    fHstartdz(0),
    fHenddx(0),
    fHenddy(0),
    fHenddz(0),
    fHlvsl(0),
    fHdl(0),
    fHpvsp(0),
    fHpvspc(0),
    fHdp(0),
    fHdpc(0),
    fHppull(0),
    fHppullc(0),
    fHmcstartx(0),
    fHmcstarty(0),
    fHmcstartz(0),
    fHmcendx(0),
    fHmcendy(0),
    fHmcendz(0),
    fHmctheta(0),
    fHmcphi(0),
    fHmctheta_xz(0),
    fHmctheta_yz(0),
    fHmcmom(0),
    fHmclen(0),
    fHgstartx(0),
    fHgstarty(0),
    fHgstartz(0),
    fHgendx(0),
    fHgendy(0),
    fHgendz(0),
    fHgtheta(0),
    fHgphi(0),
    fHgtheta_xz(0),
    fHgtheta_yz(0),
    fHgmom(0),
    fHglen(0),
    fHestartx(0),
    fHestarty(0),
    fHestartz(0),
    fHeendx(0),
    fHeendy(0),
    fHeendz(0),
    fHetheta(0),
    fHephi(0),
    fHetheta_xz(0),
    fHetheta_yz(0),
    fHemom(0),
    fHelen(0)
  {}
AnaTree::AnaTree::MCHists::MCHists(const std::string& subdir)
  //
  // Purpose: Initializing constructor.
  //
  {
    // Make sure all histogram pointers are initially zero.
    *this = MCHists();
    // Get services.
    art::ServiceHandle<geo::Geometry> geom;
    art::ServiceHandle<art::TFileService> tfs;
    // Make histogram directory.
    art::TFileDirectory topdir = tfs->mkdir("trkana", "TrackAnaCT histograms");
    art::TFileDirectory dir = topdir.mkdir(subdir);
    // Book histograms.
    fHduvcosth = dir.make<TH2F>("duvcosth", "Delta(uv) vs. Colinearity", 
                                100, 0.95, 1., 100, 0., 1.);
    fHcosth = dir.make<TH1F>("colin", "Colinearity", 100, 0.95, 1.);
    fHmcu = dir.make<TH1F>("mcu", "MC Truth U", 100, -5., 5.);
    fHmcv = dir.make<TH1F>("mcv", "MC Truth V", 100, -5., 5.);
    fHmcw = dir.make<TH1F>("mcw", "MC Truth W", 100, -20., 20.);
    fHupull = dir.make<TH1F>("dupull", "U Pull", 100, -20., 20.);
    fHvpull = dir.make<TH1F>("dvpull", "V Pull", 100, -20., 20.);
    fHmcdudw = dir.make<TH1F>("mcdudw", "MC Truth U Slope", 100, -0.2, 0.2);
    fHmcdvdw = dir.make<TH1F>("mcdvdw", "MV Truth V Slope", 100, -0.2, 0.2);
    fHdudwpull = dir.make<TH1F>("dudwpull", "U Slope Pull", 100, -10., 10.);
    fHdvdwpull = dir.make<TH1F>("dvdwpull", "V Slope Pull", 100, -10., 10.);
    fHstartdx = dir.make<TH1F>("startdx", "Start Delta x", 100, -10., 10.);
    fHstartdy = dir.make<TH1F>("startdy", "Start Delta y", 100, -10., 10.);
    fHstartdz = dir.make<TH1F>("startdz", "Start Delta z", 100, -10., 10.);
    fHenddx = dir.make<TH1F>("enddx", "End Delta x", 100, -10., 10.);
    fHenddy = dir.make<TH1F>("enddy", "End Delta y", 100, -10., 10.);
    fHenddz = dir.make<TH1F>("enddz", "End Delta z", 100, -10., 10.);
    fHlvsl = dir.make<TH2F>("lvsl", "Reco Length vs. MC Truth Length",
                            100, 0., 1.1 * geom->Cryostat(0).Length(), 100, 0., 1.1 * geom->Cryostat(0).Length());
    fHdl = dir.make<TH1F>("dl", "Track Length Minus MC Particle Length", 100, -50., 50.);
    fHpvsp = dir.make<TH2F>("pvsp", "Reco Momentum vs. MC Truth Momentum",
                            100, 0., 5., 100, 0., 5.);
    fHpvspc = dir.make<TH2F>("pvspc", "Reco Momentum vs. MC Truth Momentum (Contained Tracks)",
                             100, 0., 5., 100, 0., 5.);
    fHdp = dir.make<TH1F>("dp", "Reco-MC Momentum Difference", 100, -5., 5.);
    fHdpc = dir.make<TH1F>("dpc", "Reco-MC Momentum Difference (Contained Tracks)",
                           100, -5., 5.);
    fHppull = dir.make<TH1F>("ppull", "Momentum Pull", 100, -10., 10.);
    fHppullc = dir.make<TH1F>("ppullc", "Momentum Pull (Contained Tracks)", 100, -10., 10.);
    fHmcstartx = dir.make<TH1F>("mcxstart", "MC X Start Position",
                                10, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHmcstarty = dir.make<TH1F>("mcystart", "MC Y Start Position",
                                10, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHmcstartz = dir.make<TH1F>("mczstart", "MC Z Start Position",
                                10, 0., geom->Cryostat(0).Length());
    fHmcendx = dir.make<TH1F>("mcxend", "MC X End Position",
                              10, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHmcendy = dir.make<TH1F>("mcyend", "MC Y End Position",
                              10, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHmcendz = dir.make<TH1F>("mczend", "MC Z End Position",
                              10, 0., geom->Cryostat(0).Length());
    fHmctheta = dir.make<TH1F>("mctheta", "MC Theta", 40, 0., 3.142);
    fHmcphi = dir.make<TH1F>("mcphi", "MC Phi", 40, -3.142, 3.142);
    fHmctheta_xz = dir.make<TH1F>("mctheta_xz", "MC Theta_xz", 40, -3.142, 3.142);
    fHmctheta_yz = dir.make<TH1F>("mctheta_yz", "MC Theta_yz", 40, -3.142, 3.142);
    fHmcmom = dir.make<TH1F>("mcmom", "MC Momentum", 10, 0., 10.);
    fHmclen = dir.make<TH1F>("mclen", "MC Particle Length", 10, 0., 1.1 * geom->Cryostat(0).Length());
    fHgstartx = dir.make<TH1F>("gxstart", "Good X Start Position",
                               10, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHgstarty = dir.make<TH1F>("gystart", "Good Y Start Position",
                               10, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHgstartz = dir.make<TH1F>("gzstart", "Good Z Start Position",
                               10, 0., geom->Cryostat(0).Length());
    fHgendx = dir.make<TH1F>("gxend", "Good X End Position",
                             10, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHgendy = dir.make<TH1F>("gyend", "Good Y End Position",
                             10, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHgendz = dir.make<TH1F>("gzend", "Good Z End Position",
                             10, 0., geom->Cryostat(0).Length());
    fHgtheta = dir.make<TH1F>("gtheta", "Good Theta", 40, 0., 3.142);
    fHgphi = dir.make<TH1F>("gphi", "Good Phi", 40, -3.142, 3.142);
    fHgtheta_xz = dir.make<TH1F>("gtheta_xz", "Good Theta_xz", 40, -3.142, 3.142);
    fHgtheta_yz = dir.make<TH1F>("gtheta_yz", "Good Theta_yz", 40, -3.142, 3.142);
    fHgmom = dir.make<TH1F>("gmom", "Good Momentum", 10, 0., 10.);
    fHglen = dir.make<TH1F>("glen", "Good Particle Length", 10, 0., 1.1 * geom->Cryostat(0).Length());
    fHestartx = dir.make<TH1F>("exstart", "Efficiency vs. X Start Position",
                               10, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHestarty = dir.make<TH1F>("eystart", "Efficiency vs. Y Start Position",
                               10, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHestartz = dir.make<TH1F>("ezstart", "Efficiency vs. Z Start Position",
                               10, 0., geom->Cryostat(0).Length());
    fHeendx = dir.make<TH1F>("exend", "Efficiency vs. X End Position",
                             10, -2.*geom->Cryostat(0).HalfWidth(), 4.*geom->Cryostat(0).HalfWidth());
    fHeendy = dir.make<TH1F>("eyend", "Efficiency vs. Y End Position",
                             10, -geom->Cryostat(0).HalfHeight(), geom->Cryostat(0).HalfHeight());
    fHeendz = dir.make<TH1F>("ezend", "Efficiency vs. Z End Position",
                             10, 0., geom->Cryostat(0).Length());
    fHetheta = dir.make<TH1F>("etheta", "Efficiency vs. Theta", 40, 0., 3.142);
    fHephi = dir.make<TH1F>("ephi", "Efficiency vs. Phi", 40, -3.142, 3.142);
    fHetheta_xz = dir.make<TH1F>("etheta_xz", "Efficiency vs. Theta_xz", 40, -3.142, 3.142);
    fHetheta_yz = dir.make<TH1F>("etheta_yz", "Efficiency vs. Theta_yz", 40, -3.142, 3.142);
    fHemom = dir.make<TH1F>("emom", "Efficiency vs. Momentum", 10, 0., 10.);
    fHelen = dir.make<TH1F>("elen", "Efficiency vs. Particle Length",
    10, 0., 1.1 * geom->Cryostat(0).Length());
  }
//void AnaTree::AnaTree::reconfigure(fhicl::ParameterSet const & p)
//{
//  // Implementation of optional member function here.
//}
void AnaTree::AnaTree::ResetVars(){

  run = -99999;
  subrun = -99999;
  event = -99999;
  evttime = -99999;
  for (int i = 0; i<3; ++i){
    efield[i] = -99999;
  }
  t0 = -99999;
  ntracks_reco = -99999;
  mcang_x = -99999;
  mcang_y = -99999;
  mcang_z = -99999;

  mcpos_x = -99999;
  mcpos_y = -99999;
  mcpos_z = -99999;


  for (int i = 0; i < kMaxTrack; ++i){
    trkstartx[i] = -99999;
    trkstarty[i] = -99999;
    trkstartz[i] = -99999;
    trkendx[i] = -99999;
    trkendy[i] = -99999;
    trkendz[i] = -99999;
  trkstartx_MC[i] = -99999;
    trkstarty_MC[i] = -99999;
    trkstartz_MC[i] = -99999;
    trkendx_MC[i] = -99999;
    trkendy_MC[i] = -99999;
    trkendz_MC[i] = -99999;
    trklen_MC[i] = -99999;
    trklen_cut_MC[i] = -99999;
    trkmom[i] = -99999;
    trkpdg[i] = -99999;
    trkd2[i] = -99999;
    trkcolin[i] = -99999;
    trktheta_xz[i] = -99999;
    trktheta_yz[i] = -99999;
    trktheta[i] = -99999;
    trkphi[i] = -99999;

    trkstartdcosx[i] = -99999;
    trkstartdcosy[i] = -99999;
    trkstartdcosz[i] = -99999;
    trklen[i] = -99999;
    trklen_L[i] = -99999;
    trkid[i] = -99999;
    trkang[i] = -99999;
    trkcolinearity[i] = -99999;
    trkmatchdisp[i] = -99999;
    trkwmatchdisp[i] = -99999;
    trklenratio[i] = -99999;


    trkenddcosx[i] = -99999;
    trkenddcosy[i] = -99999;
    trkenddcosz[i] = -99999;
    ntrkhits[i] = -99999;
    for (int j = 0; j<kMaxTrackHits; ++j){
      trkx[i][j] = -99999;
      trky[i][j] = -99999;
      trkz[i][j] = -99999;
    }
    for (int j = 0; j<3; ++j){
      trkpitch[i][j] = -99999;
    }
  }
  nhits = -99999;
}

void AnaTree::AnaTree::endJob()
  //
  // Purpose: End of job.
  //
  {

    // Fill efficiency histograms.
    for(std::map<int, MCHists>::const_iterator i = fMCHistMap.begin();
        i != fMCHistMap.end(); ++i) {
      const MCHists& mchists = i->second;
      effcalc(mchists.fHgstartx, mchists.fHmcstartx, mchists.fHestartx);
      effcalc(mchists.fHgstarty, mchists.fHmcstarty, mchists.fHestarty);
      effcalc(mchists.fHgstartz, mchists.fHmcstartz, mchists.fHestartz);
      effcalc(mchists.fHgendx, mchists.fHmcendx, mchists.fHeendx);
      effcalc(mchists.fHgendy, mchists.fHmcendy, mchists.fHeendy);
      effcalc(mchists.fHgendz, mchists.fHmcendz, mchists.fHeendz);
      effcalc(mchists.fHgtheta, mchists.fHmctheta, mchists.fHetheta);
      effcalc(mchists.fHgphi, mchists.fHmcphi, mchists.fHephi);
      effcalc(mchists.fHgtheta_xz, mchists.fHmctheta_xz, mchists.fHetheta_xz);
      effcalc(mchists.fHgtheta_yz, mchists.fHmctheta_yz, mchists.fHetheta_yz);
      effcalc(mchists.fHgmom, mchists.fHmcmom, mchists.fHemom);
      effcalc(mchists.fHglen, mchists.fHmclen, mchists.fHelen);
    }
  }


DEFINE_ART_MODULE(AnaTree::AnaTree)