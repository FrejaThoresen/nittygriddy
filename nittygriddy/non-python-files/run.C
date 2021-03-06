/*
 * This is an automatically generated file. Do yourself a favour and
 * do __not__ edit it by hand. It will be overwritten eventually and
 * you will be sad. Instead, file an issue over at the nittygriddy
 * gitlab page, please.
 */

#ifndef __CINT__
#include <assert.h>
#include <fstream>
#include <iostream>
#include <vector>

#include "TROOT.h"
#include "TProof.h"
#include "TChain.h"
#include "TObject.h"
#include "TList.h"
#include "TString.h"

#include "AliAnalysisManager.h"
#include "AliAnalysisTaskCfg.h"
#include "AliMCEventHandler.h"
#include "AliESDInputHandler.h"
#include "AliAODInputHandler.h"
#include "AliAnalysisGrid.h"
#include "AliAnalysisAlien.h"
#include "AliMultSelectionTask.h"
#include "AliAODHandler.h"

#endif
#include "GetSetting.C"
#include <stdlib.h>     /* atoi */

enum {kLOCAL, kLITE, kPOD, kGRID};
enum {kGRID_FULL, kGRID_OFFLINE, kGRID_TEST, kGRID_MERGE_ONLINE, kGRID_MERGE_OFFLINE};

TChain* makeChain() {
  TChain * chain = 0;
  if (GetSetting("datatype") == "aod") {
    chain = new TChain ("aodTree");
  }
  if (GetSetting("datatype") == "esd") {
    if (GetSetting("read_trackref") == "true" && GetSetting("runmode") != "grid") {
      chain = new TChain ("TE");  // Tree name in galice.root files
    } else {
      chain = new TChain ("esdTree");  // Tree name in AliESDs.root
    }
  }
  TString incollection = "./input_files.dat";
  ifstream file_collect(incollection.Data());
  TString line;
  while (line.ReadLine(file_collect) ) {
    chain->Add(line.Data());
  }
  return chain;
}


void setUpIncludes(Int_t runmode) {
  // Set the include directories
  // gProof is without -I :P and gSystem has to be set for all runmodes
  gSystem->AddIncludePath("-I$ALICE_ROOT/include");
  gSystem->AddIncludePath("-I$ALICE_PHYSICS/include");
  if (runmode == kLITE){
    gProof->AddIncludePath("$ALICE_ROOT/include", kTRUE);
    gProof->AddIncludePath("$ALICE_PHYSICS/include", kTRUE);
  }
}

AliAnalysisGrid* CreateAlienHandler(const std::string gridMode) {
  // Create a generic alien grid handler.
  AliAnalysisAlien *plugin = new AliAnalysisAlien();

  plugin->AddIncludePath("-I. -I$ROOTSYS/include -I$ALICE_ROOT/include -I$ALICE_PHYSICS/include");
  plugin->SetAdditionalLibs(("libOADB.so libSTEERBase.so " + GetSetting("par_files")).c_str());

  plugin->SetOverwriteMode();
  plugin->SetExecutableCommand("aliroot -q -b");
  // The following option is necessary to write the merge jdl's
  plugin->SetMergeViaJDL(true);

  // Can be "full", "test", "offline", "submit" or "merge"
  // merging only works in "full" mode?!
  if (gridMode == "offline"){
    plugin->SetRunMode(gridMode.c_str());
  }
  else if (gridMode == "test"){
    plugin->SetRunMode(gridMode.c_str());
  }
  else if (gridMode == "full"){
    plugin->SetRunMode(gridMode.c_str());
  }
  else if (gridMode == "merge_online"){
    plugin->SetRunMode("terminate");
  }
  else if (gridMode == "merge_offline"){
    plugin->SetRunMode("terminate");
    plugin->SetMergeViaJDL(false);
  }
  else {
    std::cout << "Invalid gridMode!" << std::endl;
    assert(0);
  }
  //Set versions of used packages
  plugin->SetAliPhysicsVersion(GetSetting("aliphysics_version").c_str());

  // Declare input data to be processed
  plugin->SetGridDataDir(GetSetting("datadir").c_str());
  // The alien plugin treats the first folder in data_pattern as if it
  // were preceded by a wildcard (unless preceeded by /, which makes
  // no sense, since that would be an absolute path...)!
  // i.e. my/path/AliAOD.root is treated as *my/path/AliAOD.root
  // I don't want that crab in the dataset files and it would break the local search
  // So we preceed the patter by an `/` here
  plugin->SetDataPattern(TString::Format("/%s", GetSetting("data_pattern").c_str()));
  plugin->SetGridWorkingDir(GetSetting("workdir").c_str());
  plugin->SetAnalysisMacro(TString::Format("Macro_%s.C", GetSetting("workdir").c_str()));
  plugin->SetExecutable(TString::Format("Exec_%s.sh", GetSetting("workdir").c_str()));
  plugin->SetJDLName(TString::Format("Task_%s.jdl", GetSetting("workdir").c_str()));
  plugin->SetDropToShell(false);  // do not open a shell
  plugin->SetRunPrefix(GetSetting("run_number_prefix").c_str());
  plugin->AddRunList(GetSetting("run_list").c_str());
  plugin->SetGridOutputDir("output");
  plugin->SetMaxMergeFiles(25);
  // Maximum number of files to be processed per subjob
  plugin->SetSplitMaxInputFileNumber(atoi(GetSetting("max_files_subjob").c_str()));
  plugin->SetMergeExcludes("EventStat_temp.root"
			   "event_stat.root");
  int runs_per_master = atoi(GetSetting("runs_per_master").c_str());
  int n_runs = TString(GetSetting("run_list").c_str()).CountChar(' ') + 1;
  plugin->SetNrunsPerMaster(runs_per_master > 0 ? runs_per_master : n_runs);
  // Use run number as output folder names
  plugin->SetOutputToRunNo();
  plugin->SetTTL(atoi(GetSetting("ttl").c_str()));

  // These options might be crucial in order to have the merging jdls properly set up, but who knows...
  plugin->SetInputFormat("xml-single");
  // Optionally modify job price (default 1)
  plugin->SetPrice(1);
  // Optionally modify split mode (default 'se')
  plugin->SetSplitMode("se");

  return plugin;
};


void run(const std::string gridMode="")
{
  // load GetSetting.C macro to allow access to settings for this particular dataset
  // gROOT->LoadMacro("./GetSetting.C");
  Int_t const max_events = atoi(GetSetting("max_n_events").c_str());
  Int_t runmode = -1;
  if (GetSetting("wait_for_gdb") == "true"){
    std::cout << "Execution is paused so that you cann attach gdb to the running process:" << std::endl;
    std::cout << "gdb -p " << gSystem->GetPid() << std::endl;
    std::cout << "Press any key to continue" << std::endl;
    std::cin.ignore();
  }
  if (GetSetting("runmode") == "local")
    runmode = kLOCAL;
  else if (GetSetting("runmode") == "lite")
    runmode = kLITE;
  else if (GetSetting("runmode") == "grid")
    runmode = kGRID;
  else {
    std::cout << "Invalid analysis mode: " << GetSetting("runmode")
	      << "! I have no idea whats going on..." << std::endl;
    assert(0);
  }
  // start proof if necessary
  TString proofUrl = "";
  if (runmode == kLITE) {
    proofUrl += "lite://";
    if (GetSetting("nworkers") != "-1"){
      proofUrl += "?workers=";
      proofUrl += GetSetting("nworkers");
    }
  } else if (runmode == kPOD) {
    proofUrl += "pod://";
  }
  if (proofUrl.Length()) {
    TProof::Open(proofUrl);
  }

  setUpIncludes(runmode);
  // loadLibsNonGrid();
  // Create  and setup the analysis manager
  AliAnalysisManager *mgr = new AliAnalysisManager();

  TString analysisName("AnalysisResults");
  mgr->SetCommonFileName(analysisName + TString(".root"));

  if (GetSetting("datatype") == "aod") {
    AliVEventHandler* aodH = new AliAODInputHandler;
    mgr->SetInputEventHandler(aodH);
  }
  if (GetSetting("datatype") == "esd") {
    AliESDInputHandler* esdH = new AliESDInputHandler;
    if (GetSetting("make_AOD") == "true") esdH->SetReadFriends(kFALSE);
    mgr->SetInputEventHandler(esdH);
  }
  if (GetSetting("is_mc") == "true" && GetSetting("datatype") == "esd") {
    AliMCEventHandler* handler = new AliMCEventHandler;
    if (GetSetting("read_trackref") == "true") handler->SetReadTR(kTRUE);
    mgr->SetMCtruthEventHandler(handler);
  }
  if (GetSetting("make_AOD") == "true") {
    AliAODHandler* aodH = new AliAODHandler();
    aodH->SetOutputFileName("AliAOD.root");
    mgr->SetOutputEventHandler(aodH);
  }

  if (runmode == kGRID) {
    AliAnalysisGrid *alienHandler = CreateAlienHandler(gridMode);
    alienHandler->SetKeepLogs(kTRUE);
    if (!alienHandler) return;
    mgr->SetGridHandler(alienHandler);
  }

  // Add tasks; either from MLTrainDefinition.cfg or ConfigureTrain.C
  if (GetSetting("use_train_conf") == "true") {
    std::cout << "Loading analysis tasks from MLTrainDefinition.cfg file" << std::endl;
    TObjArray *arr = AliAnalysisTaskCfg::ExtractModulesFrom("MLTrainDefinition.cfg");
    TIter next(arr);
    AliAnalysisTaskCfg *module;
    while ((module = (AliAnalysisTaskCfg*)next())) {
      module->ExecuteMacro();
      module->ExecuteConfigMacro();
    }
  } else {
    std::cout << "Loading analysis tasks from ConfigureTrain.C file" << std::endl;
    gROOT->LoadMacro("./ConfigureTrain.C+");
    gROOT->ProcessLine("ConfigureTrain()");
  }

  if (!mgr->InitAnalysis()) return;
  mgr->PrintStatus();

  if (runmode == kGRID) {
    std::cout << "Starting grid analysis" << std::endl;
    mgr->StartAnalysis("grid");
  }
  else if ((runmode == kLOCAL) || (runmode == kLITE)) {
      // Process with chain
    TChain *chain = makeChain();
      if (!chain) {
	std::cout << "Dataset is empty!" << std::endl;
	return;
      }
      if (runmode == kLITE)
	mgr->StartAnalysis("proof", chain, max_events);

      else if (runmode == kLOCAL) {
	// in order to be inconvinient, aliroot does interprete
	// max_events differently for proof and local :P
	if (max_events == -1) mgr->StartAnalysis("local", chain);
	else mgr->StartAnalysis("local", chain, max_events);
      }
    }
}
