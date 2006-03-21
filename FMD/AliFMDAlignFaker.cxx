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
// Forward Multiplicity Detector based on Silicon wafers. 
//
// This task creates fake alignrations. Which alignration, depends on
// the bit mask passed to the constructor, or added by `AddAlign'.
//
// The default is to write all alignration parameters to a local
// storage `local://cdb' which is a directory in the current
// directory. 
//                                                       
#include "AliLog.h"		   // ALILOG_H
#include "AliFMDAlignFaker.h"      // ALIFMDALIGNFAKER_H
#include <AliCDBManager.h>         // ALICDBMANAGER_H
#include <AliCDBEntry.h>           // ALICDBMANAGER_H
#include <AliAlignObj.h>
#include <AliAlignObjAngles.h>
#include <Riostream.h>
#include <TSystem.h>
#include <TMath.h>
#include <TRandom.h>
#include <TClonesArray.h>
#include <TString.h>
#include <TFile.h>
#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TROOT.h>

//====================================================================
ClassImp(AliFMDAlignFaker)
#if 0
  ; // This is here to keep Emacs for indenting the next line
#endif

//____________________________________________________________________
AliFMDAlignFaker::AliFMDAlignFaker(Int_t mask, const char* geo, 
				   const char* loc) 
  : TTask(geo, loc),
    fMask(mask),
    fRunMin(0),
    fRunMax(10), 
    fArray(0)
{
  // Default constructor 
  SetSensorDisplacement();
  SetSensorRotation();
  SetHalfDisplacement();
  SetHalfRotation();
}

//__________________________________________________________________
void
AliFMDAlignFaker::SetSensorDisplacement(Double_t x1, Double_t y1, Double_t z1,
					Double_t x2, Double_t y2, Double_t z2)
{
  // Set sensor displacement (unit is centimeters)
  fSensorTransMin.SetXYZ(x1, y1, z1);
  fSensorTransMax.SetXYZ(x2, y2, z2);
}

//__________________________________________________________________
void
AliFMDAlignFaker::SetSensorRotation(Double_t x1, Double_t y1, Double_t z1,
				    Double_t x2, Double_t y2, Double_t z2)
{
  // Set sensor rotations (unit is degrees)
  fSensorRotMin.SetXYZ(x1, y1, z1);
  fSensorRotMax.SetXYZ(x2, y2, z2);
}

//__________________________________________________________________
void
AliFMDAlignFaker::SetHalfDisplacement(Double_t x1, Double_t y1, Double_t z1,
				      Double_t x2, Double_t y2, Double_t z2)
{
  // Set half ring/cone displacement (unit is centimeters)
  fHalfTransMin.SetXYZ(x1, y1, z1);
  fHalfTransMax.SetXYZ(x2, y2, z2);
}

//__________________________________________________________________
void
AliFMDAlignFaker::SetHalfRotation(Double_t x1, Double_t y1, Double_t z1,
				  Double_t x2, Double_t y2, Double_t z2)
{
  // Set half ring/cone rotations (unit is degrees)
  fHalfRotMin.SetXYZ(x1, y1, z1);
  fHalfRotMax.SetXYZ(x2, y2, z2);
}

//__________________________________________________________________
#define IS_NODE_HALF(name) \
  (name[0] == 'F' && name[2] == 'M' && (name[3] == 'T' || name[3] == 'B'))
#define IS_NODE_SENSOR(name) \
  (name[0] == 'F' && name[2] == 'S' && name[3] == 'E')

//__________________________________________________________________
void
AliFMDAlignFaker::Exec(Option_t*)
{
  // Make the objects. 

  // Get geometry 
  if (!gGeoManager) {
    if (!TGeoManager::Import(GetName())) {
      AliFatal(Form("Failed to import geometry from %s", GetName()));
      return;
    }
  }
  // Get top volume 
  TGeoVolume* topVolume = gGeoManager->GetTopVolume();
  if (!topVolume) {
    AliFatal("No top-level volume defined");
    return;
  }
  // Make container of transforms 
  if (!fArray) fArray = new TClonesArray("AliAlignObjAngles");
  fArray->Clear();
  
  // Make an iterator
  TGeoIterator next(topVolume);
  TGeoNode* node = 0;

  // Loop over all entries in geometry to find our nodes. 
  while ((node = static_cast<TGeoNode*>(next()))) {
    const char* name =  node->GetName();
    if (!(IS_NODE_HALF(name) && TESTBIT(fMask, kHalves)) &&
	!(IS_NODE_SENSOR(name) && TESTBIT(fMask, kSensors))) 
      continue;
    
    // Get the path 
    TString path(Form("/%s", gGeoManager->GetNode(0)->GetName()));
    Int_t nLevel = next.GetLevel();
    for (Int_t lvl = 0; lvl <= nLevel; lvl++) {
      TGeoNode* p = next.GetNode(lvl);
      if (!p) {
	if (lvl != 0)
	  AliWarning(Form("No node at level %d in path %s",lvl,path.Data()));
	continue;
      }
      if (!path.IsNull()) path.Append("/");
      path.Append(p->GetName());
    }
    Int_t id = node->GetVolume()->GetNumber();
    if (IS_NODE_HALF(name))   MakeAlignHalf(path, id);
    if (IS_NODE_SENSOR(name)) MakeAlignSensor(path, id);
  }

  TString t(GetTitle());
  if (t.Contains("local://") || t.Contains("alien://")) 
    WriteToCDB();
  else 
    WriteToFile();
}
  
//__________________________________________________________________
Bool_t
AliFMDAlignFaker::MakeAlign(const TString& path, Int_t id, 
			    Double_t transX, Double_t transY, Double_t transZ,
			    Double_t rotX,   Double_t rotY, Double_t rotZ)
{
  AliDebug(1, Form("Make alignment for %s (volume %d): (%f,%f,%f) (%f,%f,%f)", 
		   path.Data(), id, transX, transY, transZ, rotX, rotY, rotZ));
  Int_t nAlign = fArray->GetEntries();
  AliAlignObjAngles* obj = 
    new ((*fArray)[nAlign]) AliAlignObjAngles(path.Data(), id,0,0,0,0,0,0);
  if (!obj) {
    AliError(Form("Failed to create alignment object for %s", path.Data()));
    return kFALSE;
  }
  if (!obj->SetLocalPars(transX, transY, transZ, rotX, rotY, rotZ)) {
    AliError(Form("Failed to set local transforms on %s", path.Data()));
    return kTRUE;
  }
  return kTRUE;
}

//__________________________________________________________________
Bool_t
AliFMDAlignFaker::MakeAlignHalf(const TString& path, Int_t id)
{
  AliDebug(15, Form("Make alignment for half-ring/cone %s", path.Data()));
  Double_t transX = gRandom->Uniform(fHalfTransMin.X(), fHalfTransMax.X());
  Double_t transY = gRandom->Uniform(fHalfTransMin.Y(), fHalfTransMax.Y());
  Double_t transZ = gRandom->Uniform(fHalfTransMin.Z(), fHalfTransMax.Z());
  Double_t rotX   = gRandom->Uniform(fHalfRotMin.X(),   fHalfRotMax.X());
  Double_t rotY   = gRandom->Uniform(fHalfRotMin.Y(),   fHalfRotMax.Y());
  Double_t rotZ   = gRandom->Uniform(fHalfRotMin.Z(),   fHalfRotMax.Z());
  return MakeAlign(path, id, transX, transY, transZ, rotX, rotY, rotZ);
}

  
//__________________________________________________________________
Bool_t
AliFMDAlignFaker::MakeAlignSensor(const TString& path, Int_t id)
{
  AliDebug(15, Form("Make alignment for sensor %s", path.Data()));
  Double_t transX = gRandom->Uniform(fSensorTransMin.X(), fSensorTransMax.X());
  Double_t transY = gRandom->Uniform(fSensorTransMin.Y(), fSensorTransMax.Y());
  Double_t transZ = gRandom->Uniform(fSensorTransMin.Z(), fSensorTransMax.Z());
  Double_t rotX   = gRandom->Uniform(fSensorRotMin.X(),   fSensorRotMax.X());
  Double_t rotY   = gRandom->Uniform(fSensorRotMin.Y(),   fSensorRotMax.Y());
  Double_t rotZ   = gRandom->Uniform(fSensorRotMin.Z(),   fSensorRotMax.Z());
  return MakeAlign(path, id, transX, transY, transZ, rotX, rotY, rotZ);
}

//__________________________________________________________________
void
AliFMDAlignFaker::WriteToCDB()
{
  // Make the objects. 
  AliCDBManager*     cdb      = AliCDBManager::Instance();
  if (GetTitle())    cdb->SetDefaultStorage(GetTitle());
    
  AliCDBMetaData* meta = new AliCDBMetaData; 
  meta->SetResponsible(gSystem->GetUserInfo()->fRealName.Data()); 
  meta->SetAliRootVersion(gROOT->GetVersion()); 
  meta->SetBeamPeriod(1); 
  meta->SetComment("Dummy data for testing");

  AliCDBId id("FMD/Align/Data", fRunMin, fRunMax);
  cdb->Put(fArray, id, meta);
  cdb->Destroy();
}

//__________________________________________________________________
void
AliFMDAlignFaker::WriteToFile()
{
  TFile* file = TFile::Open(GetTitle(), "RECREATE");
  if (!file) {
    AliFatal(Form("Failed to open file '%s' for output", GetTitle()));
    return;
  }
  file->cd();
  fArray->Write("FMDAlignment");
  file->Close();
  file->Write();
}

  
  
//____________________________________________________________________
//
// EOF
//
