/*
    Input source driver for ProtoDUNE-DP raw data
    

 */

#include "art/Framework/IO/Sources/put_product_in_principal.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Provenance/SubRunID.h"	
#include "lardataobj/RawData/ExternalTrigger.h"
#include "canvas/Utilities/Exception.h"

#include "PDDPRawInputDriver.h"

#include <exception>
#include <thread>
#include <mutex>
#include <regex>


#define CHECKBYTEBIT(var, pos) ( (var) & (1<<pos) )
#define DCBITFLAG 0x6 // 0x0 LSB -> 0x7 MSB
#define GETDCFLAG(info) (CHECKBYTEBIT(info, DCBITFLAG)>0)

// event data quality flag
#define EVDQFLAG(info) ( (info & 0x3F ) == 0 )


//
//
//
namespace 
{
  void unpackCroData( const char *buf, size_t nb, bool cflag, unsigned nsa, adcbuf_t &data )
  {
    //data.clear();
    if( !cflag ) // unpack the uncompressed data into RawDigit
      {
	data.push_back( raw::RawDigit::ADCvector_t(nsa) );
	size_t sz = 0;
	const BYTE* start = buf;
	const BYTE* stop  = start + nb;
	while(start!=stop)
	  {
	    BYTE v1 = *start++;
	    BYTE v2 = *start++;
	    BYTE v3 = *start++;
	    
	    uint16_t tmp1 = ((v1 << 4) + ((v2 >> 4) & 0xf)) & 0xfff;
	    uint16_t tmp2 = (((v2 & 0xf) << 8 ) + (v3 & 0xff)) & 0xfff;

	    if( sz == nsa ){ data.push_back(raw::RawDigit::ADCvector_t(nsa)); sz = 0; }
	    data.back()[sz++] = (short)tmp1;
	    
	    if( sz == nsa ){ data.push_back(raw::RawDigit::ADCvector_t(nsa)); sz = 0; }
	    data.back()[sz++] = (short)tmp2;
	  }
      }
    else
      //TODO finalize the format of compressed data
      //     the data for each channel should be preceeded by size in words
      {
	// should not happen ...
	mf::LogError(__FUNCTION__)<<"The format for the compressed data is to be defined";
      }  
  }

  // get byte content for a given data type
  // NOTE: cast assumes host byte order
  template<typename T> T ConvertToValue(const void *in)
    {
      const T *ptr = static_cast<const T*>(in);
      return *ptr;
    }


  // binary file format exception
  class formatexception : public std::exception
  {
    virtual const char* what() const throw()
    {
      return "Bad file format";
    }
  };
  static formatexception fex;
}



//
namespace lris
{
  
  // ctor
  PDDPRawInputDriver::PDDPRawInputDriver( fhicl::ParameterSet const &pset,
					  art::ProductRegistryHelper &helper,
					  art::SourceHelper const &pm ) :
    __sourceHelper( pm ),
    __currentSubRunID(),
    __eventCtr( 0 ),
    __eventNum( 0 )
  {
    //helper.reconstitutes< raw::ExternalTrigger, art::InEvent>("daq");
    helper.reconstitutes<std::vector<raw::RawDigit>, art::InEvent>("daq");


    // number of uncompressed ADC samples per channel in PDDP CRO data (fixed parameter)
    __nsacro = 10000;

    // could also use pset if more parametres are needed (e.g., for LRO data)
    
  }

  //
  void PDDPRawInputDriver::closeCurrentFile()
  {
    __close();
  }
   
  //
  void PDDPRawInputDriver::readFile( std::string const &name,
				     art::FileBlock* &fb )
  {
    __close();
    
    __eventCtr = 0;
    __eventNum = 0;
    
    fb = new art::FileBlock(art::FileFormatVersion(1, "DPPD RawInput 2019"), name);
    
    //
    __file.open( name.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    
    if( !__file.is_open() )
      {
	throw art::Exception( art::errors::FileOpenError )
	  << "Error opening binary file " << name << std::endl;
      }
    
    // get file size
    __filesz = __file.tellg();
  
    // move to beginning
    __file.seekg(0, std::ios::beg);
  
    // unpack event table
    if( __unpack_evtable() == 0 )
      {
	__close();
	throw art::Exception( art::errors::FileReadError )
	  << "File " << name << " does not have any events"<< std::endl;
      }

    __currentSubRunID = art::SubRunID();
    __file_seqno      = __get_file_seqno( name );
  }


  //
  bool PDDPRawInputDriver::readNext(  art::RunPrincipal* const &inR,
				      art::SubRunPrincipal* const &inSR,
				      art::RunPrincipal* &outR,
				      art::SubRunPrincipal* &outSR,
				      art::EventPrincipal* &outE )
  {
    mf::LogInfo(__FUNCTION__)<<"Processing "<<__eventCtr<<" event record";
    if( __eventCtr == __eventNum ) 
      {
	//ok we finished
	mf::LogDebug(__FUNCTION__)<<"Finished reading "<<__eventNum<<" events";
	return false;
      }
    
    // move to the next file position
    if( __events[ __eventCtr ] != __file.tellg() )
      __file.seekg( __events[ __eventCtr ], std::ios::beg );
    size_t bsz = __evsz[ __eventCtr ];
    
    std::vector<BYTE> buf;
    __readChunk( buf, bsz );
    
    // increment our event counter
    __eventCtr++;
    
    DaqEvent event;
    //bool ok = 
    __unpackEvent( buf, event );
    // not sure what art wants me to do here if this was not ok ???
    
    art::RunNumber_t rn     = event.runnum;
    art::SubRunNumber_t srn = __file_seqno; // seq no from file name
    
    uint64_t sec  = event.trigstamp.tv_sec;
    uint64_t nsec =  event.trigstamp.tv_nsec;

    // first 4 bytes are seconds and last four bytes are nsec
    uint64_t tval = (sec << 32) + nsec;
        
    // here we are using event timestamp 
    // what happens if events in the future in same sub-run 
    // (different L2 builder) event have a lower timestamp
    art::Timestamp tstamp( tval );
    
    bool doupdate;
    if( (doupdate = (rn != __currentSubRunID.run())) )
      {
	outR = __sourceHelper.makeRunPrincipal(rn, tstamp);
	doupdate = true;
      }
    if( (doupdate = (srn != __currentSubRunID.subRun())) )
      {
	outSR = __sourceHelper.makeSubRunPrincipal(rn, srn, tstamp);
      }
    
    if( doupdate )
      __currentSubRunID = art::SubRunID( rn, srn );


    outE = __sourceHelper.makeEventPrincipal( __currentSubRunID.run(), 
					      __currentSubRunID.subRun(),
					      event.evnum, tstamp );
    

    //std::unique_ptr<raw::ExternalTrigger> trig_data (new raw::ExternalTrigger(event.trigtype, tval));
    std::unique_ptr< std::vector<raw::RawDigit> > cro_data ( new std::vector<raw::RawDigit>  );

    cro_data->reserve( event.crodata.size() );
    for( size_t i=0;i<event.crodata.size();i++ )
      {
	// This ch Id is based on the order the channel data are stored in file (always the same)
	raw::ChannelID_t ch = i; 
	cro_data->push_back( raw::RawDigit(ch, __nsacro, 
					   std::move( event.crodata[i] ), 
					   event.compression) );
      }

    //art::put_product_in_principal(std::move(trig_data), *outE, "daq");
    art::put_product_in_principal(std::move(cro_data), *outE, "daq");
    
    return true;
  }
  

  //
  ///
  void PDDPRawInputDriver::__close()
  {
    if(__file.is_open())
      __file.close();  
  
    __filesz = 0;
  }

  //
  // read a chunk of bytes from file
  void PDDPRawInputDriver::__readChunk( std::vector<BYTE> &bytes, size_t sz )
  {
    bytes.resize( sz );
    __file.read( &bytes[0], sz );
    if( !__file )
      {
	ssize_t nb = __file.gcount();
	//mf::LogWarning(__FUNCTION__)
	//<<"Could not read "<<sz<" bytes "<<" only "<<nb<<" were available"<<endl;
	bytes.resize( nb );
	// reset stream state from errors
	__file.clear();
      }
  }

  //
  // unpack the header with event table
  unsigned PDDPRawInputDriver::__unpack_evtable()
  {
    unsigned rval = 0;
    //__eventNum    = 0; 
  
    std::vector<BYTE> buf;
    size_t msz = 2*sizeof(uint32_t);
    __readChunk( buf, msz);
    if( buf.size() != msz )
      {
	mf::LogError(__FUNCTION__)<<"Could not read number of events";
	return 0;
      }
  
    
    // number of events in the file
    __eventNum = ConvertToValue<uint32_t>(&buf[4]);
    rval += buf.size();
    
    // file 
    if( __eventNum == 0 )
      {
	mf::LogError(__FUNCTION__)<<"File does not contain any events";
	return 0;
      }

    mf::LogInfo(__FUNCTION__)<<"Number of events in this file "<<__eventNum;
    
    size_t evtsz = __eventNum * 4 * sizeof(uint32_t);
    if(  evtsz + rval >= __filesz )
      {
	mf::LogError(__FUNCTION__)<<"Cannot find event table";
	return 0;
      }

    // read next chunk with event table info
    __readChunk( buf, evtsz );
    rval += buf.size();
  
    // unpack sizes of events in sequence
    // we are only interested in the size here
    __evsz.clear();
    for( unsigned i=0;i<buf.size();i+=16 )
      {
	unsigned o = 0;
	//uint32_t sn = ConvertToValue<uint32_t>(&buf[i+o]);
	
	o = 4;
	uint32_t sz = ConvertToValue<uint32_t>(&buf[i+o]);
	__evsz.push_back( sz );
      }

    // generate table of positions of events in a file
    __events.clear();
  
    // first event is at the current position
    __events.push_back( __file.tellg() );
    for(size_t i=0; i < __evsz.size()-1; i++)
      {
	__file.seekg( __evsz[i], std::ios::cur );
	if( !__file )
	  {
	    mf::LogError(__FUNCTION__)<<"Event table does not match file size";
	    __file.clear();
	    __evsz.resize( __events.size() );
	    __eventNum = __events.size();
	    break;
	  }
	__events.push_back( __file.tellg() );
      }
  
    //for( auto e : __events ) std::cout<<e<<"\n";
    //for( auto e : __evsz ) std::cout<<e<<"\n";
  
    // return the number of bytes unpacked
    return rval;
  }

  //
  // unpack event info from each l1evb fragment
  unsigned PDDPRawInputDriver::__unpack_eve_info( const char *buf, eveinfo_t &ei )
  {
    //
    unsigned rval = 0;
  
    //
    //memset(&ei, 0, sizeof(ei));
    try
      {
	// check for delimiting words
	BYTE k0 = buf[0];
	BYTE k1 = buf[1];
	static const unsigned evskey = 0xFF;
	if( !( ((k0 & 0xFF) == evskey) && ((k1 & 0xFF) == evskey) ) )
	  {
	    mf::LogError(__FUNCTION__)<<"Event delimiting word could not be detected";
	    throw fex;
	  }
	rval += 2;

	// decode run number
	ei.runnum   = ConvertToValue<uint32_t>( buf+rval );
	rval += sizeof( ei.runnum );

	// run flags
	ei.runflags = (uint8_t)buf[rval++];

	// this is actually written in host byte order
	ei.ti = ConvertToValue<triginfo_t>( buf+rval );
	rval += sizeof(ei.ti);
      
	// data quality flags
	ei.evflag = (uint8_t)buf[rval++];
      
	// event number 4 bytes
	ei.evnum   = ConvertToValue<uint32_t>( buf+rval );
	rval += sizeof( ei.evnum );
      
	// size of the lro data segment
	ei.evszlro   = ConvertToValue<uint32_t>( buf+rval );
	rval += sizeof( ei.evszlro );
      
	// size of the cro data segment
	ei.evszcro   = ConvertToValue<uint32_t>( buf+rval );
	rval += sizeof( ei.evszcro );
      }
    catch(std::exception &e)
      {
	rval = 0;
      }
  
    return rval;
  }
  
  //
  //
  bool PDDPRawInputDriver::__unpackEvent( std::vector<BYTE> &buf, DaqEvent &event )
  {
    // fragments from each L1 builder
    std::vector<fragment_t> frags;
  
    unsigned idx = 0;
    for(;;)
      {
	fragment_t afrag;
	if( idx >= buf.size() ) break;
	unsigned rval = __unpack_eve_info( &buf[idx], afrag.ei );
	if( rval == 0 ) return false;
	idx += rval;
	// set point to the binary data
	afrag.bytes = &buf[idx];
	size_t dsz = afrag.ei.evszcro + afrag.ei.evszlro;
	idx += dsz;
	idx += 1; // "Bruno byte"
      
	// some basic checks
	if( frags.size() >= 1 )
	  {
	    if( frags[0].ei.runnum != afrag.ei.runnum )
	      {
		mf::LogError(__FUNCTION__)<<"run numbers do not match among event fragments";
		continue;
	      }
	    if( frags[0].ei.evnum != afrag.ei.evnum )
	      {
		mf::LogError(__FUNCTION__)<<"event numbers do not match among event fragments";
		mf::LogDebug(__FUNCTION__)<<"event number "<< frags[0].ei.evnum;
		mf::LogDebug(__FUNCTION__)<<"event number "<< afrag.ei.evnum;
		continue;
	      }
	    // check also timestamps???
	  }
      
	frags.push_back( afrag );
      }
  
    //mf::LogDebug(__FUNCTION__)<<"number of fragments "<<frags.size()<<"\n";
    // 
    std::mutex iomutex;
    std::vector<std::thread> threads(frags.size() - 1);
    unsigned nsa = __nsacro;
    for (unsigned i = 1; i<frags.size(); ++i) 
      {
	auto afrag = frags.begin() + i;
	threads[i-1] = std::thread([&iomutex, i, nsa, afrag] {
	    {
	      std::lock_guard<std::mutex> iolock(iomutex);
	      //msg_info << "Unpack thread #" << i << " is running\n";
	    }
	    //unpackLROData( f0->bytes, f0->ei.evszlro, ... );
	    unpackCroData( afrag->bytes + afrag->ei.evszlro, afrag->ei.evszcro, 
			   GETDCFLAG(afrag->ei.evflag), nsa, afrag->crodata );
	  });
      }
  
    // unpack first fragment in the main thread
    auto f0 = frags.begin();
    event.good      = EVDQFLAG( f0->ei.evflag );
    event.runnum    = f0->ei.runnum;
    event.runflags  = f0->ei.runflags;
    //
    event.evnum     = f0->ei.evnum;
    event.evflags.push_back( f0->ei.evflag );
    //
    event.trigtype  = f0->ei.ti.type;
    event.trignum   = f0->ei.ti.num;
    event.trigstamp = f0->ei.ti.ts;
  
    //unpackLROData( f0->bytes, f0->ei.evszlro, ... );
    unpackCroData( f0->bytes + f0->ei.evszlro, f0->ei.evszcro, GETDCFLAG(f0->ei.evflag),
		   nsa, event.crodata );
    
    event.compression = raw::kNone;
    // the compression should be set for all L1 event builders, 
    // since this depends on loaded AMC firmware
    if( GETDCFLAG(f0->ei.evflag) ) 
      event.compression = raw::kHuffman;
  
    // wait for other threads to complete
    for (auto& t : threads) t.join();
  
    // merge with other fragments
    for (auto it = frags.begin() + 1; it != frags.end(); ++it )
      {
	event.good = ( event.good && EVDQFLAG( f0->ei.evflag ) );
	event.evflags.push_back( it->ei.evflag );
	
	// 
	event.crodata.reserve(event.crodata.size() + it->crodata.size() );
	std::move( std::begin( it->crodata ), std::end( it->crodata ), 
		   std::back_inserter( event.crodata ));
	it->crodata.clear();
      }
  
    return true;
  }


  // assumed file name form <runno>_<seqno>_<L2evbID>.<extension>
  // e.g., 198_129_b.test
  unsigned PDDPRawInputDriver::__get_file_seqno( std::string s )
  {
    unsigned rval = 0;
    
    //std::cout << "input " << s << std::endl;
    std::regex r("[[:digit:]]+_([[:digit:]]+)_[[:alnum:]]\\.[[:alnum:]]+");
    std::smatch m; 
  
    if( !regex_search( s, m, r ) ) 
      {
	mf::LogWarning(__FUNCTION__) << "Unable to extract seqno from file name "<< s;
	return rval;
      }
  
    // 
    try 
      {
	// str() is the entire match 
	rval = std::stoi( m.str(1) );
      }
    catch (const std::invalid_argument& ia) 
      {
	rval = 0;
	mf::LogWarning(__FUNCTION__) << "This should never happen";
      }
  
    return rval;
  }
  

} //namespace lris
