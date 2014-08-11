/* ---------------- method closures -------------- */
#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */


/* DEFS: array LP: ArrayElement{
LP(void);
void recv_event(RemoteEvent* impl_msg);
LP(CkMigrateMessage* impl_msg);
};
 */
#ifndef CK_TEMPLATES_ONLY
 int CkIndex_LP::__idx=0;
#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
/* DEFS: LP(void);
 */

void CProxyElement_LP::insert(int onPE)
{ 
  void *impl_msg = CkAllocSysMsg();
   UsrToEnv(impl_msg)->setMsgtype(ArrayEltInitMsg);
   ckInsert((CkArrayMessage *)impl_msg,CkIndex_LP::idx_LP_void(),onPE);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void recv_event(RemoteEvent* impl_msg);
 */

void CProxyElement_LP::recv_event(RemoteEvent* impl_msg) 
{
  ckCheck();
  UsrToEnv(impl_msg)->setMsgtype(ForArrayEltMsg);
  CkArrayMessage *impl_amsg=(CkArrayMessage *)impl_msg;
  impl_amsg->array_setIfNotThere(CkArray_IfNotThere_buffer);
  ckSend(impl_amsg, CkIndex_LP::idx_recv_event_RemoteEvent(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: LP(CkMigrateMessage* impl_msg);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: LP(void);
 */

CkArrayID CProxy_LP::ckNew(const CkArrayOptions &opts)
{
  void *impl_msg = CkAllocSysMsg();
  UsrToEnv(impl_msg)->setMsgtype(ArrayEltInitMsg);
  return ckCreateArray((CkArrayMessage *)impl_msg, CkIndex_LP::idx_LP_void(), opts);
}

CkArrayID CProxy_LP::ckNew(const int s1)
{
  void *impl_msg = CkAllocSysMsg();
  CkArrayOptions opts(s1);
  UsrToEnv(impl_msg)->setMsgtype(ArrayEltInitMsg);
  return ckCreateArray((CkArrayMessage *)impl_msg, CkIndex_LP::idx_LP_void(), opts);
}

// Entry point registration function

int CkIndex_LP::reg_LP_void() {
  int epidx = CkRegisterEp("LP(void)",
      _call_LP_void, 0, __idx, 0);
  return epidx;
}


void CkIndex_LP::_call_LP_void(void* impl_msg, void* impl_obj_void)
{
  LP* impl_obj = static_cast<LP *>(impl_obj_void);
  CkFreeSysMsg(impl_msg);
  new (impl_obj) LP();
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void recv_event(RemoteEvent* impl_msg);
 */

void CProxy_LP::recv_event(RemoteEvent* impl_msg) 
{
  ckCheck();
  UsrToEnv(impl_msg)->setMsgtype(ForArrayEltMsg);
  CkArrayMessage *impl_amsg=(CkArrayMessage *)impl_msg;
  impl_amsg->array_setIfNotThere(CkArray_IfNotThere_buffer);
  ckBroadcast(impl_amsg, CkIndex_LP::idx_recv_event_RemoteEvent(),0);
}

// Entry point registration function

int CkIndex_LP::reg_recv_event_RemoteEvent() {
  int epidx = CkRegisterEp("recv_event(RemoteEvent* impl_msg)",
      _call_recv_event_RemoteEvent, CMessage_RemoteEvent::__idx, __idx, 0);
  CkRegisterMessagePupFn(epidx, (CkMessagePupFn)RemoteEvent::ckDebugPup);
  return epidx;
}


void CkIndex_LP::_call_recv_event_RemoteEvent(void* impl_msg, void* impl_obj_void)
{
  LP* impl_obj = static_cast<LP *>(impl_obj_void);
  impl_obj->recv_event((RemoteEvent*)impl_msg);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: LP(CkMigrateMessage* impl_msg);
 */

// Entry point registration function

int CkIndex_LP::reg_LP_CkMigrateMessage() {
  int epidx = CkRegisterEp("LP(CkMigrateMessage* impl_msg)",
      _call_LP_CkMigrateMessage, 0, __idx, 0);
  return epidx;
}


void CkIndex_LP::_call_LP_CkMigrateMessage(void* impl_msg, void* impl_obj_void)
{
  LP* impl_obj = static_cast<LP *>(impl_obj_void);
  new (impl_obj) LP((CkMigrateMessage*)impl_msg);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: LP(void);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: void recv_event(RemoteEvent* impl_msg);
 */

void CProxySection_LP::recv_event(RemoteEvent* impl_msg) 
{
  ckCheck();
  UsrToEnv(impl_msg)->setMsgtype(ForArrayEltMsg);
  CkArrayMessage *impl_amsg=(CkArrayMessage *)impl_msg;
  impl_amsg->array_setIfNotThere(CkArray_IfNotThere_buffer);
  ckSend(impl_amsg, CkIndex_LP::idx_recv_event_RemoteEvent(),0);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: LP(CkMigrateMessage* impl_msg);
 */
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
void CkIndex_LP::__register(const char *s, size_t size) {
  __idx = CkRegisterChare(s, size, TypeArray);
  CkRegisterBase(__idx, CkIndex_ArrayElement::__idx);
  // REG: LP(void);
  idx_LP_void();
  CkRegisterDefaultCtor(__idx, idx_LP_void());

  // REG: void recv_event(RemoteEvent* impl_msg);
  idx_recv_event_RemoteEvent();

  // REG: LP(CkMigrateMessage* impl_msg);
  idx_LP_CkMigrateMessage();
  CkRegisterMigCtor(__idx, idx_LP_CkMigrateMessage());

}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
void _registerlp(void)
{
  static int _done = 0; if(_done) return; _done = 1;
/* REG: array LP: ArrayElement{
LP(void);
void recv_event(RemoteEvent* impl_msg);
LP(CkMigrateMessage* impl_msg);
};
*/
  CkIndex_LP::__register("LP", sizeof(LP));

}
#endif /* CK_TEMPLATES_ONLY */
