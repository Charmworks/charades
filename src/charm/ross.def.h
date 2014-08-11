



/* ---------------- method closures -------------- */
#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */





/* DEFS: readonly CProxy_PE pes;
 */
extern CProxy_PE pes;
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_pes(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p|pes;
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: readonly CProxy_LP lps;
 */
extern CProxy_LP lps;
#ifndef CK_TEMPLATES_ONLY
extern "C" void __xlater_roPup_lps(void *_impl_pup_er) {
  PUP::er &_impl_p=*(PUP::er *)_impl_pup_er;
  _impl_p|lps;
}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: mainchare Main: Chare{
Main(CkArgMsg* impl_msg);
Main(CkMigrateMsg* impl_msg);
};
 */
#ifndef CK_TEMPLATES_ONLY
 int CkIndex_Main::__idx=0;
#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
/* DEFS: Main(CkArgMsg* impl_msg);
 */

CkChareID CProxy_Main::ckNew(CkArgMsg* impl_msg, int impl_onPE)
{
  CkChareID impl_ret;
  CkCreateChare(CkIndex_Main::__idx, CkIndex_Main::idx_Main_CkArgMsg(), impl_msg, &impl_ret, impl_onPE);
  return impl_ret;
}

void CProxy_Main::ckNew(CkArgMsg* impl_msg, CkChareID* pcid, int impl_onPE)
{
  CkCreateChare(CkIndex_Main::__idx, CkIndex_Main::idx_Main_CkArgMsg(), impl_msg, pcid, impl_onPE);
}

  CProxy_Main::CProxy_Main(CkArgMsg* impl_msg, int impl_onPE)
{
  CkChareID impl_ret;
  CkCreateChare(CkIndex_Main::__idx, CkIndex_Main::idx_Main_CkArgMsg(), impl_msg, &impl_ret, impl_onPE);
  ckSetChareID(impl_ret);
}

// Entry point registration function

int CkIndex_Main::reg_Main_CkArgMsg() {
  int epidx = CkRegisterEp("Main(CkArgMsg* impl_msg)",
      _call_Main_CkArgMsg, CMessage_CkArgMsg::__idx, __idx, 0);
  CkRegisterMessagePupFn(epidx, (CkMessagePupFn)CkArgMsg::ckDebugPup);
  return epidx;
}


void CkIndex_Main::_call_Main_CkArgMsg(void* impl_msg, void* impl_obj_void)
{
  Main* impl_obj = static_cast<Main *>(impl_obj_void);
  new (impl_obj) Main((CkArgMsg*)impl_msg);
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
/* DEFS: Main(CkMigrateMsg* impl_msg);
 */

CkChareID CProxy_Main::ckNew(CkMigrateMsg* impl_msg, int impl_onPE)
{
  CkChareID impl_ret;
  CkCreateChare(CkIndex_Main::__idx, CkIndex_Main::idx_Main_CkMigrateMsg(), impl_msg, &impl_ret, impl_onPE);
  return impl_ret;
}

void CProxy_Main::ckNew(CkMigrateMsg* impl_msg, CkChareID* pcid, int impl_onPE)
{
  CkCreateChare(CkIndex_Main::__idx, CkIndex_Main::idx_Main_CkMigrateMsg(), impl_msg, pcid, impl_onPE);
}

  CProxy_Main::CProxy_Main(CkMigrateMsg* impl_msg, int impl_onPE)
{
  CkChareID impl_ret;
  CkCreateChare(CkIndex_Main::__idx, CkIndex_Main::idx_Main_CkMigrateMsg(), impl_msg, &impl_ret, impl_onPE);
  ckSetChareID(impl_ret);
}

// Entry point registration function

int CkIndex_Main::reg_Main_CkMigrateMsg() {
  int epidx = CkRegisterEp("Main(CkMigrateMsg* impl_msg)",
      _call_Main_CkMigrateMsg, CMessage_CkMigrateMsg::__idx, __idx, 0);
  CkRegisterMessagePupFn(epidx, (CkMessagePupFn)CkMigrateMsg::ckDebugPup);
  return epidx;
}


void CkIndex_Main::_call_Main_CkMigrateMsg(void* impl_msg, void* impl_obj_void)
{
  Main* impl_obj = static_cast<Main *>(impl_obj_void);
  CkArgMsg *m=(CkArgMsg *)impl_msg;
  new (impl_obj) Main(m->argc,m->argv);
  delete m;
}
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
#endif /* CK_TEMPLATES_ONLY */
#ifndef CK_TEMPLATES_ONLY
void CkIndex_Main::__register(const char *s, size_t size) {
  __idx = CkRegisterChare(s, size, TypeMainChare);
  CkRegisterBase(__idx, CkIndex_Chare::__idx);
  // REG: Main(CkArgMsg* impl_msg);
  idx_Main_CkArgMsg();
  CkRegisterMainChare(__idx, idx_Main_CkArgMsg());

  // REG: Main(CkMigrateMsg* impl_msg);
  idx_Main_CkMigrateMsg();
  CkRegisterMainChare(__idx, idx_Main_CkMigrateMsg());

}
#endif /* CK_TEMPLATES_ONLY */

/* DEFS: message RemoteEvent{
char userData[];
}
;
 */
#ifndef CK_TEMPLATES_ONLY
void *CMessage_RemoteEvent::operator new(size_t s){
  return RemoteEvent::alloc(__idx, s, 0, 0);
}
void *CMessage_RemoteEvent::operator new(size_t s, int* sz){
  return RemoteEvent::alloc(__idx, s, sz, 0);
}
void *CMessage_RemoteEvent::operator new(size_t s, int* sz,const int pb){
  return RemoteEvent::alloc(__idx, s, sz, pb);
}
void *CMessage_RemoteEvent::operator new(size_t s, int sz0) {
  int sizes[1];
  sizes[0] = sz0;
  return RemoteEvent::alloc(__idx, s, sizes, 0);
}
void *CMessage_RemoteEvent::operator new(size_t s, int sz0, const int p) {
  int sizes[1];
  sizes[0] = sz0;
  return RemoteEvent::alloc(__idx, s, sizes, p);
}
void* CMessage_RemoteEvent::alloc(int msgnum, size_t sz, int *sizes, int pb) {
  CkpvAccess(_offsets)[0] = ALIGN_DEFAULT(sz);
  if(sizes==0)
    CkpvAccess(_offsets)[1] = CkpvAccess(_offsets)[0];
  else
    CkpvAccess(_offsets)[1] = CkpvAccess(_offsets)[0] + ALIGN_DEFAULT(sizeof(char)*sizes[0]);
  return CkAllocMsg(msgnum, CkpvAccess(_offsets)[1], pb);
}
CMessage_RemoteEvent::CMessage_RemoteEvent() {
RemoteEvent *newmsg = (RemoteEvent *)this;
  newmsg->userData = (char *) ((char *)newmsg + CkpvAccess(_offsets)[0]);
}
void CMessage_RemoteEvent::dealloc(void *p) {
  CkFreeMsg(p);
}
void* CMessage_RemoteEvent::pack(RemoteEvent *msg) {
  msg->userData = (char *) ((char *)msg->userData - (char *)msg);
  return (void *) msg;
}
RemoteEvent* CMessage_RemoteEvent::unpack(void* buf) {
  RemoteEvent *msg = (RemoteEvent *) buf;
  msg->userData = (char *) ((size_t)msg->userData + (char *)msg);
  return msg;
}
int CMessage_RemoteEvent::__idx=0;
#endif /* CK_TEMPLATES_ONLY */

#ifndef CK_TEMPLATES_ONLY
void _registerross(void)
{
  static int _done = 0; if(_done) return; _done = 1;
  _registerpe();

  _registerlp();

  CkRegisterReadonly("pes","CProxy_PE",sizeof(pes),(void *) &pes,__xlater_roPup_pes);

  CkRegisterReadonly("lps","CProxy_LP",sizeof(lps),(void *) &lps,__xlater_roPup_lps);

/* REG: mainchare Main: Chare{
Main(CkArgMsg* impl_msg);
Main(CkMigrateMsg* impl_msg);
};
*/
  CkIndex_Main::__register("Main", sizeof(Main));

/* REG: message RemoteEvent{
char userData[];
}
;
*/
CMessage_RemoteEvent::__register("RemoteEvent", sizeof(RemoteEvent),(CkPackFnPtr) RemoteEvent::pack,(CkUnpackFnPtr) RemoteEvent::unpack);

}
extern "C" void CkRegisterMainModule(void) {
  _registerross();
}
#endif /* CK_TEMPLATES_ONLY */
