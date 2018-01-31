/**
 * Author: gplkrsh2@illinois.edu (Harshitha Menon)
 * A distributed load balancer.
*/

#include "PrefixLB.h"

#include "elements.h"

CreateLBFunc_Def(PrefixLB, "The distributed load balancer")

using std::vector;

PrefixLB::PrefixLB(CkMigrateMessage *m) : CBase_PrefixLB(m) {
}

PrefixLB::PrefixLB(const CkLBOptions &opt) : CBase_PrefixLB(opt) {
//  __sdag_init();
  lbname = "PrefixLB";
  if (CkMyPe() == 0)
    CkPrintf("[%d] PrefixLB created\n",CkMyPe());
  InitLB(opt);
}


void PrefixLB::InitLB(const CkLBOptions &opt) {
  thisProxy = CProxy_PrefixLB(thisgroup);
}

void PrefixLB::Strategy(const DistBaseLB::LDStats* const stats) {
	if (CkMyPe() == 0)
		CkPrintf("[%d] In PrefixLB strategy\n", CkMyPe());

  // Initialize constants
  kUseAck = true;
  kTransferThreshold = 1.03;
  kTargetTransferThreshold = kTransferThreshold;
  // Indicates use all the information present
  kPartialInfoCount = -1;
  // Maximum number of times we will try to find a PE to transfer an object
  // successfully
  kMaxTrials = CkNumPes();
  // Max gossip messages sent from each PE
  kMaxGossipMsgCount = 2 * CmiLog2(CkNumPes());
 
  my_stats = stats;

	my_load = 0.0;
  kMaxObjPickTrials = my_stats->n_objs;
	for (int i = 0; i < my_stats->n_objs; i++) {
		my_load += my_stats->objData[i].wallTime; 
  }
  b_load = my_stats->total_walltime - (my_stats->idletime + my_load);
  //my_load += b_load;

  // Reset member variables
	pe_no.clear();
	loads.clear();
	distribution.clear();
  lb_started = false;
  gossip_msg_count = 0;
  negack_count = 0;

  total_migrates = 0;
  total_migrates_ack = 0;

  if (CkMyPe() == 0) {
    CkPrintf("[%d] In PrefixLB strategy starting at %lf threshold %lf UseAck %d\n", CkMyPe(), CkWallTimer(), kTransferThreshold, kUseAck);
  }

  srand((unsigned)(CmiWallTimer()*1.0e06) + CkMyPe());
  // Use reduction to obtain the average load in the system
  CkCallback cb(CkReductionTarget(PrefixLB, AvgLoadReduction), thisProxy);
  contribute(sizeof(double), &my_load, CkReduction::sum_double, cb);
}

/*
* Once the reduction callback is obtained for average load in the system, the
* gossiping starts. Only the underloaded processors gossip.
* Termination of gossip is via QD and callback is DoneGossip.
*/
void PrefixLB::AvgLoadReduction(double x) {
  avg_load = x/CkNumPes();
  // Calculate the average load by considering the threshold for imbalance
  thr_avg = kTransferThreshold * avg_load;

  // If my load is less than the avg_load, I am underloaded. Initiate the gossip
  // by sending my load information to random neighbors.
  if (my_load < thr_avg) {
		double r_loads[1];
		int r_pe_no[1];
    r_loads[0] = my_load;
    r_pe_no[0] = CkMyPe();
    // Initialize the req_hop of the message to 0
		req_hop = 0;
  }

  init_load = my_load;
  CkCallback cb(CkReductionTarget(PrefixLB, MaxLoadReductionBeforeLB), thisProxy);
  contribute(sizeof(double), &my_load, CkReduction::max_double, cb);
}

void PrefixLB::MaxLoadReductionBeforeLB(double init_max_load) {
  if (CkMyPe() == 0 && _lb_args.debug()>=1) {
    CkPrintf("PrefixLB>>>Before LB: max = %lf, ave = %lf, ratio = %lf\n", init_max_load, avg_load, init_max_load / avg_load);
  }
  kTransferThreshold = init_max_load/avg_load;
#if 0
  if (CkMyPe() == 0) {
    CkCallback cb(CkIndex_PrefixLB::FoundPrefix(), thisProxy);
    CkStartQD(cb);
  }
#endif
  if (CkMyPe() == 0){
//    prefix_sum = my_load;
    thisProxy[CkMyPe()+1].receivedPrefixSum(my_load);
    CkCallback cb(CkReductionTarget(PrefixLB, FoundPrefix), thisProxy);
    contribute(sizeof(double), &my_load, CkReduction::max_double, cb);
  }
}

void PrefixLB::FoundPrefix(){
  double underload = (avg_load * CkMyPe()) - prefix_sum;
  double local_overload = my_load-avg_load;
  total_migrates = 0;
  CkPrintf("\n[PE-%d] Prefix sum load = %lf, my load = %lf, underload = %lf\n", CkMyPe(), prefix_sum, my_load, underload);
  if(CkMyPe()>0 && underload>0.0){
    objs_count = 0;
    // Count the number of objs that are migratable and whose load is not 0.
    for(int i=0; i < my_stats->n_objs; i++) {
      if (my_stats->objData[i].migratable &&
          my_stats->objData[i].wallTime > 0.0001) {
        objs_count++;
      }
    }

    minHeap objs(objs_count);
    for(int i=0; i < my_stats->n_objs; i++) {
      if (my_stats->objData[i].migratable &&
          my_stats->objData[i].wallTime > 0.0001) {
        InfoRecord* item = new InfoRecord;
        item->load = my_stats->objData[i].wallTime;
        item->Id = i;
        objs.insert(item);
      }
    }
  
    double tmp_prefix_sum = prefix_sum;
    double tmp_my_load = my_load;
    while (tmp_prefix_sum < (thr_avg*CkMyPe()) && tmp_my_load > 0.0) {
      if (objs_count < 2) break;
      InfoRecord* obj = objs.deleteMin();
      tmp_prefix_sum += obj->load;
      tmp_my_load -= obj->load;
      MigrateInfo* migrateMe = new MigrateInfo;
      migrateMe->obj = my_stats->objData[obj->Id].handle;
      migrateMe->from_pe = CkMyPe();
      migrateMe->to_pe = CkMyPe()-1;
      migrateInfo.push_back(migrateMe);
      total_migrates++;
    }
    
    CkPrintf("\n[PE-%d]Will send load %lf (%d objects) to left[after prefix =%lf]\n",CkMyPe(), underload, total_migrates, tmp_prefix_sum); fflush(stdout);
  }

//  total_migrates = 0;

  msg = new(total_migrates,CkNumPes(),CkNumPes(),0) LBMigrateMsg;
  msg->n_moves = total_migrates;
  for(int i=0; i < total_migrates; i++) {
    MigrateInfo* item = (MigrateInfo*) migrateInfo[i];
    msg->moves[i] = *item;
    delete item;
    migrateInfo[i] = 0;
  }
  migrateInfo.clear();

#if 1
//  if(total_migrates>0)
  contribute(CkCallback(CkReductionTarget(PrefixLB, SendAfterBarrier),
                thisProxy));
#endif
}

void PrefixLB::SendAfterBarrier(){
  if(CkMyPe()>0)
    thisProxy[CkMyPe()-1].expected(total_migrates);
  if(CkMyPe()==CkNumPes()-1) 
      contribute(CkCallback(CkReductionTarget(PrefixLB, SendAfterBarrier2),
                thisProxy));
}

void PrefixLB::expected(int migrates){
  migrates_expected = migrates;
  contribute(CkCallback(CkReductionTarget(PrefixLB, SendAfterBarrier2),
                thisProxy));
}

void PrefixLB::SendAfterBarrier2(){
  ProcessMigrationDecision(msg);
}
void PrefixLB::receivedPrefixSum(double prefix_load){
  prefix_sum = prefix_load;
  if(CkMyPe()!=CkNumPes()-1)
    thisProxy[CkMyPe()+1].receivedPrefixSum(prefix_sum+my_load);
  CkCallback cb(CkReductionTarget(PrefixLB, FoundPrefix), thisProxy);
  contribute(sizeof(double), &my_load, CkReduction::max_double, cb);
}

#include "PrefixLB.def.h"
