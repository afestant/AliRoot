/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id: AliCTPInputTimeParams.cxx 22322 2007-11-22 11:43:14Z cvetan $ */

#include <Riostream.h>


#include "TObject.h"
#include "AliLog.h"   
#include "AliCTPInputTimeParams.h"

ClassImp(AliCTPInputTimeParams)

//_____________________________________________________________________________
AliCTPInputTimeParams::AliCTPInputTimeParams():
  fName(0),
  fLevel(0),        
  fDelay(0),        
  fEdge(0)
{
 // Default constructor
}     

//_____________________________________________________________________________
AliCTPInputTimeParams::AliCTPInputTimeParams( TString& name, UInt_t& level, UInt_t delay, TString edge ):
  fName(name),        
  fLevel(level),        
  fDelay( delay),
  fEdge(edge)
{
}
//_____________________________________________________________________________
AliCTPInputTimeParams::AliCTPInputTimeParams(const AliCTPInputTimeParams &ctptime):
 TObject(ctptime),
 fName(ctptime.fName),
 fLevel(ctptime.fLevel),
 fDelay(ctptime.fDelay),
 fEdge(ctptime.fEdge)
{
 // copy constructor
}
//_____________________________________________________________________________
AliCTPInputTimeParams& AliCTPInputTimeParams::operator=(const AliCTPInputTimeParams &ctptime)
{
 //assignment operator
 if(this==&ctptime) return *this;
 ((TObject *)this)->operator=(ctptime);
 fName=ctptime.fName;
 fLevel=ctptime.fLevel;
 fDelay=ctptime.fDelay;
 fEdge=ctptime.fEdge;
 return *this;
}
//_____________________________________________________________________________
void AliCTPInputTimeParams::SetCTPInputTimeParams( TString name, UInt_t level, UInt_t delay, TString edge )
{
  fName = name;        
  fLevel = level;        
  fDelay = delay;
  fEdge = edge;
}

//_____________________________________________________________________________
void AliCTPInputTimeParams::Print( const Option_t* ) const
{
   // Print
  cout << "  CTP Input Time Params " << endl;
  cout << "  Input Name: " << fName << endl;
  cout << "  Level:      " << fLevel << endl;
  cout << "  Delay:      " << fDelay << endl;
  cout << "  Edge:       " << fEdge << endl;
}
