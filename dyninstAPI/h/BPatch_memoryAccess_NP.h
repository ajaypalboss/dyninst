/* $Id: BPatch_memoryAccess_NP.h,v 1.13 2003/08/01 23:02:46 jodom Exp $ */

#ifndef _MemoryAccess_h_
#define _MemoryAccess_h_

#include <stdlib.h>
#include <BPatch_point.h>
#include <BPatch_instruction.h>

/* Pseudoregisters definitions */
#define POWER_XER2531	9999

#define IA32_EMULATE	1000
#define IA32_ESCAS	1000
#define IA32_NESCAS	1001
#define IA32_ECMPS	1002
#define IA32_NECMPS	1003

#define IA32prefetchNTA  0
#define IA32prefetchT0   1
#define IA32prefetchT1   2
#define IA32prefetchT2   3
#define IA32AMDprefetch  100
#define IA32AMDprefetchw 101


/* This is believed to be machine independent, modulo register numbers of course */
struct BPATCH_DLL_EXPORT BPatch_addrSpec_NP
{
  // the formula is regs[0] + 2 ^ scale * regs[1] + imm
  int imm;      // immediate
  unsigned int scale;
  int regs[2];  // registers: -1 means none, 0 is 1st, 1 is 2nd and so on
                // some pseudoregisters may be used, see the documentation

public:
  BPatch_addrSpec_NP(int _imm, int _ra = -1, int _rb = -1, int _scale = 0)
    :  imm(_imm), scale(_scale)
  {
    regs[0] = _ra;
    regs[1] = _rb;
  }

  BPatch_addrSpec_NP() : imm(0), scale(0)
  {
    regs[0] = 0;
    regs[1] = 0;
  }

  int getImm() const { return imm; }
  int getScale() const { return scale; }
  int getReg(unsigned i) const { return regs[i]; }

  bool equals(const BPatch_addrSpec_NP& ar) const
  {
    return
      (imm == ar.imm) &&
      (scale == ar.scale) &&
      (regs[0] == ar.regs[0]) &&
      (regs[1] == ar.regs[1]);
  }
};

#define BPatch_countSpec_NP BPatch_addrSpec_NP // right now it has the same form

class BPatch_memoryAccess;
extern void initOpCodeInfo();


class BPATCH_DLL_EXPORT BPatch_memoryAccess : public BPatch_instruction
{
  friend class BPatch_function;
  friend class AstNode;
  friend class InstrucIter;

 public:

  // Utility function to filter out the points that don't have a 2nd memory access on x86
  static BPatch_Vector<BPatch_point*>* filterPoints(const BPatch_Vector<BPatch_point*> &points,
                                                    unsigned int numMAs);
  
 private:
  // VG(8/13/02): removed redundant isPrefetch fields; preFcn>=0 is the same thing

  BPatch_addrSpec_NP start[nmaxacc_NP];
  BPatch_countSpec_NP count[nmaxacc_NP];

protected:
  BPatch_addrSpec_NP getStartAddr(int which = 0) const { return start[which]; }
  BPatch_countSpec_NP getByteCount(int which = 0) const { return count[which]; }


  // initializes only the first access - general case
  void set1st(bool _isLoad, bool _isStore,
              int _imm_s, int _ra_s, int _rb_s, unsigned int _scale_s,
              int _imm_c, int _ra_c, int _rb_c, unsigned int _scale_c,
              int _preFcn, int _cond, bool _nt)
  {
    nacc = 1;
    isLoad[0] = _isLoad;
    isStore[0] = _isStore;
    start[0] = BPatch_addrSpec_NP(_imm_s, _ra_s, _rb_s, _scale_s);
    count[0] = BPatch_countSpec_NP(_imm_c, _ra_c, _rb_c, _scale_c);
    preFcn[0] = _preFcn;
    condition[0] = _cond;
    nonTemporal[0] = _nt;
  }

  // initializes only the first access - no scale for count
  void set1st(bool _isLoad, bool _isStore,
              int _imm_s, int _ra_s, int _rb_s,
              int _imm_c, int _ra_c = -1, int _rb_c = -1,
              unsigned int _scale_s = 0, int _preFcn = -1,
              int _cond = -1, bool _nt = false)
  {
    nacc = 1;
    isLoad[0] = _isLoad;
    isStore[0] = _isStore;
    start[0] = BPatch_addrSpec_NP(_imm_s, _ra_s, _rb_s, _scale_s);
    count[0] = BPatch_countSpec_NP(_imm_c, _ra_c, _rb_c);
    preFcn[0] = _preFcn;
    condition[0] = _cond;
    nonTemporal[0] = _nt;
  }

 public:
  static BPatch_memoryAccess* const none;

  static BPatch_memoryAccess* init_tables()
  {
    initOpCodeInfo();
    return NULL;
  }

  // initializes only the first access; #bytes is a constant
  BPatch_memoryAccess(const void *buf, int _sz,
		      bool _isLoad, bool _isStore, unsigned int _bytes,
                      int _imm, int _ra, int _rb, unsigned int _scale = 0,
                      int _cond = -1, bool _nt = false) : BPatch_instruction(buf, _sz)
  {
    set1st(_isLoad, _isStore, _imm, _ra, _rb, _bytes, -1, -1, _scale, -1, _cond, _nt);
  }

  // initializes only the first access; #bytes is an expression w/scale
  BPatch_memoryAccess(const void *buf, int _sz,
		      bool _isLoad, bool _isStore,
                      int _imm_s, int _ra_s, int _rb_s, unsigned int _scale_s,
                      int _imm_c, int _ra_c, int _rb_c, unsigned int _scale_c,
                      int _cond, bool _nt, int _preFcn = -1) : BPatch_instruction(buf, _sz)
  {
    set1st(_isLoad, _isStore,
           _imm_s, _ra_s, _rb_s, _scale_s,
           _imm_c, _ra_c, _rb_c, _scale_c,
           _preFcn, _cond, _nt);
  }

  // initializes only the first access; #bytes is an expression
  BPatch_memoryAccess(const void *buf, int _sz,
		      bool _isLoad, bool _isStore, bool _isPrefetch,
                      int _imm_s, int _ra_s, int _rb_s,
                      int _imm_c, int _ra_c, int _rb_c,
                      unsigned short _preFcn) : BPatch_instruction(buf, _sz)
  {
    assert(_isPrefetch); // VG(8/13/02): historical reasons...
    set1st(_isLoad, _isStore, _imm_s, _ra_s, _rb_s, _imm_c, _ra_c, _rb_c, 0, _preFcn);
  }

  // initializes only the first access; #bytes is an expression & not a prefetch
  BPatch_memoryAccess(const void *buf, int _sz,
	       bool _isLoad, bool _isStore,
	       int _imm_s, int _ra_s, int _rb_s,
	       int _imm_c, int _ra_c, int _rb_c) : BPatch_instruction(buf, _sz)
  {
    set1st(_isLoad, _isStore, _imm_s, _ra_s, _rb_s, _imm_c, _ra_c, _rb_c);
  }

  // sets 2nd access; #bytes is constant
  void set2nd(bool _isLoad, bool _isStore, unsigned int _bytes,
              int _imm, int _ra, int _rb, unsigned int _scale = 0)
  {
    if(nacc >= 2)
      return;
    nacc = 2;
    isLoad[1] = _isLoad;
    isStore[1] = _isStore;
    start[1] = BPatch_addrSpec_NP(_imm, _ra, _rb, _scale);
    count[1] = BPatch_countSpec_NP(_bytes);
    preFcn[1] = -1;
    condition[1] = -1;
    nonTemporal[1] = false;
  }

  // sets 2nd access; #bytes is an expression w/scale
  void set2nd(bool _isLoad, bool _isStore,
              int _imm_s, int _ra_s, int _rb_s, unsigned int _scale_s,
              int _imm_c, int _ra_c, int _rb_c, unsigned int _scale_c,
              int _cond, bool _nt)
  {
    if(nacc >= 2)
      return;
    nacc = 2;
    isLoad[1] = _isLoad;
    isStore[1] = _isStore;
    start[1] = BPatch_addrSpec_NP(_imm_s, _ra_s, _rb_s, _scale_s);
    count[1] = BPatch_countSpec_NP(_imm_c, _ra_c, _rb_c, _scale_c);
    preFcn[1] = -1;
    condition[1] = _cond;
    nonTemporal[1] = _nt;
  }


  // initializes both accesses; #bytes is a constant
  BPatch_memoryAccess(const void *buf, int _sz,
		      bool _isLoad, bool _isStore, unsigned int _bytes,
                      int _imm, int _ra, int _rb, unsigned int _scale,
                      bool _isLoad2, bool _isStore2, unsigned int _bytes2,
                      int _imm2, int _ra2, int _rb2, unsigned int _scale2) : BPatch_instruction(buf, _sz)
  {
    set1st(_isLoad, _isStore, _imm, _ra, _rb, _bytes, -1, -1, _scale);
    set2nd(_isLoad2, _isStore2, _bytes2, _imm2, _ra2, _rb2, _scale2);
  }

  // initializes both accesses; #bytes is an expression & not a prefetch
  BPatch_memoryAccess(const void *buf, int _sz,
		      bool _isLoad, bool _isStore,
                      int _imm_s, int _ra_s, int _rb_s, unsigned int _scale_s,
                      int _imm_c, int _ra_c, int _rb_c, unsigned int _scale_c,
                      bool _isLoad2, bool _isStore2,
                      int _imm2_s, int _ra2_s, int _rb2_s, unsigned int _scale2_s,
                      int _imm2_c, int _ra2_c, int _rb2_c, unsigned int _scale2_c) : BPatch_instruction(buf, _sz)
  {
    set1st(_isLoad, _isStore, 
           _imm_s, _ra_s, _rb_s, _scale_s,
           _imm_c, _ra_c, _rb_c, _scale_c,
           -1, -1, false);
    set2nd(_isLoad2, _isStore2,
           _imm2_s, _ra2_s, _rb2_s, _scale2_s,
           _imm2_c, _ra2_c, _rb2_c, _scale2_c,
           -1, false);
  }


  bool equals(const BPatch_memoryAccess* mp) const { return mp ? equals(*mp) : false; }

  bool equals(const BPatch_memoryAccess& rp) const
  {
    bool res = nacc == rp.nacc;

    if(!res)
      return res;

    for(unsigned int i=0; i<nacc; ++i) {
      res = res &&
        (isLoad[i] == rp.isLoad[i]) &&
        (isStore[i] == rp.isStore[i]) &&
        (start[i].equals(rp.start[i])) &&
        (count[i].equals(rp.count[i])) &&
        (preFcn[i] == rp.preFcn[i]) &&
        (condition[i] == rp.condition[i]) && 
        (nonTemporal[i] == rp.nonTemporal[i]);
      if(!res)
        break;
    }

    return res;
  }

  BPatch_addrSpec_NP getStartAddr_NP(int which = 0) const { return start[which]; }
  BPatch_countSpec_NP getByteCount_NP(int which = 0) const { return count[which]; }

};

#endif
