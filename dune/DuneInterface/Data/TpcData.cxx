// TpcData.cxx

#include "dune/DuneInterface/Data/TpcData.h"

//**********************************************************************

TpcData::TpcData() : m_parent(nullptr) { }

//**********************************************************************

TpcData::TpcData(const AdcDataVector& adcs) : m_parent(nullptr), m_adcs(adcs) { }

//**********************************************************************

TpcData* TpcData::addTpcData(Name nam, bool copyAdcData) {
  if ( nam == "" || nam == "." ) return nullptr;
  Name::size_type ipos = nam.rfind("/");
  if ( ipos != Name::npos ) {
    TpcData* pdat = getTpcData(nam.substr(0, ipos));
    if ( pdat == nullptr ) return nullptr;
    return pdat->addTpcData(nam.substr(ipos+1), copyAdcData);
  }
  
  if ( m_dat.count(nam) ) return nullptr;
  TpcData& tpc = m_dat[nam];
  if ( copyAdcData ) tpc.m_adcs = m_adcs;
  tpc.m_parent = this;
  return &tpc;
}

//**********************************************************************

TpcData* TpcData::getTpcData(Name nam) {
  if ( nam == "" || nam == "." ) return this;
  Name::size_type ipos = nam.find("/");
  if ( ipos == Name::npos ) return m_dat.count(nam) ? &m_dat[nam] : nullptr;
  TpcData* pdat = getTpcData(nam.substr(0,ipos));
  if ( pdat == nullptr ) return nullptr;
  return pdat->getTpcData(nam.substr(ipos+1));
}


//**********************************************************************

const TpcData* TpcData::getTpcData(Name nam) const {
  if ( nam == "" || nam == "." ) return this;
  Name::size_type ipos = nam.find("/");
  if ( ipos == Name::npos ) {
    TpcDataMap::const_iterator idat = m_dat.find(nam);
    if ( idat == m_dat.end() ) return nullptr;
    return &idat->second;
  }
  const TpcData* pdat = getTpcData(nam.substr(0,ipos));
  if ( pdat == nullptr ) return nullptr;
  return pdat->getTpcData(nam.substr(ipos+1));
}

//**********************************************************************

TpcData::AdcDataPtr TpcData::createAdcData(bool updateParent) {
  return addAdcData(AdcDataPtr(new AdcChannelDataMap), updateParent);
}

//**********************************************************************

TpcData::AdcDataPtr TpcData::addAdcData(AdcDataPtr padc, bool updateParent) {
  m_adcs.push_back(padc);
  if ( updateParent && m_parent != nullptr ) m_parent->addAdcData(padc, true);
  return padc;
}

//**********************************************************************

void TpcData::clearAdcData() {
  m_adcs.clear();
}

//**********************************************************************
