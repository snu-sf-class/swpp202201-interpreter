#include <sstream>
#include <iomanip>

#include "state.h"
#include "error.h"


CostStack::CostStack(const string &_fname): fname(_fname), cost(0), callees() {}

double CostStack::get_cost() const { return cost; }

void CostStack::add_cost(double _cost) { cost += _cost; }

void CostStack::set_callee(CostStack *callee) {
  callees.push_back(callee);
}

string CostStack::to_string(const string& indent) const {
  stringstream ss;
  ss << fixed << setprecision(4);
  ss << indent << fname << ": " << cost << endl;
  for (auto it: callees)
    ss << it->to_string(indent + "| ");
  return ss.str();
}


State::State(): regfile(), memory(), main_cost(nullptr), total_wait_cost(0), program(nullptr) {
  for (double& c: cost_per_inst)
    c = 0.0;
  for (int& c: inst_count)
    c = 0;
}

void State::set_program(Program* _program) {
  if (program == nullptr)
    program = _program;
}

double State::get_cost_value() const { return main_cost->get_cost(); }

CostStack * State::get_cost() const { return main_cost; }

uint64_t State::get_max_alloced_size() const {
  return memory.get_max_alloced_size();
}

void State::update_cost_log(Opcode opcode, double inst_cost, double wait_cost) {
  cost_per_inst[opcode] += inst_cost;
  inst_count[opcode]++;
  total_wait_cost += wait_cost;
}

uint64_t State::exec_function(CostStack* parent, Function* function) {
  auto cost = new CostStack(function->get_fname());
  if (parent == nullptr)
    main_cost = cost;
  else
    parent->set_callee(cost);

  Stmt* curr = function->get_first_bb();
  if (curr == nullptr)
    invoke_runtime_error("missing first basic block");

  while (true) {
    error_line_num = curr->get_line();

    switch (curr->get_opcode()) {
      case Ret: {
        auto stmt = dynamic_cast<StmtRet*>(curr);
        auto ret = stmt->get_val(cost->get_cost(), regfile);
        cost->add_cost(Cost::RET + ret.second);
        update_cost_log(Ret, Cost::RET, ret.second);
        if (parent != nullptr)
          parent->add_cost(cost->get_cost());
        return ret.first;
      }
      case BrUncond: {
        auto stmt = dynamic_cast<StmtBrUncond*>(curr);
        string bb = stmt->get_bb();
        curr = function->get_bb(bb);
        if (curr == nullptr) {
          invoke_runtime_error("branching to an undefined basic block");
          return 0;
        }
        cost->add_cost(Cost::BRUNCOND);
        update_cost_log(BrUncond, Cost::BRUNCOND, 0);
        break;
      }
      case BrCond: {
        auto stmt = dynamic_cast<StmtBrCond*>(curr);
        auto bb = stmt->get_bb(cost->get_cost(), regfile);
        curr = function->get_bb(bb.first);
        if (curr == nullptr) {
          invoke_runtime_error("branching to an undefined basic block");
          return 0;
        }
        double inst_cost = stmt->get_eval() ? Cost::BRCOND_TRUE : Cost::BRCOND_FALSE;
        cost->add_cost(inst_cost + bb.second);
        update_cost_log(BrCond, inst_cost, bb.second);
        break;
      }
      case Switch: {
        auto stmt = dynamic_cast<StmtSwitch*>(curr);
        auto bb = stmt->get_bb(cost->get_cost(), regfile);
        curr = function->get_bb(bb.first);
        if (curr == nullptr) {
          invoke_runtime_error("branching to an undefined basic block");
          return 0;
        }
        cost->add_cost(Cost::SWITCH + bb.second);
        update_cost_log(Switch, Cost::SWITCH, bb.second);
        break;
      }
      case Call: {
        auto stmt = dynamic_cast<StmtCall*>(curr);
        string fname = stmt->get_fname();
        Function* callee = program->get_function(fname);
        if (callee == nullptr) {
          invoke_runtime_error("calling an undefined function");
          return 0;
        }

        int nargs = callee->get_nargs();
        if (nargs != stmt->get_nargs()) {
          invoke_runtime_error("calling with incorrect number of arguments");
          return 0;
        }

        RegFile old = regfile;

        regfile.set_nargs(nargs);
        double wait_cost = stmt->setup_args(cost->get_cost(), old, regfile);
        double inst_cost = Cost::CALL + nargs * Cost::PER_ARG;
        cost->add_cost(inst_cost + wait_cost);
        update_cost_log(Call, inst_cost, wait_cost);
        uint64_t ret = exec_function(cost, callee);
        regfile = old;
        regfile.write_reg(curr->get_lhs(), ret);

        curr = stmt->get_next();
        break;
      }
      default: {
        auto costs = curr->exec(cost->get_cost(), regfile, memory);
        cost->add_cost(costs.first + costs.second);
        update_cost_log(curr->get_opcode(), costs.first, costs.second);
        curr = curr->get_next();
      }
    }
  }
}

uint64_t State::exec_program() {
  Function* main = program->get_function("main");
  if (main == nullptr)
    invoke_runtime_error("missing main function");
  uint64_t res = exec_function(nullptr, main);
  return res;
}

string State::inst_log_line(Opcode opcode, const string &inst) const {
  stringstream ss;
  ss << fixed << setprecision(4);
  ss << inst << "\t" << inst_count[opcode] << "\t" << cost_per_inst[opcode];
  return ss.str();
}

string State::inst_log_to_string() const {
  stringstream ss;
  ss << fixed << setprecision(4);
  ss << "Instruction" << "\t" << "Count" << "\t" << "Cost" << endl;
  ss << inst_log_line(Ret, "Ret") << endl;
  ss << inst_log_line(BrUncond, "BrUncond") << endl;
  ss << inst_log_line(BrCond, "BrCond") << endl;
  ss << inst_log_line(BrCond, "BrCond") << endl;
  ss << inst_log_line(Switch, "Switch") << endl;
  ss << inst_log_line(Malloc, "Malloc") << endl;
  ss << inst_log_line(Free, "Free") << endl;
  ss << inst_log_line(Load, "Load") << endl;
  ss << inst_log_line(Store, "Store") << endl;
  ss << inst_log_line(Bop, "BinaryOp") << endl;
  ss << inst_log_line(Sum, "Sum") << endl;
  ss << inst_log_line(Uop, "UnaryOp") << endl;
  ss << inst_log_line(Select, "Select") << endl;
  ss << inst_log_line(Call, "Call") << endl;
  ss << inst_log_line(Read, "Read") << endl;
  ss << inst_log_line(Write, "Write") << endl;
  return ss.str();
}

double State::get_total_wait_cost() const {
  return total_wait_cost;
}
