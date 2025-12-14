/*
 *  TeX code generator for TCC
 *
 *  Copyright (c) 2025 Vasyl Bodnar
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef TARGET_DEFS_ONLY

/* number of available registers */
#define NB_REGS 4

/* a register can belong to several classes. The classes must be
   sorted from more general to more precise (see gv2() code which does
   assumptions on it). */
#define RC_INT 0x0001   /* generic integer register */
#define RC_FLOAT 0x0002 /* generic float register */
#define RC_EAX 0x0004
#define RC_ST0 0x0008
#define RC_ECX 0x0010
#define RC_EDX 0x0020

#define RC_IRET RC_EAX /* function return: integer register */
#define RC_IRE2 RC_ECX /* function return: second integer register */
#define RC_FRET RC_EAX /* function return: float register */

/* pretty names for the registers */
enum {
  TREG_EAX = 0,
  TREG_ECX,
  TREG_EDX,
  TREG_ST0,
};

/* return registers for function */
#define REG_IRET TREG_EAX /* single word int return register */
#define REG_IRE2 TREG_ECX /* second word return register (for long long) */
#define REG_FRET TREG_EAX /* float return register */

/* pointer size, in bytes */
#define PTR_SIZE 4

/* long double size and alignment, in bytes */
#define LDOUBLE_SIZE 8
#define LDOUBLE_ALIGN 4
/* maximum alignment (for aligned attribute support) */
#define MAX_ALIGN 4

#undef CONFIG_TCC_BCHECK

/******************************************************/
#else /* ! TARGET_DEFS_ONLY */
/******************************************************/
#define USING_GLOBALS
#include "tcc.h"

ST_DATA const char *const target_machine_defs = "__TeX__\0";

ST_DATA const int reg_classes[NB_REGS] = {
    /* eax */ RC_INT | RC_EAX,
    /* ecx */ RC_INT | RC_ECX,
    /* edx */ RC_INT | RC_EDX,
    /* st0 */ RC_INT | RC_ST0,
};

/* TODO: This is a hack so we only need to \newcount once */
ST_DATA int allocated[NB_REGS] = {0, 0, 0, 0};

#define MAX_STR_SIZE 300
ST_DATA char output_str[MAX_STR_SIZE];

ST_FUNC void out(const char *str, int size) {
  if (nocode_wanted)
    return;
  int ind1 = ind + size;
  if (ind1 > (int)cur_text_section->data_allocated)
    section_realloc(cur_text_section, ind1);
  for (int i = 0; i < size; i++) {
    cur_text_section->data[ind + i] = str[i];
  }
  ind = ind1;
}

ST_FUNC void out_r_new(int r1) {
  int size =
      snprintf(output_str, MAX_STR_SIZE,
               "\\expandafter\\newcount\\csname @reg%d\\endcsname%%\n", r1);
  out(output_str, size);
}

ST_FUNC void out_rr_move(int r1, int r2) {
  int size =
      snprintf(output_str, MAX_STR_SIZE,
               "\\expandafter\\csname @reg%d\\endcsname=\\expandafter\\csname "
               "@reg%d\\endcsname%%\n",
               r1, r2);
  out(output_str, size);
}

ST_FUNC void out_rc_move(int r1, int cnst) {
  int size =
      snprintf(output_str, MAX_STR_SIZE,
               "\\expandafter\\csname @reg%d\\endcsname=%d%%\n", r1, cnst);
  out(output_str, size);
}

ST_FUNC void out_rm_move(int r1, int addr) {
  int size =
      snprintf(output_str, MAX_STR_SIZE,
               "\\expandafter\\csname @reg%d\\endcsname=\\expandafter\\csname "
               "@mem%d\\endcsname%%\n",
               r1, addr);
  out(output_str, size);
}

ST_FUNC void out_rs_move(int r1, int st_off) {
  int size;
  if (st_off) {
    size = snprintf(
        output_str, MAX_STR_SIZE,
        "\\expandafter\\csname @regStTmp\\endcsname=\\expandafter\\csname "
        "@regSt\\endcsname%%\n\\advance\\expandafter\\csname "
        "@regStTmp\\endcsname"
        "by%d%%\n\\expandafter\\csname @reg%d\\endcsname=\\expandafter\\csname "
        "@mem\\the\\@regStTmp\\endcsname%%\n",
        r1, st_off);
  } else {
    size = snprintf(
        output_str, MAX_STR_SIZE,
        "\\expandafter\\csname @reg%d\\endcsname=\\expandafter\\csname "
        "@mem\\the\\@regSt\\endcsname%%\n",
        r1);
  }
  out(output_str, size);
}

ST_FUNC void out_rrm_move(int r1, int r2) {
  int size =
      snprintf(output_str, MAX_STR_SIZE,
               "\\expandafter\\csname @reg%d\\endcsname=\\expandafter\\csname "
               "@mem\\the\\csname @reg%d\\endcsname\\endcsname%%\n",
               r1, r2);
  out(output_str, size);
}

ST_FUNC void out_mr_move(int addr, int r2) {
  int size =
      snprintf(output_str, MAX_STR_SIZE,
               "\\expandafter\\xdef\\csname "
               "@mem%d\\endcsname{\\expandafter\\csname @reg%d\\endcsname}%%\n",
               addr, r2);
  out(output_str, size);
}

ST_FUNC void out_sr_move(int st_off, int r2) {
  int size;
  if (st_off) {
    size = snprintf(
        output_str, MAX_STR_SIZE,
        "\\expandafter\\csname @regStTmp\\endcsname=\\expandafter\\csname "
        "@regSt\\endcsname%%\n\\advance\\expandafter\\csname "
        "@regStTmp\\endcsname"
        "by%d%%\n\\expandafter\\xdef\\csname "
        "@mem\\expandafter\\the\\@regStTmp\\endcsname{\\expandafter\\csname "
        "@reg%d\\endcsname}%%\n",
        st_off, r2);
  } else {
    size = snprintf(
        output_str, MAX_STR_SIZE,
        "\\expandafter\\xdef\\csname "
        "@mem\\expandafter\\the\\@regSt\\endcsname{\\expandafter\\csname "
        "@reg%d\\endcsname}%%\n",
        r2);
  }
  out(output_str, size);
}

ST_FUNC void out_rmr_move(int r1, int r2) {
  int size = snprintf(output_str, MAX_STR_SIZE,
                      "\\expandafter\\xdef\\csname "
                      "@mem\\expandafter\\the\\csname "
                      "@reg%d\\endcsname\\endcsname{\\expandafter\\csname "
                      "@reg%d\\endcsname}%%\n",
                      r1, r2);
  out(output_str, size);
}

/* output a symbol and patch all calls to it */
ST_FUNC void gsym_addr(int t, int a) {
  printf("gsym_addr\n");
  (void)t;
  (void)a;
}

/* load 'r' from value 'v' */
ST_FUNC void load(int r, SValue *v) {
  printf("load %d\n", r);
  if (!allocated[r]) {
    out_r_new(r);
    allocated[r] = 1;
  }
  int typ = v->r & VT_VALMASK;
  if (v->r & VT_LVAL) {
    if (typ == VT_LLOCAL) {
      SValue v1;
      v1.type.t = VT_INT;
      v1.r = VT_LOCAL | VT_LVAL;
      v1.c.i = v->c.i;
      v1.sym = NULL;
      typ = r;
      if (!(reg_classes[v->r] & RC_INT))
        typ = get_reg(RC_INT);
      load(typ, &v1);
      printf("VT_LLOCAL ");
    }
    switch (v->r) {
    default:
      out_rrm_move(r, typ);
      break;
    case VT_CONST:
      out_rm_move(r, v->c.i);
      break;
    case VT_LOCAL:
      out_rs_move(r, v->c.i);
      break;
    }
  } else {
    switch (typ) {
    default:
      if (typ != r) {
        out_rr_move(r, typ);
      }
      break;
    case VT_CONST:
      out_rc_move(r, v->c.i);
      break;
    case VT_LOCAL:
      out_rs_move(r, v->c.i);
      break;
    case VT_CMP:
      printf("VT_CMP\n");
      break;
    case VT_JMP:
      printf("VT_JMP\n");
      break;
    case VT_JMPI:
      printf("VT_JMPI\n");
      break;
    }
  }
}

/* store register 'r' in lvalue 'v' */
ST_FUNC void store(int r, SValue *v) {
  printf("store %d\n", r);
  int typ = v->r & VT_VALMASK;
  if (v->r & VT_LVAL) {
    out_rmr_move(typ, r);
  } else {
    switch (typ) {
    default:
      if (typ != r) {
        if (!allocated[typ]) {
          out_r_new(typ);
          allocated[typ] = 1;
        }
        out_rr_move(typ, r);
      }
      break;
    case VT_CONST:
      out_mr_move(v->c.i, r);
      break;
    case VT_LOCAL:
      out_sr_move(v->c.i, r);
      break;
    }
  }
}

/* 'is_jmp' is '1' if it is a jump */
ST_FUNC void gcall_or_jmp(int is_jmp) {
  printf("gcall_or_jmp\n");
  (void)is_jmp;
}

/* Return the number of registers needed to return the struct, or 0 if
   returning via struct pointer. */
ST_FUNC int gfunc_sret(CType *vt, int variadic, CType *ret, int *ret_align,
                       int *regsize) {
  *ret_align = 1; // Never have to re-align return values
  return 0;
}

/* generate function call with address in (vtop->t, vtop->c) and free function
   context. Stack entry is popped */
ST_FUNC void gfunc_call(int nb_args) {
  printf("gfunc_call\n");
  (void)nb_args;
}

/* generate function prolog of type 't' */
ST_FUNC void gfunc_prolog(Sym *func_sym) {
  printf("gfunc_prolog\n");
  (void)func_sym;
}

/* generate function epilog */
ST_FUNC void gfunc_epilog(void) { printf("gfunc_epilog\n"); }

ST_FUNC void gen_fill_nops(int bytes) {
  printf("void\n");
  (void)bytes;
}

/* generate a jump to a label */
ST_FUNC int gjmp(int t) {
  printf("gjmp\n");
  (void)t;
  return 0;
}

/*generate a jump to a fixed address */
ST_FUNC void gjmp_addr(int a) {
  printf("gjmp_addr\n");
  (void)a;
}

/* generate a test. set 'inv' to invert test. Stack entry is popped */
ST_FUNC int gjmp_cond(int op, int t) {
  printf("gjmp_cond\n");
  (void)op;
  (void)t;
  return 0;
}

ST_FUNC int gjmp_append(int n0, int t) {
  printf("gjmp_append\n");
  (void)n0;
  (void)t;
  return 0;
}

/* generate an integer binary operation */
ST_FUNC void gen_opi(int op) {
  printf("gen_opi\n");
  (void)op;
}

/* generate a floating point operation 'v = t1 op t2' instruction. The
   two operands are guaranteed to have the same floating point type */
ST_FUNC void gen_opf(int op) {
  printf("gen_opf\n");
  (void)op;
}

/* convert integers to fp 't' type. Must handle 'int', 'unsigned int'
   and 'long long' cases. */
ST_FUNC void gen_cvt_itof(int t) {
  printf("gen_cvt_itof\n");
  (void)t;
}

/* convert fp to int 't' type */
ST_FUNC void gen_cvt_ftoi(int t) {
  printf("gen_cvt_ftoi\n");
  (void)t;
}

/* convert from one floating point type to another */
ST_FUNC void gen_cvt_ftof(int t) {
  printf("gen_cvt_ftof\n");
  (void)t;
}

/* computed goto support */
ST_FUNC void ggoto(void) { printf("ggoto\n"); }

/* Save the stack pointer onto the stack and return the location of its address
 */
ST_FUNC void gen_vla_sp_save(int addr) {
  tcc_error("variable length arrays unsupported for this target");
}

/* Restore the SP from a location on the stack */
ST_FUNC void gen_vla_sp_restore(int addr) {
  tcc_error("variable length arrays unsupported for this target");
}

/* Subtract from the stack pointer, and push the resulting value onto the stack
 */
ST_FUNC void gen_vla_alloc(CType *type, int align) {
  tcc_error("variable length arrays unsupported for this target");
}

/*************************************************************/
#endif
/*************************************************************/
