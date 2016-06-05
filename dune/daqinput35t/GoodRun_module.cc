////////////////////////////////////////////////////////////////////////
// Class:       GoodRun
// Module Type: Analyzer
// File:        GoodRun_module.cc
//
// Module written by Karl Warburton to compare the timings for RCE, SSP
//  and PTB data streams. 
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "lardataobj/RawData/ExternalTrigger.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/raw.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/RecoBase/OpHit.h"

#include "larevt/CalibrationDBI/Interface/ChannelStatusService.h"
#include "larevt/CalibrationDBI/Interface/ChannelStatusProvider.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include "TTree.h"
#include "TH2D.h"

const short unsigned int MaxSamples  = 2048;
const unsigned int MaxChannels = 2048;

namespace DAQToOffline {
  class GoodRun;
}

class DAQToOffline::GoodRun : public art::EDAnalyzer {
public:
  explicit GoodRun(fhicl::ParameterSet const & p);
  virtual ~GoodRun();

  void analyze(art::Event const & evt) override;
  void beginJob();
  void beginRun(const art::Run& r);
  void endRun(const art::Run& r);
  void endJob();
  void printParameterSet();

private:

  void FillArray( float ADCs[MaxSamples], unsigned int NSamples, raw::RawDigit digit );

  // Information for good run list histogram
  TTree* fTree;
  int RunNumber;
  int TotEvents;
  int NumberOfRCEs;
  double FracRCEs;
  double AvADCPedDiff;
  double FracWaveforms;
  double FracOpHits;
  int nPTBTrigsOn110;
  int nPTBTrigsOn111;
  int nPTBTrigsOn112;
  int nPTBTrigsOn113;
  int nPTBTrigsOn114;
  int nPTBTrigsOn115;
  int EvJustSSPs;

  //TH2D* ADCDiff;
  //TH2D* ADCPed;

  
  TTree* FlatTree;
  int Event;
  unsigned int DigSize;
  unsigned int NSamples;
  unsigned int Channel[2048];
  float ADCs_Channel_10,   ADCs_Channel_70,   ADCs_Channel_110;
  float ADCs_Channel_160,  ADCs_Channel_205,  ADCs_Channel_260;
  float ADCs_Channel_300,  ADCs_Channel_410,  ADCs_Channel_500;
  float ADCs_Channel_525,  ADCs_Channel_585,  ADCs_Channel_625;
  float ADCs_Channel_675,  ADCs_Channel_720,  ADCs_Channel_775;
  float ADCs_Channel_815,  ADCs_Channel_915,  ADCs_Channel_1015;
  float ADCs_Channel_1040, ADCs_Channel_1095, ADCs_Channel_1135;
  float ADCs_Channel_1185, ADCs_Channel_1230, ADCs_Channel_1285;
  float ADCs_Channel_1325, ADCs_Channel_1435, ADCs_Channel_1525;
  float ADCs_Channel_1545, ADCs_Channel_1605, ADCs_Channel_1645;
  float ADCs_Channel_1695, ADCs_Channel_1740, ADCs_Channel_1795;
  float ADCs_Channel_1835, ADCs_Channel_1945, ADCs_Channel_2035;

  bool fMakeFlatTree;
  std::string fCounterModuleLabel, fWaveformModuleLabel, fRawDigitModuleLabel, fOpHitModuleLabel;
  
  std::vector<int> MyUsefulChans;
};

DAQToOffline::GoodRun::GoodRun(fhicl::ParameterSet const & pset)
  : EDAnalyzer(pset)
  , fMakeFlatTree       (pset.get<bool>("MakeFlatTree"))
    //---------------------------RCE--------------------------------------
  , fCounterModuleLabel (pset.get<std::string>("CounterModuleLabel"))
  , fWaveformModuleLabel(pset.get<std::string>("WaveformModuleLabel"))
  , fRawDigitModuleLabel(pset.get<std::string>("RawDigitModuleLabel"))
  , fOpHitModuleLabel   (pset.get<std::string>("OpHitModuleLabel"))
    //---------------------------SSP--------------------------------------
//  ,fSSPFragType        ( pset.get<std::string>("SSPFragType"))
//  ,fSSPRawDataLabel    ( pset.get<std::string>("SSPRawDataLabel"))
    //---------------------------PTB--------------------------------------
//  ,fPTBFragType        ( pset.get<std::string>("PTBFragType"))
//  ,fPTBRawDataLabel    ( pset.get<std::string>("PTBRawDataLabel"))
    //--------------------------------------------------------------------
{
}

DAQToOffline::GoodRun::~GoodRun() {
}

void DAQToOffline::GoodRun::beginJob() {
  
  art::ServiceHandle<art::TFileService> tfs;
    
  if (!fMakeFlatTree) {
    fTree = tfs->make<TTree>("RunList","RunList Information");
    fTree->Branch("RunNumber"     ,&RunNumber     );
    fTree->Branch("TotEvents"     ,&TotEvents     );
    fTree->Branch("NumberOfRCEs"  ,&NumberOfRCEs  );
    fTree->Branch("FracRCEs"      ,&FracRCEs      );
    fTree->Branch("AvADCPedDiff"  ,&AvADCPedDiff  );
    fTree->Branch("FracWaveforms" ,&FracWaveforms );
    fTree->Branch("FracOpHits"    ,&FracOpHits    );
    fTree->Branch("nPTBTrigsOn110",&nPTBTrigsOn110);
    fTree->Branch("nPTBTrigsOn111",&nPTBTrigsOn111);
    fTree->Branch("nPTBTrigsOn112",&nPTBTrigsOn112);
    fTree->Branch("nPTBTrigsOn113",&nPTBTrigsOn113);
    fTree->Branch("nPTBTrigsOn114",&nPTBTrigsOn114);
    fTree->Branch("nPTBTrigsOn115",&nPTBTrigsOn115);
    fTree->Branch("EvJustSSPs"    ,&EvJustSSPs    );
    //ADCDiff = tfs->make<TH2D>("ADCDiff","Difference in average ADC value and pedestal; Channel number; Difference (ADCs)", 2049,0,2048, 400,-50,50);
    //ADCPed  = tfs->make<TH2D>("ADCPed" ,"Comparison of Average ADC values, and pedestal; Average ADC; Pedestal", 1200, 400, 1000, 1200, 400, 1000);

  } else {

    // I need to choose which channels I want to look at.....
    // Runs 18407 - 18411 used RCEs 0, 4,     12,     15
    // Runs 18412 - 18422 used RCEs 0, 4, 11, 12, 13, 15
    // Runs 18423 - 18430 used RCEs 0, 4, 11

    // To convert from the RCE number to the channels I want to store, look at Mikes detailed channel map here:
    //   https://cdcvs.fnal.gov/redmine/projects/35ton/wiki/Channel_map
    FlatTree = tfs->make<TTree>("FlatDigitTree","FlatDigitTree");
    FlatTree->Branch("Event"   ,&Event   ,"Event/I"   );
    FlatTree->Branch("NSamples",&NSamples,"NSamples/I");
    FlatTree->Branch("ADCs_Channel_10"  ,  &ADCs_Channel_10  , "ADCs_Channel_10/F");
    FlatTree->Branch("ADCs_Channel_70"  ,  &ADCs_Channel_70  , "ADCs_Channel_70/F");
    FlatTree->Branch("ADCs_Channel_110" ,  &ADCs_Channel_110 , "ADCs_Channel_110/F");

    FlatTree->Branch("ADCs_Channel_160" ,  &ADCs_Channel_160 , "ADCs_Channel_160/F");
    FlatTree->Branch("ADCs_Channel_205" ,  &ADCs_Channel_205 , "ADCs_Channel_205/F");
    FlatTree->Branch("ADCs_Channel_260" ,  &ADCs_Channel_260 , "ADCs_Channel_260/F");

    FlatTree->Branch("ADCs_Channel_300" ,  &ADCs_Channel_300 , "ADCs_Channel_300/F");
    FlatTree->Branch("ADCs_Channel_410" ,  &ADCs_Channel_410 , "ADCs_Channel_410/F");
    FlatTree->Branch("ADCs_Channel_500" ,  &ADCs_Channel_500 , "ADCs_Channel_500/F");

    FlatTree->Branch("ADCs_Channel_525" ,  &ADCs_Channel_525 , "ADCs_Channel_525/F");
    FlatTree->Branch("ADCs_Channel_585" ,  &ADCs_Channel_585 , "ADCs_Channel_585/F");
    FlatTree->Branch("ADCs_Channel_625" ,  &ADCs_Channel_625 , "ADCs_Channel_625/F");

    FlatTree->Branch("ADCs_Channel_675" ,  &ADCs_Channel_675 , "ADCs_Channel_675/F");
    FlatTree->Branch("ADCs_Channel_720" ,  &ADCs_Channel_720 , "ADCs_Channel_720/F");
    FlatTree->Branch("ADCs_Channel_775" ,  &ADCs_Channel_775 , "ADCs_Channel_775/F");

    FlatTree->Branch("ADCs_Channel_815" ,  &ADCs_Channel_815 , "ADCs_Channel_815/F");
    FlatTree->Branch("ADCs_Channel_915" ,  &ADCs_Channel_915 , "ADCs_Channel_915/F");
    FlatTree->Branch("ADCs_Channel_1015",  &ADCs_Channel_1015, "ADCs_Channel_1015/F");

    FlatTree->Branch("ADCs_Channel_1040",  &ADCs_Channel_1040, "ADCs_Channel_1040/F");
    FlatTree->Branch("ADCs_Channel_1095",  &ADCs_Channel_1095, "ADCs_Channel_1095/F");
    FlatTree->Branch("ADCs_Channel_1135",  &ADCs_Channel_1135, "ADCs_Channel_1135/F");

    FlatTree->Branch("ADCs_Channel_1185",  &ADCs_Channel_1185, "ADCs_Channel_1185/F");
    FlatTree->Branch("ADCs_Channel_1230",  &ADCs_Channel_1230, "ADCs_Channel_1230/F");
    FlatTree->Branch("ADCs_Channel_1285",  &ADCs_Channel_1285, "ADCs_Channel_1285/F");

    FlatTree->Branch("ADCs_Channel_1325",  &ADCs_Channel_1325, "ADCs_Channel_1325/F");
    FlatTree->Branch("ADCs_Channel_1435",  &ADCs_Channel_1435, "ADCs_Channel_1435/F");
    FlatTree->Branch("ADCs_Channel_1525",  &ADCs_Channel_1525, "ADCs_Channel_1525/F");

    FlatTree->Branch("ADCs_Channel_1545",  &ADCs_Channel_1545, "ADCs_Channel_1545/F");
    FlatTree->Branch("ADCs_Channel_1605",  &ADCs_Channel_1605, "ADCs_Channel_1605/F");
    FlatTree->Branch("ADCs_Channel_1645",  &ADCs_Channel_1645, "ADCs_Channel_1645/F");

    FlatTree->Branch("ADCs_Channel_1695",  &ADCs_Channel_1695, "ADCs_Channel_1695/F");
    FlatTree->Branch("ADCs_Channel_1740",  &ADCs_Channel_1740, "ADCs_Channel_1740/F");
    FlatTree->Branch("ADCs_Channel_1795",  &ADCs_Channel_1795, "ADCs_Channel_1795/F");

    FlatTree->Branch("ADCs_Channel_1835",  &ADCs_Channel_1835, "ADCs_Channel_1835/F");
    FlatTree->Branch("ADCs_Channel_1945",  &ADCs_Channel_1945, "ADCs_Channel_1945/F");
    FlatTree->Branch("ADCs_Channel_2035",  &ADCs_Channel_2035, "ADCs_Channel_2035/F");
  }
}

void DAQToOffline::GoodRun::beginRun(const art::Run & r) {

  RunNumber = TotEvents = NumberOfRCEs = FracRCEs = AvADCPedDiff = FracWaveforms = FracOpHits = nPTBTrigsOn110 = nPTBTrigsOn111 = nPTBTrigsOn112 = nPTBTrigsOn113 = nPTBTrigsOn114 = nPTBTrigsOn115 = EvJustSSPs = 0;
  std::cout << "At the start of the run....going to reset all the variables. " << std::endl;

}

void DAQToOffline::GoodRun::analyze(art::Event const & evt)
{
  if (TotEvents==0) {
    RunNumber = evt.run();
    std::cout << "Got runNumber it is " << RunNumber << std::endl;
  }
  ++TotEvents;

  // get raw::RawDigits
  art::Handle< std::vector< raw::RawDigit> > externalRawDigitListHandle;
  std::vector< art::Ptr< raw::RawDigit> > digits;
  if (evt.getByLabel(fRawDigitModuleLabel, externalRawDigitListHandle) )
    art::fill_ptr_vector(digits,externalRawDigitListHandle);
  
  if (fMakeFlatTree) {
    Event    = evt.event();
    DigSize  = digits.size();
    NSamples = std::min( digits[0]->Samples(), MaxSamples );
    unsigned int Samp=0;
    while ( Samp != NSamples ) {
      for (unsigned int dig=0; dig<DigSize; ++dig ) {
	if      ( digits[dig]->Channel() == 10   ) ADCs_Channel_10   = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 70   ) ADCs_Channel_70   = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 110  ) ADCs_Channel_110  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	
	else if ( digits[dig]->Channel() == 160  ) ADCs_Channel_160  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 205  ) ADCs_Channel_205  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 260  ) ADCs_Channel_260  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 300  ) ADCs_Channel_300  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 410  ) ADCs_Channel_410  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 500  ) ADCs_Channel_500  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 525  ) ADCs_Channel_525  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 585  ) ADCs_Channel_585  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 625  ) ADCs_Channel_625  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 675  ) ADCs_Channel_675  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 720  ) ADCs_Channel_720  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 775  ) ADCs_Channel_775  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 815  ) ADCs_Channel_815  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 915  ) ADCs_Channel_915  = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1015 ) ADCs_Channel_1015 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 1040 ) ADCs_Channel_1040 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1095 ) ADCs_Channel_1095 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1135 ) ADCs_Channel_1135 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 1185 ) ADCs_Channel_1185 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1230 ) ADCs_Channel_1230 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1285 ) ADCs_Channel_1285 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 1325 ) ADCs_Channel_1325 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1435 ) ADCs_Channel_1435 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1525 ) ADCs_Channel_1525 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 1545 ) ADCs_Channel_1545 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1605 ) ADCs_Channel_1605 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1645 ) ADCs_Channel_1645 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 1695 ) ADCs_Channel_1695 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1740 ) ADCs_Channel_1740 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1795 ) ADCs_Channel_1795 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();

	else if ( digits[dig]->Channel() == 1835 ) ADCs_Channel_1835 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 1945 ) ADCs_Channel_1945 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
	else if ( digits[dig]->Channel() == 2035 ) ADCs_Channel_2035 = digits[dig]->ADC(Samp+1) - digits[dig]->GetPedestal();
      } // Loop through Digits.
      ++Samp;
      FlatTree->Fill();
    } // While have less Samples than I want.
  } else {
    // get raw::ExternalTriggers
    art::Handle< std::vector< raw::ExternalTrigger> > externalTriggerListHandle;
    std::vector< art::Ptr< raw::ExternalTrigger> > trigs;
    if (evt.getByLabel(fCounterModuleLabel, externalTriggerListHandle) )
      art::fill_ptr_vector(trigs,externalTriggerListHandle);

    // get raw::OpDetWaveforms
    art::Handle< std::vector< raw::OpDetWaveform> > externalWaveformListHandle;
    std::vector< art::Ptr< raw::OpDetWaveform> > waveforms;
    if (evt.getByLabel(fWaveformModuleLabel, externalWaveformListHandle) )
      art::fill_ptr_vector(waveforms,externalWaveformListHandle);

    // get recob::OpHits
    art::Handle< std::vector< recob::OpHit> > externalOpHitListHandle;
    std::vector< art::Ptr< recob::OpHit> > ophits;
    if (evt.getByLabel(fOpHitModuleLabel, externalOpHitListHandle) )
      art::fill_ptr_vector(ophits,externalOpHitListHandle);

    bool RCEsPresent = false;
    //bool PTBPresent  = false;
    bool WavePresent = false;
    //bool OHitPresent = false;

    // RCEs
    if (digits.size()) {
      RCEsPresent = true;
      ++FracRCEs;
      if (NumberOfRCEs == 0) {
	NumberOfRCEs = digits.size() / 128;
      
	//GET THE LIST OF BAD CHANNELS.
	lariov::ChannelStatusProvider const& channelStatus = art::ServiceHandle<lariov::ChannelStatusService>()->GetProvider();
	lariov::ChannelStatusProvider::ChannelSet_t const BadChannels = channelStatus.BadChannels();
      
	for (size_t dig=0; dig<digits.size(); ++dig) {
	  double AvADC = 0;
	  unsigned int Channel = digits[dig]->Channel();
	  double Pedestal = digits[dig]->GetPedestal();
	  for (size_t samp=0; samp<digits[dig]->Samples(); ++samp)
	    AvADC += digits[dig]->ADC(samp);
	  AvADC = AvADC / digits[dig]->Samples();
	  // Do I want to use this channel? Both AvADC and Pedestal, plus not a 'bad' channel...
	  bool UseChan = true;
	  if ( AvADC == 0 && Pedestal == 0)
	    UseChan = false;
	  for (auto it = BadChannels.begin(); it != BadChannels.end(); it++) {
	    if(Channel==*it) {
	      UseChan = false;
	      break;
	    }
	  } // Loop through bad chans.
	  if ( UseChan ) {
	    double ADCPedDiff = AvADC - Pedestal;
	    //ADCDiff->Fill( Channel, ADCPedDiff );
	    //ADCPed ->Fill( AvADC  , Pedestal   );
	    AvADCPedDiff += fabs(ADCPedDiff);
	  } // If have data for this tick, ie not turned off.
	} // Loop over digits
	AvADCPedDiff = AvADCPedDiff / digits.size();
      } // NumberOfRCEs == 0
    }
  
    // PTB
    if (trigs.size()) {
      //PTBPresent = true;
      for (size_t tr=0; tr<trigs.size(); ++tr) {
	if (trigs[tr]->GetTrigID() < 100) continue;
	if (trigs[tr]->GetTrigID() == 110) ++nPTBTrigsOn110;
	if (trigs[tr]->GetTrigID() == 111) ++nPTBTrigsOn111;
	if (trigs[tr]->GetTrigID() == 112) ++nPTBTrigsOn112;
	if (trigs[tr]->GetTrigID() == 113) ++nPTBTrigsOn113;
	if (trigs[tr]->GetTrigID() == 114) ++nPTBTrigsOn114;
	if (trigs[tr]->GetTrigID() == 115) ++nPTBTrigsOn115;
      }
    }

    // Waveforms
    if (waveforms.size()) {
      WavePresent = true;
      ++FracWaveforms;
      if (!RCEsPresent) ++EvJustSSPs;
    }

    // OpHits
    if (ophits.size()) {
      //OHitPresent = true;
      ++FracOpHits;
      if (!RCEsPresent && !WavePresent) ++EvJustSSPs;
    }
  }
}

void DAQToOffline::GoodRun::endRun(const art::Run & r)
{
  if (!fMakeFlatTree) {
    FracRCEs      = FracRCEs / TotEvents;
    FracWaveforms = FracWaveforms / TotEvents;
    FracOpHits    = FracOpHits / TotEvents;
    std::cout << "At the end of the run, I am putting the following into the TTree:\n"
	      << RunNumber << " " << TotEvents << " " << NumberOfRCEs << " " << FracRCEs << " " << AvADCPedDiff << " " << FracWaveforms << " " << FracOpHits << " "
	      << nPTBTrigsOn110 << " " << nPTBTrigsOn111 << " " << nPTBTrigsOn112 << " " << nPTBTrigsOn113 << " " << nPTBTrigsOn114 << " " << nPTBTrigsOn115 << " " << EvJustSSPs
	      << std::endl;
    fTree->Fill();
  }
}

void DAQToOffline::GoodRun::endJob() {
}

DEFINE_ART_MODULE(DAQToOffline::GoodRun)
