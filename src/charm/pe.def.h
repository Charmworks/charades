/* ---------------- method closures -------------- */
#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY

    struct Closure_PE::GVT_begin_3_closure : public SDAG::Closure {
      

      GVT_begin_3_closure() {
        init();
      }
      GVT_begin_3_closure(CkMigrateMessage*) {
        init();
      }
            void pup(PUP::er& __p) {
        packClosure(__p);
      }
      virtual ~GVT_begin_3_closure() {
      }
      PUPable_decl(SINGLE_ARG(GVT_begin_3_closure));
    };
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY

    struct Closure_PE::GVT_contribute_4_closure : public SDAG::Closure {
      

      GVT_contribute_4_closure() {
        init();
      }
      GVT_contribute_4_closure(CkMigrateMessage*) {
        init();
      }
            void pup(PUP::er& __p) {
        packClosure(__p);
      }
      virtual ~GVT_contribute_4_closure() {
      }
      PUPable_decl(SINGLE_ARG(GVT_contribute_4_closure));
    };
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY

    struct Closure_PE::GVT_end_5_closure : public SDAG::Closure {
      

      GVT_end_5_closure() {
        init();
      }
      GVT_end_5_closure(CkMigrateMessage*) {
        init();
      }
            void pup(PUP::er& __p) {
        packClosure(__p);
      }
      virtual ~GVT_end_5_closure() {
      }
      PUPable_decl(SINGLE_ARG(GVT_end_5_closure));
    };
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY

    struct Closure_PE::execute_seq_6_closure : public SDAG::Closure {
      

      execute_seq_6_closure() {
        init();
      }
      execute_seq_6_closure(CkMigrateMessage*) {
        init();
      }
            void pup(PUP::er& __p) {
        packClosure(__p);
      }
      virtual ~execute_seq_6_closure() {
      }
      PUPable_decl(SINGLE_ARG(execute_seq_6_closure));
    };
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY

    struct Closure_PE::execute_cons_7_closure : public SDAG::Closure {
      

      execute_cons_7_closure() {
        init();
      }
      execute_cons_7_closure(CkMigrateMessage*) {
        init();
      }
            void pup(PUP::er& __p) {
        packClosure(__p);
      }
      virtual ~execute_cons_7_closure() {
      }
      PUPable_decl(SINGLE_ARG(execute_cons_7_closure));
    };
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY

    struct Closure_PE::execute_opt_8_closure : public SDAG::Closure {
      

      execute_opt_8_closure() {
        init();
      }
      execute_opt_8_closure(CkMigrateMessage*) {
        init();
      }
            void pup(PUP::er& __p) {
        packClosure(__p);
      }
      virtual ~execute_opt_8_closure() {
      }
      PUPable_decl(SINGLE_ARG(execute_opt_8_closure));
    };
#endif /* CK_TEMPLATES_ONLY */


/* DEFS: group PE: IrrGroup{
PE(void);
PE(int impl_noname_0, int impl_noname_1);
void GVT_begin(void);
void GVT_contribute(void);
void GVT_end(void);
void execute_seq(void);
void execute_cons(void);
void execute_opt(void);
};
 */
#ifndef CK_TEMPLATES_ONLY
 int CkIndex_PE::__idx=0;
#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
/* DEFS: PE(void);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: PE(int impl_noname_0, int impl_noname_1);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_begin(void);
 */

void CProxyElement_PE::GVT_begin(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSend(ckDelegatedPtr(),CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupPe(), ckGetGroupID());
  } else CkSendMsgBranch(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupPe(), ckGetGroupID(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_contribute(void);
 */

void CProxyElement_PE::GVT_contribute(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSend(ckDelegatedPtr(),CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupPe(), ckGetGroupID());
  } else CkSendMsgBranch(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupPe(), ckGetGroupID(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_end(void);
 */

void CProxyElement_PE::GVT_end(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSend(ckDelegatedPtr(),CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupPe(), ckGetGroupID());
  } else CkSendMsgBranch(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupPe(), ckGetGroupID(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_seq(void);
 */

void CProxyElement_PE::execute_seq(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSend(ckDelegatedPtr(),CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupPe(), ckGetGroupID());
  } else CkSendMsgBranch(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupPe(), ckGetGroupID(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_cons(void);
 */

void CProxyElement_PE::execute_cons(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSend(ckDelegatedPtr(),CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupPe(), ckGetGroupID());
  } else CkSendMsgBranch(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupPe(), ckGetGroupID(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_opt(void);
 */

void CProxyElement_PE::execute_opt(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSend(ckDelegatedPtr(),CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupPe(), ckGetGroupID());
  } else CkSendMsgBranch(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupPe(), ckGetGroupID(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: PE(void);
 */

CkGroupID CProxy_PE::ckNew(void)
{
  void *impl_msg = CkAllocSysMsg();
  UsrToEnv(impl_msg)->setMsgtype(BocInitMsg);
  return CkCreateGroup(CkIndex_PE::__idx, CkIndex_PE::idx_PE_void(), impl_msg);
}

// Entry point registration function

int CkIndex_PE::reg_PE_void() {
  int epidx = CkRegisterEp("PE(void)",
      _call_PE_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_PE_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  new (impl_obj) PE();
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: PE(int impl_noname_0, int impl_noname_1);
 */

CkGroupID CProxy_PE::ckNew(int impl_noname_0, int impl_noname_1, const CkEntryOptions *impl_e_opts)
{
  //Marshall: int impl_noname_0, int impl_noname_1
  int impl_off=0;
  { //Find the size of the PUP'd data
    PUP::sizer implP;
    implP|impl_noname_0;
    implP|impl_noname_1;
    impl_off+=implP.size();
  }
  CkMarshallMsg *impl_msg=CkAllocateMarshallMsg(impl_off,impl_e_opts);
  { //Copy over the PUP'd data
    PUP::toMem implP((void *)impl_msg->msgBuf);
    implP|impl_noname_0;
    implP|impl_noname_1;
  }
  UsrToEnv(impl_msg)->setMsgtype(BocInitMsg);
  if (impl_e_opts)
    UsrToEnv(impl_msg)->setGroupDep(impl_e_opts->getGroupDepID());
  return CkCreateGroup(CkIndex_PE::__idx, CkIndex_PE::idx_PE_marshall2(), impl_msg);
}

  CProxy_PE::CProxy_PE(int impl_noname_0, int impl_noname_1, const CkEntryOptions *impl_e_opts)
{
  //Marshall: int impl_noname_0, int impl_noname_1
  int impl_off=0;
  { //Find the size of the PUP'd data
    PUP::sizer implP;
    implP|impl_noname_0;
    implP|impl_noname_1;
    impl_off+=implP.size();
  }
  CkMarshallMsg *impl_msg=CkAllocateMarshallMsg(impl_off,impl_e_opts);
  { //Copy over the PUP'd data
    PUP::toMem implP((void *)impl_msg->msgBuf);
    implP|impl_noname_0;
    implP|impl_noname_1;
  }
  UsrToEnv(impl_msg)->setMsgtype(BocInitMsg);
  if (impl_e_opts)
    UsrToEnv(impl_msg)->setGroupDep(impl_e_opts->getGroupDepID());
  ckSetGroupID(CkCreateGroup(CkIndex_PE::__idx, CkIndex_PE::idx_PE_marshall2(), impl_msg));
}

// Entry point registration function

int CkIndex_PE::reg_PE_marshall2() {
  int epidx = CkRegisterEp("PE(int impl_noname_0, int impl_noname_1)",
      _call_PE_marshall2, CkMarshallMsg::__idx, __idx, 0+CK_EP_NOKEEP);
  CkRegisterMarshallUnpackFn(epidx, _callmarshall_PE_marshall2);
  CkRegisterMessagePupFn(epidx, _marshallmessagepup_PE_marshall2);

  return epidx;
}


void CkIndex_PE::_call_PE_marshall2(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkMarshallMsg *impl_msg_typed=(CkMarshallMsg *)impl_msg;
  char *impl_buf=impl_msg_typed->msgBuf;
  /*Unmarshall pup'd fields: int impl_noname_0, int impl_noname_1*/
  PUP::fromMem implP(impl_buf);
  int impl_noname_0; implP|impl_noname_0;
  int impl_noname_1; implP|impl_noname_1;
  impl_buf+=CK_ALIGN(implP.size(),16);
  /*Unmarshall arrays:*/
  new (impl_obj) PE(impl_noname_0, impl_noname_1);
}

int CkIndex_PE::_callmarshall_PE_marshall2(char* impl_buf, void* impl_obj_void) {
  PE* impl_obj = static_cast< PE *>(impl_obj_void);
  /*Unmarshall pup'd fields: int impl_noname_0, int impl_noname_1*/
  PUP::fromMem implP(impl_buf);
  int impl_noname_0; implP|impl_noname_0;
  int impl_noname_1; implP|impl_noname_1;
  impl_buf+=CK_ALIGN(implP.size(),16);
  /*Unmarshall arrays:*/
  new (impl_obj) PE(impl_noname_0, impl_noname_1);
  return implP.size();
}

void CkIndex_PE::_marshallmessagepup_PE_marshall2(PUP::er &implDestP,void *impl_msg) {
  CkMarshallMsg *impl_msg_typed=(CkMarshallMsg *)impl_msg;
  char *impl_buf=impl_msg_typed->msgBuf;
  /*Unmarshall pup'd fields: int impl_noname_0, int impl_noname_1*/
  PUP::fromMem implP(impl_buf);
  int impl_noname_0; implP|impl_noname_0;
  int impl_noname_1; implP|impl_noname_1;
  impl_buf+=CK_ALIGN(implP.size(),16);
  /*Unmarshall arrays:*/
  if (implDestP.hasComments()) implDestP.comment("impl_noname_0");
  implDestP|impl_noname_0;
  if (implDestP.hasComments()) implDestP.comment("impl_noname_1");
  implDestP|impl_noname_1;
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_begin(void);
 */

void CProxy_PE::GVT_begin(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupBroadcast(ckDelegatedPtr(),CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID());
  } else CkBroadcastMsgBranch(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID(),0);
}

void CProxy_PE::GVT_begin(int npes, int *pes) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchMulti(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID(), npes, pes,0);
}

void CProxy_PE::GVT_begin(CmiGroup &grp) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchGroup(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID(), grp,0);
}

// Entry point registration function

int CkIndex_PE::reg_GVT_begin_void() {
  int epidx = CkRegisterEp("GVT_begin(void)",
      _call_GVT_begin_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_GVT_begin_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  impl_obj->GVT_begin();
}
PUPable_def(SINGLE_ARG(Closure_PE::GVT_begin_3_closure));
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_contribute(void);
 */

void CProxy_PE::GVT_contribute(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupBroadcast(ckDelegatedPtr(),CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID());
  } else CkBroadcastMsgBranch(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID(),0);
}

void CProxy_PE::GVT_contribute(int npes, int *pes) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchMulti(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID(), npes, pes,0);
}

void CProxy_PE::GVT_contribute(CmiGroup &grp) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchGroup(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID(), grp,0);
}

// Entry point registration function

int CkIndex_PE::reg_GVT_contribute_void() {
  int epidx = CkRegisterEp("GVT_contribute(void)",
      _call_GVT_contribute_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_GVT_contribute_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  impl_obj->GVT_contribute();
}
PUPable_def(SINGLE_ARG(Closure_PE::GVT_contribute_4_closure));
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_end(void);
 */

void CProxy_PE::GVT_end(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupBroadcast(ckDelegatedPtr(),CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID());
  } else CkBroadcastMsgBranch(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID(),0);
}

void CProxy_PE::GVT_end(int npes, int *pes) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchMulti(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID(), npes, pes,0);
}

void CProxy_PE::GVT_end(CmiGroup &grp) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchGroup(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID(), grp,0);
}

// Entry point registration function

int CkIndex_PE::reg_GVT_end_void() {
  int epidx = CkRegisterEp("GVT_end(void)",
      _call_GVT_end_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_GVT_end_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  impl_obj->GVT_end();
}
PUPable_def(SINGLE_ARG(Closure_PE::GVT_end_5_closure));
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_seq(void);
 */

void CProxy_PE::execute_seq(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupBroadcast(ckDelegatedPtr(),CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID());
  } else CkBroadcastMsgBranch(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID(),0);
}

void CProxy_PE::execute_seq(int npes, int *pes) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchMulti(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID(), npes, pes,0);
}

void CProxy_PE::execute_seq(CmiGroup &grp) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchGroup(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID(), grp,0);
}

// Entry point registration function

int CkIndex_PE::reg_execute_seq_void() {
  int epidx = CkRegisterEp("execute_seq(void)",
      _call_execute_seq_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_execute_seq_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  impl_obj->execute_seq();
}
PUPable_def(SINGLE_ARG(Closure_PE::execute_seq_6_closure));
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_cons(void);
 */

void CProxy_PE::execute_cons(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupBroadcast(ckDelegatedPtr(),CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID());
  } else CkBroadcastMsgBranch(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID(),0);
}

void CProxy_PE::execute_cons(int npes, int *pes) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchMulti(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID(), npes, pes,0);
}

void CProxy_PE::execute_cons(CmiGroup &grp) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchGroup(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID(), grp,0);
}

// Entry point registration function

int CkIndex_PE::reg_execute_cons_void() {
  int epidx = CkRegisterEp("execute_cons(void)",
      _call_execute_cons_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_execute_cons_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  impl_obj->execute_cons();
}
PUPable_def(SINGLE_ARG(Closure_PE::execute_cons_7_closure));
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_opt(void);
 */

void CProxy_PE::execute_opt(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupBroadcast(ckDelegatedPtr(),CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID());
  } else CkBroadcastMsgBranch(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID(),0);
}

void CProxy_PE::execute_opt(int npes, int *pes) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchMulti(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID(), npes, pes,0);
}

void CProxy_PE::execute_opt(CmiGroup &grp) {
  void *impl_msg = CkAllocSysMsg();
  CkSendMsgBranchGroup(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID(), grp,0);
}

// Entry point registration function

int CkIndex_PE::reg_execute_opt_void() {
  int epidx = CkRegisterEp("execute_opt(void)",
      _call_execute_opt_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_PE::_call_execute_opt_void(void* impl_msg, void* impl_obj_void)
{
  PE* impl_obj = static_cast<PE *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  impl_obj->execute_opt();
}
PUPable_def(SINGLE_ARG(Closure_PE::execute_opt_8_closure));
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: PE(void);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: PE(int impl_noname_0, int impl_noname_1);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_begin(void);
 */

void CProxySection_PE::GVT_begin(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSectionSend(ckDelegatedPtr(),CkIndex_PE::idx_GVT_begin_void(), impl_msg, ckGetNumSections(), ckGetSectionIDs());
  } else {
    void *impl_msg_tmp = (ckGetNumSections()>1) ? CkCopyMsg((void **) &impl_msg) : impl_msg;
    for (int i=0; i<ckGetNumSections(); ++i) {
       impl_msg_tmp= (i<ckGetNumSections()-1) ? CkCopyMsg((void **) &impl_msg):impl_msg;
       CkSendMsgBranchMulti(CkIndex_PE::idx_GVT_begin_void(), impl_msg_tmp, ckGetGroupIDn(i), ckGetNumElements(i), ckGetElements(i),0);
    }
  }
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_contribute(void);
 */

void CProxySection_PE::GVT_contribute(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSectionSend(ckDelegatedPtr(),CkIndex_PE::idx_GVT_contribute_void(), impl_msg, ckGetNumSections(), ckGetSectionIDs());
  } else {
    void *impl_msg_tmp = (ckGetNumSections()>1) ? CkCopyMsg((void **) &impl_msg) : impl_msg;
    for (int i=0; i<ckGetNumSections(); ++i) {
       impl_msg_tmp= (i<ckGetNumSections()-1) ? CkCopyMsg((void **) &impl_msg):impl_msg;
       CkSendMsgBranchMulti(CkIndex_PE::idx_GVT_contribute_void(), impl_msg_tmp, ckGetGroupIDn(i), ckGetNumElements(i), ckGetElements(i),0);
    }
  }
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void GVT_end(void);
 */

void CProxySection_PE::GVT_end(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSectionSend(ckDelegatedPtr(),CkIndex_PE::idx_GVT_end_void(), impl_msg, ckGetNumSections(), ckGetSectionIDs());
  } else {
    void *impl_msg_tmp = (ckGetNumSections()>1) ? CkCopyMsg((void **) &impl_msg) : impl_msg;
    for (int i=0; i<ckGetNumSections(); ++i) {
       impl_msg_tmp= (i<ckGetNumSections()-1) ? CkCopyMsg((void **) &impl_msg):impl_msg;
       CkSendMsgBranchMulti(CkIndex_PE::idx_GVT_end_void(), impl_msg_tmp, ckGetGroupIDn(i), ckGetNumElements(i), ckGetElements(i),0);
    }
  }
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_seq(void);
 */

void CProxySection_PE::execute_seq(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSectionSend(ckDelegatedPtr(),CkIndex_PE::idx_execute_seq_void(), impl_msg, ckGetNumSections(), ckGetSectionIDs());
  } else {
    void *impl_msg_tmp = (ckGetNumSections()>1) ? CkCopyMsg((void **) &impl_msg) : impl_msg;
    for (int i=0; i<ckGetNumSections(); ++i) {
       impl_msg_tmp= (i<ckGetNumSections()-1) ? CkCopyMsg((void **) &impl_msg):impl_msg;
       CkSendMsgBranchMulti(CkIndex_PE::idx_execute_seq_void(), impl_msg_tmp, ckGetGroupIDn(i), ckGetNumElements(i), ckGetElements(i),0);
    }
  }
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_cons(void);
 */

void CProxySection_PE::execute_cons(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSectionSend(ckDelegatedPtr(),CkIndex_PE::idx_execute_cons_void(), impl_msg, ckGetNumSections(), ckGetSectionIDs());
  } else {
    void *impl_msg_tmp = (ckGetNumSections()>1) ? CkCopyMsg((void **) &impl_msg) : impl_msg;
    for (int i=0; i<ckGetNumSections(); ++i) {
       impl_msg_tmp= (i<ckGetNumSections()-1) ? CkCopyMsg((void **) &impl_msg):impl_msg;
       CkSendMsgBranchMulti(CkIndex_PE::idx_execute_cons_void(), impl_msg_tmp, ckGetGroupIDn(i), ckGetNumElements(i), ckGetElements(i),0);
    }
  }
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void execute_opt(void);
 */

void CProxySection_PE::execute_opt(void)
{
  ckCheck();
  void *impl_msg = CkAllocSysMsg();
  if (ckIsDelegated()) {
     CkGroupMsgPrep(CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetGroupID());
     ckDelegatedTo()->GroupSectionSend(ckDelegatedPtr(),CkIndex_PE::idx_execute_opt_void(), impl_msg, ckGetNumSections(), ckGetSectionIDs());
  } else {
    void *impl_msg_tmp = (ckGetNumSections()>1) ? CkCopyMsg((void **) &impl_msg) : impl_msg;
    for (int i=0; i<ckGetNumSections(); ++i) {
       impl_msg_tmp= (i<ckGetNumSections()-1) ? CkCopyMsg((void **) &impl_msg):impl_msg;
       CkSendMsgBranchMulti(CkIndex_PE::idx_execute_opt_void(), impl_msg_tmp, ckGetGroupIDn(i), ckGetNumElements(i), ckGetElements(i),0);
    }
  }
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
void CkIndex_PE::__register(const char *s, size_t size) {
  __idx = CkRegisterChare(s, size, TypeGroup);
  CkRegisterBase(__idx, CkIndex_IrrGroup::__idx);
   CkRegisterGroupIrr(__idx,PE::isIrreducible());
  // REG: PE(void);
  idx_PE_void();
  CkRegisterDefaultCtor(__idx, idx_PE_void());

  // REG: PE(int impl_noname_0, int impl_noname_1);
  idx_PE_marshall2();

  // REG: void GVT_begin(void);
  idx_GVT_begin_void();

  // REG: void GVT_contribute(void);
  idx_GVT_contribute_void();

  // REG: void GVT_end(void);
  idx_GVT_end_void();

  // REG: void execute_seq(void);
  idx_execute_seq_void();

  // REG: void execute_cons(void);
  idx_execute_cons_void();

  // REG: void execute_opt(void);
  idx_execute_opt_void();

}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
void _registerpe(void)
{
  static int _done = 0; if(_done) return; _done = 1;
/* REG: group PE: IrrGroup{
PE(void);
PE(int impl_noname_0, int impl_noname_1);
void GVT_begin(void);
void GVT_contribute(void);
void GVT_end(void);
void execute_seq(void);
void execute_cons(void);
void execute_opt(void);
};
*/
  CkIndex_PE::__register("PE", sizeof(PE));

}
#endif /* CK_TEMPLATES_ONLY */
