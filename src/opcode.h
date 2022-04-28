#ifndef SWPP_ASM_INTERPRETER_OPCODE_H
#define SWPP_ASM_INTERPRETER_OPCODE_H


enum Opcode {
  // terminators
  Ret = 0,
  BrUncond,
  BrCond,
  Switch,

  // memory operations
  Malloc,
  Free,
  Load,
  Store,

  // binary operations
  Bop,

  // sum
  Sum,

  // unary operations
  Uop,

  // ternary operation
  Select,

  // function call
  Call,

  // assertion
  Assert,

  // read and write
  Read,
  Write,

  LEN_OPCODE
};

enum BopKind {
  // arithmetic operations
  Udiv = 0,
  Sdiv,
  Urem,
  Srem,
  Mul,

  // logical operations
  Shl,
  Lshr,
  Ashr,
  And,
  Or,
  Xor,
  Add,
  Sub,

  // comparisons
  Eq,
  Ne,
  Ugt,
  Uge,
  Ult,
  Ule,
  Sgt,
  Sge,
  Slt,
  Sle
};

enum UopKind {
  Incr = 0,
  Decr
};

class Cost {
public:
  // cost of terminators
  constexpr static double RET = 1.0;
  constexpr static double BRUNCOND = 1.0;
  constexpr static double BRCOND_TRUE = 6.0;
  constexpr static double BRCOND_FALSE = 1.0;
  constexpr static double SWITCH=1.2;

  // cost of memory operations
  constexpr static double MALLOC = 16.0;
  constexpr static double FREE = 16.0;
  constexpr static double STACK = 6.0;
  constexpr static double HEAP = 12.0;
  constexpr static double ALOAD = 1.0;
  constexpr static double WAIT_STACK = 10.0;
  constexpr static double WAIT_HEAP = 16.0;

  // cost of binary operations
  constexpr static double MULDIV = 1.0;
  constexpr static double LOGICAL = 4.0;
  constexpr static double ADDSUB = 5.0;

  // cost of sum operation
  constexpr static double SUM = 5.2;

  // cost of unary operartions
  constexpr static double UOP = 1.0;

  // cost of comparison
  constexpr static double COMP = 1.0;

  // cost of ternary operation
  constexpr static double TERNARY = 1.2;

  // cost of function call
  constexpr static double CALL = 2.0;
  constexpr static double PER_ARG = 1.0;

  // cost of assertion
  constexpr static double ASSERT = 0.0;
};


#endif //SWPP_ASM_INTERPRETER_OPCODE_H
