/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

#ifndef FlowEventSimpleMaker_H
#define FlowEventSimpleMaker_H

#include "AliFlowCommon/AliFlowEventSimple.h"  
#include "AliFlowCommon/AliFlowTrackSimpleCuts.h"

class TTree;

// FlowEventSimpleMaker:
// Class to fill the AliFlowEventSimple with AliFlowTrackSimple objects
// This works also outside the AliRoot Framework
          
class FlowEventSimpleMaker {

 public:

  FlowEventSimpleMaker();             //constructor
  virtual  ~FlowEventSimpleMaker();   //destructor
  
  //TTree
  AliFlowEventSimple* FillTracks(TTree* anInput, AliFlowTrackSimpleCuts* RPCuts, AliFlowTrackSimpleCuts* POICuts);   //use own cut class
    
 private:
  FlowEventSimpleMaker(const FlowEventSimpleMaker& anAnalysis);            //copy constructor
  FlowEventSimpleMaker& operator=(const FlowEventSimpleMaker& anAnalysis); //assignment operator
          
  ClassDef(FlowEventSimpleMaker,0)    // macro for rootcint
};
      
#endif

