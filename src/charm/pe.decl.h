#ifndef _DECL_pe_H_
#define _DECL_pe_H_
#include "charm++.h"
#include "envelope.h"
#include <memory>
#include "sdag.h"
/* DECLS: group PE: IrrGroup{
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
 class PE;
 class CkIndex_PE;
 class CProxy_PE;
 class CProxyElement_PE;
 class CProxySection_PE;
/* --------------- index object ------------------ */
class CkIndex_PE:public CkIndex_IrrGroup{
  public:
    typedef PE local_t;
    typedef CkIndex_PE index_t;
    typedef CProxy_PE proxy_t;
    typedef CProxyElement_PE element_t;
    typedef CProxySection_PE section_t;

    static int __idx;
    static void __register(const char *s, size_t size);
    /* DECLS: PE(void);
     */
    // Entry point registration at startup
    
    static int reg_PE_void();
    // Entry point index lookup
    
    inline static int idx_PE_void() {
      static int epidx = reg_PE_void();
      return epidx;
    }

    
    static int ckNew(void) { return idx_PE_void(); }
    
    static void _call_PE_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_PE_void(void* impl_msg, void* impl_obj);
    /* DECLS: PE(int impl_noname_0, int impl_noname_1);
     */
    // Entry point registration at startup
    
    static int reg_PE_marshall2();
    // Entry point index lookup
    
    inline static int idx_PE_marshall2() {
      static int epidx = reg_PE_marshall2();
      return epidx;
    }

    
    static int ckNew(int impl_noname_0, int impl_noname_1) { return idx_PE_marshall2(); }
    
    static void _call_PE_marshall2(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_PE_marshall2(void* impl_msg, void* impl_obj);
    
    static int _callmarshall_PE_marshall2(char* impl_buf, void* impl_obj_void);
    
    static void _marshallmessagepup_PE_marshall2(PUP::er &p,void *msg);
    /* DECLS: void GVT_begin(void);
     */
    // Entry point registration at startup
    
    static int reg_GVT_begin_void();
    // Entry point index lookup
    
    inline static int idx_GVT_begin_void() {
      static int epidx = reg_GVT_begin_void();
      return epidx;
    }

    
    inline static int idx_GVT_begin(void (PE::*)(void) ) {
      return idx_GVT_begin_void();
    }


    
    static int GVT_begin(void) { return idx_GVT_begin_void(); }
    
    static void _call_GVT_begin_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_GVT_begin_void(void* impl_msg, void* impl_obj);
    /* DECLS: void GVT_contribute(void);
     */
    // Entry point registration at startup
    
    static int reg_GVT_contribute_void();
    // Entry point index lookup
    
    inline static int idx_GVT_contribute_void() {
      static int epidx = reg_GVT_contribute_void();
      return epidx;
    }

    
    inline static int idx_GVT_contribute(void (PE::*)(void) ) {
      return idx_GVT_contribute_void();
    }


    
    static int GVT_contribute(void) { return idx_GVT_contribute_void(); }
    
    static void _call_GVT_contribute_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_GVT_contribute_void(void* impl_msg, void* impl_obj);
    /* DECLS: void GVT_end(void);
     */
    // Entry point registration at startup
    
    static int reg_GVT_end_void();
    // Entry point index lookup
    
    inline static int idx_GVT_end_void() {
      static int epidx = reg_GVT_end_void();
      return epidx;
    }

    
    inline static int idx_GVT_end(void (PE::*)(void) ) {
      return idx_GVT_end_void();
    }


    
    static int GVT_end(void) { return idx_GVT_end_void(); }
    
    static void _call_GVT_end_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_GVT_end_void(void* impl_msg, void* impl_obj);
    /* DECLS: void execute_seq(void);
     */
    // Entry point registration at startup
    
    static int reg_execute_seq_void();
    // Entry point index lookup
    
    inline static int idx_execute_seq_void() {
      static int epidx = reg_execute_seq_void();
      return epidx;
    }

    
    inline static int idx_execute_seq(void (PE::*)(void) ) {
      return idx_execute_seq_void();
    }


    
    static int execute_seq(void) { return idx_execute_seq_void(); }
    
    static void _call_execute_seq_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_execute_seq_void(void* impl_msg, void* impl_obj);
    /* DECLS: void execute_cons(void);
     */
    // Entry point registration at startup
    
    static int reg_execute_cons_void();
    // Entry point index lookup
    
    inline static int idx_execute_cons_void() {
      static int epidx = reg_execute_cons_void();
      return epidx;
    }

    
    inline static int idx_execute_cons(void (PE::*)(void) ) {
      return idx_execute_cons_void();
    }


    
    static int execute_cons(void) { return idx_execute_cons_void(); }
    
    static void _call_execute_cons_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_execute_cons_void(void* impl_msg, void* impl_obj);
    /* DECLS: void execute_opt(void);
     */
    // Entry point registration at startup
    
    static int reg_execute_opt_void();
    // Entry point index lookup
    
    inline static int idx_execute_opt_void() {
      static int epidx = reg_execute_opt_void();
      return epidx;
    }

    
    inline static int idx_execute_opt(void (PE::*)(void) ) {
      return idx_execute_opt_void();
    }


    
    static int execute_opt(void) { return idx_execute_opt_void(); }
    
    static void _call_execute_opt_void(void* impl_msg, void* impl_obj);
    
    static void _call_sdag_execute_opt_void(void* impl_msg, void* impl_obj);
};
/* --------------- element proxy ------------------ */
class CProxyElement_PE: public CProxyElement_IrrGroup{
  public:
    typedef PE local_t;
    typedef CkIndex_PE index_t;
    typedef CProxy_PE proxy_t;
    typedef CProxyElement_PE element_t;
    typedef CProxySection_PE section_t;

    CProxyElement_PE(void) {}
    CProxyElement_PE(const IrrGroup *g) : CProxyElement_IrrGroup(g){  }
    CProxyElement_PE(CkGroupID _gid,int _onPE,CK_DELCTOR_PARAM) : CProxyElement_IrrGroup(_gid,_onPE,CK_DELCTOR_ARGS){  }
    CProxyElement_PE(CkGroupID _gid,int _onPE) : CProxyElement_IrrGroup(_gid,_onPE){  }

    int ckIsDelegated(void) const
    { return CProxyElement_IrrGroup::ckIsDelegated(); }
    inline CkDelegateMgr *ckDelegatedTo(void) const
    { return CProxyElement_IrrGroup::ckDelegatedTo(); }
    inline CkDelegateData *ckDelegatedPtr(void) const
    { return CProxyElement_IrrGroup::ckDelegatedPtr(); }
    CkGroupID ckDelegatedIdx(void) const
    { return CProxyElement_IrrGroup::ckDelegatedIdx(); }
inline void ckCheck(void) const {CProxyElement_IrrGroup::ckCheck();}
CkChareID ckGetChareID(void) const
   {return CProxyElement_IrrGroup::ckGetChareID();}
CkGroupID ckGetGroupID(void) const
   {return CProxyElement_IrrGroup::ckGetGroupID();}
operator CkGroupID () const { return ckGetGroupID(); }

    inline void setReductionClient(CkReductionClientFn fn,void *param=NULL) const
    { CProxyElement_IrrGroup::setReductionClient(fn,param); }
    inline void ckSetReductionClient(CkReductionClientFn fn,void *param=NULL) const
    { CProxyElement_IrrGroup::ckSetReductionClient(fn,param); }
    inline void ckSetReductionClient(CkCallback *cb) const
    { CProxyElement_IrrGroup::ckSetReductionClient(cb); }
int ckGetGroupPe(void) const
{return CProxyElement_IrrGroup::ckGetGroupPe();}

    void ckDelegate(CkDelegateMgr *dTo,CkDelegateData *dPtr=NULL)
    {       CProxyElement_IrrGroup::ckDelegate(dTo,dPtr); }
    void ckUndelegate(void)
    {       CProxyElement_IrrGroup::ckUndelegate(); }
    void pup(PUP::er &p)
    {       CProxyElement_IrrGroup::pup(p); }
    void ckSetGroupID(CkGroupID g) {
      CProxyElement_IrrGroup::ckSetGroupID(g);
    }
    PE* ckLocalBranch(void) const {
      return ckLocalBranch(ckGetGroupID());
    }
    static PE* ckLocalBranch(CkGroupID gID) {
      return (PE*)CkLocalBranch(gID);
    }
/* DECLS: PE(void);
 */
    

/* DECLS: PE(int impl_noname_0, int impl_noname_1);
 */
    

/* DECLS: void GVT_begin(void);
 */
    
    void GVT_begin(void);

/* DECLS: void GVT_contribute(void);
 */
    
    void GVT_contribute(void);

/* DECLS: void GVT_end(void);
 */
    
    void GVT_end(void);

/* DECLS: void execute_seq(void);
 */
    
    void execute_seq(void);

/* DECLS: void execute_cons(void);
 */
    
    void execute_cons(void);

/* DECLS: void execute_opt(void);
 */
    
    void execute_opt(void);

};
PUPmarshall(CProxyElement_PE)
/* ---------------- collective proxy -------------- */
class CProxy_PE: public CProxy_IrrGroup{
  public:
    typedef PE local_t;
    typedef CkIndex_PE index_t;
    typedef CProxy_PE proxy_t;
    typedef CProxyElement_PE element_t;
    typedef CProxySection_PE section_t;

    CProxy_PE(void) {}
    CProxy_PE(const IrrGroup *g) : CProxy_IrrGroup(g){  }
    CProxy_PE(CkGroupID _gid,CK_DELCTOR_PARAM) : CProxy_IrrGroup(_gid,CK_DELCTOR_ARGS){  }
    CProxy_PE(CkGroupID _gid) : CProxy_IrrGroup(_gid){  }
    CProxyElement_PE operator[](int onPE) const
      {return CProxyElement_PE(ckGetGroupID(),onPE,CK_DELCTOR_CALL);}

    int ckIsDelegated(void) const
    { return CProxy_IrrGroup::ckIsDelegated(); }
    inline CkDelegateMgr *ckDelegatedTo(void) const
    { return CProxy_IrrGroup::ckDelegatedTo(); }
    inline CkDelegateData *ckDelegatedPtr(void) const
    { return CProxy_IrrGroup::ckDelegatedPtr(); }
    CkGroupID ckDelegatedIdx(void) const
    { return CProxy_IrrGroup::ckDelegatedIdx(); }
inline void ckCheck(void) const {CProxy_IrrGroup::ckCheck();}
CkChareID ckGetChareID(void) const
   {return CProxy_IrrGroup::ckGetChareID();}
CkGroupID ckGetGroupID(void) const
   {return CProxy_IrrGroup::ckGetGroupID();}
operator CkGroupID () const { return ckGetGroupID(); }

    inline void setReductionClient(CkReductionClientFn fn,void *param=NULL) const
    { CProxy_IrrGroup::setReductionClient(fn,param); }
    inline void ckSetReductionClient(CkReductionClientFn fn,void *param=NULL) const
    { CProxy_IrrGroup::ckSetReductionClient(fn,param); }
    inline void ckSetReductionClient(CkCallback *cb) const
    { CProxy_IrrGroup::ckSetReductionClient(cb); }

    void ckDelegate(CkDelegateMgr *dTo,CkDelegateData *dPtr=NULL)
    {       CProxy_IrrGroup::ckDelegate(dTo,dPtr); }
    void ckUndelegate(void)
    {       CProxy_IrrGroup::ckUndelegate(); }
    void pup(PUP::er &p)
    {       CProxy_IrrGroup::pup(p); }
    void ckSetGroupID(CkGroupID g) {
      CProxy_IrrGroup::ckSetGroupID(g);
    }
    PE* ckLocalBranch(void) const {
      return ckLocalBranch(ckGetGroupID());
    }
    static PE* ckLocalBranch(CkGroupID gID) {
      return (PE*)CkLocalBranch(gID);
    }
/* DECLS: PE(void);
 */
    
    static CkGroupID ckNew(void);

/* DECLS: PE(int impl_noname_0, int impl_noname_1);
 */
    
    static CkGroupID ckNew(int impl_noname_0, int impl_noname_1, const CkEntryOptions *impl_e_opts=NULL);
    CProxy_PE(int impl_noname_0, int impl_noname_1, const CkEntryOptions *impl_e_opts=NULL);

/* DECLS: void GVT_begin(void);
 */
    
    void GVT_begin(void);
    
    void GVT_begin(int npes, int *pes);
    
    void GVT_begin(CmiGroup &grp);

/* DECLS: void GVT_contribute(void);
 */
    
    void GVT_contribute(void);
    
    void GVT_contribute(int npes, int *pes);
    
    void GVT_contribute(CmiGroup &grp);

/* DECLS: void GVT_end(void);
 */
    
    void GVT_end(void);
    
    void GVT_end(int npes, int *pes);
    
    void GVT_end(CmiGroup &grp);

/* DECLS: void execute_seq(void);
 */
    
    void execute_seq(void);
    
    void execute_seq(int npes, int *pes);
    
    void execute_seq(CmiGroup &grp);

/* DECLS: void execute_cons(void);
 */
    
    void execute_cons(void);
    
    void execute_cons(int npes, int *pes);
    
    void execute_cons(CmiGroup &grp);

/* DECLS: void execute_opt(void);
 */
    
    void execute_opt(void);
    
    void execute_opt(int npes, int *pes);
    
    void execute_opt(CmiGroup &grp);

};
PUPmarshall(CProxy_PE)
/* ---------------- section proxy -------------- */
class CProxySection_PE: public CProxySection_IrrGroup{
  public:
    typedef PE local_t;
    typedef CkIndex_PE index_t;
    typedef CProxy_PE proxy_t;
    typedef CProxyElement_PE element_t;
    typedef CProxySection_PE section_t;

    CProxySection_PE(void) {}
    CProxySection_PE(const IrrGroup *g) : CProxySection_IrrGroup(g){  }
    CProxySection_PE(const CkGroupID &_gid,const int *_pelist,int _npes,CK_DELCTOR_PARAM) : CProxySection_IrrGroup(_gid,_pelist,_npes,CK_DELCTOR_ARGS){  }
    CProxySection_PE(const CkGroupID &_gid,const int *_pelist,int _npes) : CProxySection_IrrGroup(_gid,_pelist,_npes){  }
    CProxySection_PE(int n,const CkGroupID *_gid, int const * const *_pelist,const int *_npes) : CProxySection_IrrGroup(n,_gid,_pelist,_npes){  }
    CProxySection_PE(int n,const CkGroupID *_gid, int const * const *_pelist,const int *_npes,CK_DELCTOR_PARAM) : CProxySection_IrrGroup(n,_gid,_pelist,_npes,CK_DELCTOR_ARGS){  }

    int ckIsDelegated(void) const
    { return CProxySection_IrrGroup::ckIsDelegated(); }
    inline CkDelegateMgr *ckDelegatedTo(void) const
    { return CProxySection_IrrGroup::ckDelegatedTo(); }
    inline CkDelegateData *ckDelegatedPtr(void) const
    { return CProxySection_IrrGroup::ckDelegatedPtr(); }
    CkGroupID ckDelegatedIdx(void) const
    { return CProxySection_IrrGroup::ckDelegatedIdx(); }
inline void ckCheck(void) const {CProxySection_IrrGroup::ckCheck();}
CkChareID ckGetChareID(void) const
   {return CProxySection_IrrGroup::ckGetChareID();}
CkGroupID ckGetGroupID(void) const
   {return CProxySection_IrrGroup::ckGetGroupID();}
operator CkGroupID () const { return ckGetGroupID(); }

    inline void setReductionClient(CkReductionClientFn fn,void *param=NULL) const
    { CProxySection_IrrGroup::setReductionClient(fn,param); }
    inline void ckSetReductionClient(CkReductionClientFn fn,void *param=NULL) const
    { CProxySection_IrrGroup::ckSetReductionClient(fn,param); }
    inline void ckSetReductionClient(CkCallback *cb) const
    { CProxySection_IrrGroup::ckSetReductionClient(cb); }
inline int ckGetNumSections() const
{ return CProxySection_IrrGroup::ckGetNumSections(); }
inline CkSectionInfo &ckGetSectionInfo()
{ return CProxySection_IrrGroup::ckGetSectionInfo(); }
inline CkSectionID *ckGetSectionIDs()
{ return CProxySection_IrrGroup::ckGetSectionIDs(); }
inline CkSectionID &ckGetSectionID()
{ return CProxySection_IrrGroup::ckGetSectionID(); }
inline CkSectionID &ckGetSectionID(int i)
{ return CProxySection_IrrGroup::ckGetSectionID(i); }
inline CkGroupID ckGetGroupIDn(int i) const
{ return CProxySection_IrrGroup::ckGetGroupIDn(i); }
inline int *ckGetElements() const
{ return CProxySection_IrrGroup::ckGetElements(); }
inline int *ckGetElements(int i) const
{ return CProxySection_IrrGroup::ckGetElements(i); }
inline int ckGetNumElements() const
{ return CProxySection_IrrGroup::ckGetNumElements(); } 
inline int ckGetNumElements(int i) const
{ return CProxySection_IrrGroup::ckGetNumElements(i); }

    void ckDelegate(CkDelegateMgr *dTo,CkDelegateData *dPtr=NULL)
    {       CProxySection_IrrGroup::ckDelegate(dTo,dPtr); }
    void ckUndelegate(void)
    {       CProxySection_IrrGroup::ckUndelegate(); }
    void pup(PUP::er &p)
    {       CProxySection_IrrGroup::pup(p); }
    void ckSetGroupID(CkGroupID g) {
      CProxySection_IrrGroup::ckSetGroupID(g);
    }
    PE* ckLocalBranch(void) const {
      return ckLocalBranch(ckGetGroupID());
    }
    static PE* ckLocalBranch(CkGroupID gID) {
      return (PE*)CkLocalBranch(gID);
    }
/* DECLS: PE(void);
 */
    

/* DECLS: PE(int impl_noname_0, int impl_noname_1);
 */
    

/* DECLS: void GVT_begin(void);
 */
    
    void GVT_begin(void);

/* DECLS: void GVT_contribute(void);
 */
    
    void GVT_contribute(void);

/* DECLS: void GVT_end(void);
 */
    
    void GVT_end(void);

/* DECLS: void execute_seq(void);
 */
    
    void execute_seq(void);

/* DECLS: void execute_cons(void);
 */
    
    void execute_cons(void);

/* DECLS: void execute_opt(void);
 */
    
    void execute_opt(void);

};
PUPmarshall(CProxySection_PE)
typedef CBaseT1<Group, CProxy_PE> CBase_PE;

/* ---------------- method closures -------------- */
class Closure_PE {
  public:



    struct GVT_begin_3_closure;


    struct GVT_contribute_4_closure;


    struct GVT_end_5_closure;


    struct execute_seq_6_closure;


    struct execute_cons_7_closure;


    struct execute_opt_8_closure;

};

extern void _registerpe(void);
#endif
