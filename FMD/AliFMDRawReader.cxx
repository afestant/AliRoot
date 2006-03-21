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

/* $Id$ */

//____________________________________________________________________
//
// Class to read ADC values from a AliRawReader object. 
//
// This class uses the AliFMDRawStreamer class to read the ALTRO
// formatted data. 
// 
//          +-------+
//          | TTask |
//          +-------+
//              ^
//              |
//      +-----------------+  <<references>>  +--------------+
//      | AliFMDRawReader |<>----------------| AliRawReader |
//      +-----------------+                  +--------------+
//              |                                  ^
//              | <<uses>>                         |
//              V                                  |
//      +-----------------+      <<uses>>          |
//      | AliFMDRawStream |------------------------+
//      +-----------------+
//              |
//              V
//      +----------------+
//      | AliAltroStream |
//      +----------------+
//
#include <AliLog.h>		// ALILOG_H
#include "AliFMDParameters.h"	// ALIFMDPARAMETERS_H
#include "AliFMDDigit.h"	// ALIFMDDIGIT_H
#include "AliFMDRawStream.h"	// ALIFMDRAWSTREAM_H 
#include "AliRawReader.h"	// ALIRAWREADER_H 
#include "AliFMDRawReader.h"	// ALIFMDRAWREADER_H 
#include "AliFMDAltroIO.h"	// ALIFMDALTROIO_H 
#include <TArrayI.h>		// ROOT_TArrayI
#include <TTree.h>		// ROOT_TTree
#include <TClonesArray.h>	// ROOT_TClonesArray
#include <iostream>
#include <iomanip>
#include <sstream>
#define PRETTY_HEX(N,X) \
  "  0x" << std::setfill('0') << std::setw(N) << std::hex << X \
         << std::setfill(' ') << std::dec

//____________________________________________________________________
ClassImp(AliFMDRawReader)
#if 0
  ; // This is here to keep Emacs for indenting the next line
#endif

//____________________________________________________________________
AliFMDRawReader::AliFMDRawReader(AliRawReader* reader, TTree* tree) 
  : TTask("FMDRawReader", "Reader of Raw ADC values from the FMD"),
    fTree(tree),
    fReader(reader), 
    fSampleRate(1)
{
  // Default CTOR
}


//____________________________________________________________________
void
AliFMDRawReader::Exec(Option_t*) 
{
  // Read raw data into the digits array, using AliFMDAltroReader. 
  if (!fReader->ReadHeader()) {
    Error("ReadAdcs", "Couldn't read header");
    return;
  }

  TClonesArray* array = new TClonesArray("AliFMDDigit");
  fTree->Branch("FMD", &array);

  // Get sample rate 
  AliFMDParameters* pars = AliFMDParameters::Instance();

  // Select FMD DDL's 
  fReader->Select(AliFMDParameters::kBaseDDL>>8);

  UShort_t stripMin = 0;
  UShort_t stripMax = 127;
  UShort_t preSamp  = 0;
  
  do {
    UChar_t* cdata;
    if (!fReader->ReadNextData(cdata)) break;
    size_t   nchar = fReader->GetDataSize();
    UShort_t ddl   = AliFMDParameters::kBaseDDL + fReader->GetDDLID();
    UShort_t rate  = pars->GetSampleRate(ddl);
    AliDebug(1, Form("Reading %d bytes (%d 10bit words) from %d", 
		     nchar, nchar * 8 / 10, ddl));
    // Make a stream to read from 
    std::string str((char*)(cdata), nchar);
    std::istringstream s(str);
    // Prep the reader class.
    AliFMDAltroReader r(s);
    // Data array is approx twice the size needed. 
    UShort_t data[2048], hwaddr, last;
    while (r.ReadChannel(hwaddr, last, data) > 0) {
      AliDebug(5, Form("Read channel 0x%x of size %d", hwaddr, last));
      UShort_t det, sec, str;
      Char_t   ring;
      if (!pars->Hardware2Detector(ddl, hwaddr, det, ring, sec, str)) {
	AliError(Form("Failed to detector id from DDL %d "
		      "and hardware address 0x%x", ddl, hwaddr));
	continue;
      }
      AliDebug(5, Form("DDL 0x%04x, address 0x%03x maps to FMD%d%c[%2d,%3d]", 
		       ddl, hwaddr, det, ring, sec, str));

      // Loop over the `timebins', and make the digits
      for (size_t i = 0; i < last; i++) {
	if (i < preSamp) continue;
	Int_t n = array->GetEntries();
	UShort_t curStr = str + stripMin + i / rate;
	if ((curStr-str) > stripMax) {
	  AliError(Form("Current strip is %d but DB says max is %d", 
			curStr, stripMax));
	}
	AliDebug(5, Form("making digit for FMD%d%c[%2d,%3d] from sample %4d", 
			 det, ring, sec, curStr, i));
	new ((*array)[n]) AliFMDDigit(det, ring, sec, curStr, data[i], 
				      (rate >= 2 ? data[i+1] : 0),
				      (rate >= 3 ? data[i+2] : 0));
	if (rate >= 2) i++;
	if (rate >= 3) i++;
	}
	if (r.IsBof()) break;
    }
  } while (true);
  AliDebug(1, Form("Got a grand total of %d digits", array->GetEntries()));
}

#if 0
// This is the old method, for comparison.   It's really ugly, and far
// too convoluted. 
//____________________________________________________________________
void
AliFMDRawReader::Exec(Option_t*) 
{
  // Read raw data into the digits array
  if (!fReader->ReadHeader()) {
    Error("ReadAdcs", "Couldn't read header");
    return;
  }

  Int_t n = 0;
  TClonesArray* array = new TClonesArray("AliFMDDigit");
  fTree->Branch("FMD", &array);

  // Get sample rate 
  AliFMDParameters* pars = AliFMDParameters::Instance();
  fSampleRate = pars->GetSampleRate(AliFMDParameters::kBaseDDL);

  // Use AliAltroRawStream to read the ALTRO format.  No need to
  // reinvent the wheel :-) 
  AliFMDRawStream input(fReader, fSampleRate);
  // Select FMD DDL's 
  fReader->Select(AliFMDParameters::kBaseDDL);
  
  Int_t    oldDDL      = -1;
  Int_t    count       = 0;
  UShort_t detector    = 1; // Must be one here
  UShort_t oldDetector = 0;
  Bool_t   next        = kTRUE;

  // local Cache 
  TArrayI counts(10);
  counts.Reset(-1);
  
  // Loop over data in file 
  while (next) {
    next = input.Next();

    count++; 
    Int_t ddl = fReader->GetDDLID();
    AliDebug(10, Form("Current DDL is %d", ddl));
    if (ddl != oldDDL || input.IsNewStrip() || !next) {
      // Make a new digit, if we have some data (oldDetector == 0,
      // means that we haven't really read anything yet - that is,
      // it's the first time we get here). 
      if (oldDetector > 0) {
	// Got a new strip. 
	AliDebug(10, Form("Add a new strip: FMD%d%c[%2d,%3d] "
			  "(current: FMD%d%c[%2d,%3d])", 
			  oldDetector, input.PrevRing(), 
			  input.PrevSector() , input.PrevStrip(),
			  detector , input.Ring(), input.Sector(), 
			  input.Strip()));
	new ((*array)[n]) AliFMDDigit(oldDetector, 
				      input.PrevRing(), 
				      input.PrevSector(), 
				      input.PrevStrip(), 
				      counts[0], counts[1], counts[2]);
	n++;
#if 0
	AliFMDDigit* digit = 
	  static_cast<AliFMDDigit*>(fFMD->Digits()->
				    UncheckedAt(fFMD->GetNdigits()-1));
#endif 
      }
	
      if (!next) { 
	AliDebug(10, Form("Read %d channels for FMD%d", 
			  count + 1, detector));
	break;
      }
    
    
      // If we got a new DDL, it means we have a new detector. 
      if (ddl != oldDDL) {
	if (detector != 0) 
	  AliDebug(10, Form("Read %d channels for FMD%d", count + 1, detector));
	// Reset counts, and update the DDL cache 
	count       = 0;
	oldDDL      = ddl;
	// Check that we're processing a FMD detector 
	Int_t detId = fReader->GetDetectorID();
	if (detId != (AliFMDParameters::kBaseDDL >> 8)) {
	  AliError(Form("Detector ID %d != %d",
			detId, (AliFMDParameters::kBaseDDL >> 8)));
	  break;
	}
	// Figure out what detector we're deling with 
	oldDetector = detector;
	switch (ddl) {
	case 0: detector = 1; break;
	case 1: detector = 2; break;
	case 2: detector = 3; break;
	default:
	  AliError(Form("Unknown DDL 0x%x for FMD", ddl));
	  return;
	}
	AliDebug(10, Form("Reading ADCs for 0x%x  - That is FMD%d",
			  fReader->GetEquipmentId(), detector));
      }
      counts.Reset(-1);
    }
    
    counts[input.Sample()] = input.Count();
    
    AliDebug(10, Form("ADC of FMD%d%c[%2d,%3d] += %d",
		      detector, input.Ring(), input.Sector(), 
		      input.Strip(), input.Count()));
    oldDetector = detector;
  }
  fTree->Fill();
  return;

}
#endif

//____________________________________________________________________
// 
// EOF
//
