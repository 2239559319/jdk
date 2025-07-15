/*
 * Copyright (c) 2025, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "opto/graphInvariants.hpp"

#include "rootnode.hpp"

#ifndef PRODUCT

#define OUTPUT_STEP (-1)

void print_path(const Node* center, const Node_List& steps, const GrowableArray<int>& path, stringStream& ss) {
  assert(steps.size() == static_cast<uint>(path.length()) + 1, "sanity");
  assert(steps.at(steps.size() - 1) == center, "sanity");
  if (path.is_empty()) {
    ss.print_cr("At center node");
    center->dump("\n", false, &ss);
    return;
  }
  ss.print("At node\n    ");
  center->dump("\n", false, &ss);
  ss.print_cr("  From path:");
  ss.print("    [center]");
  steps.at(0)->dump("\n", false, &ss);
  for (int i = 0; i < path.length(); ++i) {
    if (path.at(i) >= 0) {
      // It's an input
      ss.print("      <-(%d)-", path.at(i));
    } else {
      // It's an output
      ss.print("         -->");
    }
    steps.at(i + 1)->dump("\n", false, &ss);
  }
}

struct Pattern : ResourceObj {
  virtual bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream&) const = 0;
};

struct True : Pattern {
  bool check(const Node*, Node_List&, GrowableArray<int>&, stringStream&) const override {
    return true;
  }
};

struct Bind : Pattern {
  explicit Bind(const Node*& binding) : _binding(binding) {}
  bool check(const Node* center, Node_List&, GrowableArray<int>&, stringStream&) const override {
    _binding = center;
    return true;
  }
  const Node*& _binding;
};

struct And : Pattern {
private:
  template <typename... PP>
  static void make_helper(And* a, Pattern* pattern, PP... others) {
    a->_checks.push(pattern);
    make_helper(a, others...);
  }
  static void make_helper(And*) {}

public:
  template <typename... PP>
  static And* make(PP... patterns) {
    And* andd = new And();
    make_helper(andd, patterns...);
    return andd;
  }

  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    for (int i = 0; i < _checks.length(); ++i) {
      if (!_checks.at(i)->check(center, steps, path, ss)) {
        return false;
      }
    }
    return true;
  }
  GrowableArray<Pattern*> _checks;
};

struct HasExactlyNInputs : Pattern {
  explicit HasExactlyNInputs(uint expect_req) : _expect_req(expect_req) {}
  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (center->req() != _expect_req) {
      print_path(center, steps, path, ss);
      ss.print_cr("Unexpected number of input. Expected: %d. Found: %d", _expect_req, center->req());
      return false;
    }
    return true;
  }
  const uint _expect_req;
};

struct HasAtLeastNInputs : Pattern {
  explicit HasAtLeastNInputs(uint expect_req) : _expect_req(expect_req) {}
  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (center->req() < _expect_req) {
      print_path(center, steps, path, ss);
      ss.print_cr("Too small number of input. Expected: %d. Found: %d", _expect_req, center->req());
      return false;
    }
    return true;
  }
  const uint _expect_req;
};

struct AtInput : Pattern {
  AtInput(uint which_input, const Pattern* pattern) : _which_input(which_input), _pattern(pattern) {}
  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    assert(_which_input < center->req(), "First check the input number");
    if (center->in(_which_input) == nullptr) {
      print_path(center, steps, path, ss);
      ss.print_cr("Input at index %d is nullptr.", _which_input);
      return false;
    }
    steps.push(center->in(_which_input));
    path.push(static_cast<int>(_which_input));
    bool result = _pattern->check(center->in(_which_input), steps, path, ss);
    path.pop();
    steps.pop();
    return result;
  }
  const uint _which_input;
  const Pattern* const _pattern;
};

struct HasType : Pattern {
  explicit HasType(bool (Node::*type_check)() const) : _type_check(type_check) {}
  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!(center->*_type_check)()) {
      print_path(center, steps, path, ss);
      ss.print_cr("Unexpected type: %s.", center->Name());
      return false;
    }
    return true;
  }
  bool (Node::*_type_check)() const;
};

struct HasNOutputs : Pattern {
  explicit HasNOutputs(uint expect_outcnt) : _expect_outcnt(expect_outcnt) {}
  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (center->outcnt() != _expect_outcnt) {
      print_path(center, steps, path, ss);
      ss.print_cr("Unexpected number of outputs. Expected: %d. Found: %d", _expect_outcnt, center->outcnt());
      return false;
    }
    return true;
  }
  const uint _expect_outcnt;
};

struct AtSingleOutputOfType : Pattern {
  AtSingleOutputOfType(bool (Node::*type_check)() const, const Pattern* pattern) : _type_check(type_check), _pattern(pattern) {
  }

  bool check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    Node_List outputs_of_correct_type;
    for (DUIterator_Fast imax, i = center->fast_outs(imax); i < imax; i++) {
      Node* out = center->fast_out(i);
      if ((out->*_type_check)()) {
        outputs_of_correct_type.push(out);
      }
    }
    if (outputs_of_correct_type.size() != 1) {
      print_path(center, steps, path, ss);
      ss.print_cr("Non-unique output of expected type. Found: %d.", outputs_of_correct_type.size());
      for (uint i = 0; i < outputs_of_correct_type.size(); ++i) {
        outputs_of_correct_type.at(i)->dump("\n", false, &ss);
      }
      return false;
    }
    steps.push(outputs_of_correct_type.at(0));
    path.push(OUTPUT_STEP);
    bool result = _pattern->check(outputs_of_correct_type.at(0), steps, path, ss);
    path.pop();
    steps.pop();
    return result;
  }
  bool (Node::*_type_check)() const;
  const Pattern* const _pattern;
};

struct PatternBasedCheck : LocalGraphInvariant {
  const Pattern* const _pattern;
  explicit PatternBasedCheck(const Pattern* pattern) : _pattern(pattern) {}
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    return _pattern->check(center, steps, path, ss) ? CheckResult::VALID : CheckResult::FAILED;
  }
};

struct IfProjections : PatternBasedCheck {
  IfProjections()
      : PatternBasedCheck(
            And::make(
                new HasNOutputs(2),
                new AtSingleOutputOfType(&Node::is_IfTrue, new True()),
                new AtSingleOutputOfType(&Node::is_IfFalse, new True()))) {
  }
  const char* name() const override {
    return "IfProjections";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_If()) {
      return CheckResult::NOT_APPLICABLE;
    }
    return PatternBasedCheck::check(center, steps, path, ss);
  }
};

struct PhiArity : PatternBasedCheck {
  const Node* region_node = nullptr;
  PhiArity()
      : PatternBasedCheck(
            And::make(
                new HasAtLeastNInputs(1),
                new AtInput(
                    0,
                    And::make(
                        new HasType(&Node::is_Region),
                        new Bind(region_node))))) {
  }
  const char* name() const override {
    return "PhiArity";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_Phi()) {
      return CheckResult::NOT_APPLICABLE;
    }
    CheckResult result = PatternBasedCheck::check(center, steps, path, ss);
    if (result != CheckResult::VALID) {
      return result;
    }
    assert(region_node != nullptr, "sanity");
    if (region_node->req() != center->req()) {
      print_path(center, steps, path, ss);
      ss.print_cr("Phi nodes must have the same arity as their Region node. Phi arity: %d; Region arity: %d.", center->req(), region_node->req());
      return CheckResult::FAILED;
    }
    return CheckResult::VALID;
  }
};

struct ControlSuccessor : LocalGraphInvariant {
  const char* name() const override {
    return "ControlSuccessor";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_CFG()) {
      return CheckResult::NOT_APPLICABLE;
    }

    Node_List ctrl_succ;
    uint cfg_out = 0;
    for (DUIterator_Fast imax, i = center->fast_outs(imax); i < imax; i++) {
      Node* out = center->fast_out(i);
      if (out->is_CFG()) {
        cfg_out++;
        ctrl_succ.push(out);
      }
    }

    if (center->is_If() || center->is_Start() || center->is_Root() || center->is_Region() || center->is_NeverBranch()) {
      if (cfg_out != 2) {
        ss.print_cr("%s node must have exactly two control successors. Found %d.", center->Name(), cfg_out);
        for (uint i = 0; i < ctrl_succ.size(); ++i) {
          ctrl_succ.at(i)->dump("\n", false, &ss);
        }
        return CheckResult::FAILED;
      }
    } else if (center->Opcode() == Op_SafePoint) {
      if (cfg_out < 1 || cfg_out > 2) {
        ss.print_cr("%s node must have one or two control successors. Found %d.", center->Name(), cfg_out);
        for (uint i = 0; i < ctrl_succ.size(); ++i) {
          ctrl_succ.at(i)->dump("\n", false, &ss);
        }
        return CheckResult::FAILED;
      }
      if (cfg_out == 2) {
        if (!ctrl_succ.at(0)->is_Root() && !ctrl_succ.at(1)->is_Root()) {
          ss.print_cr("One of the two control outputs of a %s node must be Root.", center->Name());
          for (uint i = 0; i < ctrl_succ.size(); ++i) {
            ctrl_succ.at(i)->dump("\n", false, &ss);
          }
          return CheckResult::FAILED;
        }
      }
    } else if (center->is_Catch() || center->is_Jump()) {
      if (cfg_out < 1) {
        ss.print_cr("%s node must have at least one control successors. Found %d.", center->Name(), cfg_out);
        return CheckResult::FAILED;
      }
    } else {
      if (cfg_out != 1) {
        ss.print_cr("Ordinary CFG nodes must have exactly one successor. Found %d.", cfg_out);
        for (uint i = 0; i < ctrl_succ.size(); ++i) {
          ctrl_succ.at(i)->dump("\n", false, &ss);
        }
        return CheckResult::FAILED;
      }
    }

    return CheckResult::VALID;
  }
};

struct RegionSelfLoop : LocalGraphInvariant {
  const char* name() const override {
    return "RegionSelfLoop";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_Region() && !center->is_Start() && !center->is_Root()) {
      return CheckResult::NOT_APPLICABLE;
    }

    if (center->req() == 0) {
      ss.print_cr("%s nodes must have at least one input.", center->Name());
      return CheckResult::FAILED;
    }

    Node* self = center->in(LoopNode::Self);

    if (center != self || (center->is_Region() && self == nullptr)) {
      ss.print_cr("%s nodes' 0-th input must be itself or nullptr (for a copy Region).", center->Name());
      return CheckResult::FAILED;
    }

    if (self == nullptr) {
      // Must be a copy Region
      Node_List non_null_inputs;
      for (uint i = 0; i < center->req(); i++) {
        if (center->in(i) != nullptr) {
          non_null_inputs.push(center->in(i));
        }
      }
      if (non_null_inputs.size() != 1) {
        ss.print_cr("%s copy nodes must have exactly one non-null input. Found: %d.", center->Name(), non_null_inputs.size());
        for (uint i = 0; i < non_null_inputs.size(); ++i) {
          non_null_inputs.at(i)->dump("\n", false, &ss);
        }
      }
    }

    return CheckResult::VALID;
  }
};

// CountedLoopEnd -> IfTrue -> CountedLoop
struct CountedLoopInvariants : PatternBasedCheck {
  const Node* counted_loop = nullptr;
  CountedLoopInvariants()
      : PatternBasedCheck(
            And::make(
                new HasExactlyNInputs(3),
                new AtInput(
                    LoopNode::LoopBackControl,
                    And::make(
                        new HasType(&Node::is_IfTrue),
                        new HasAtLeastNInputs(1),
                        new AtInput(
                            0,
                            And::make(
                                new HasType(&Node::is_BaseCountedLoopEnd),
                                new Bind(counted_loop))))))) {}
  const char* name() const override {
    return "CountedLoopInvariants";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_CountedLoop() && !center->is_LongCountedLoop()) {
      return CheckResult::NOT_APPLICABLE;
    }

    bool is_long = center->is_LongCountedLoop();

    CheckResult result = PatternBasedCheck::check(center, steps, path, ss);
    if (result != CheckResult::VALID) {
      return result;
    }
    assert(counted_loop != nullptr, "sanity");
    if (is_long) {
      if (counted_loop->is_CountedLoopEnd()) {
        ss.print_cr("A CountedLoopEnd is the backedge of a LongCountedLoop.");
        return CheckResult::FAILED;
      }
    } else {
      if (counted_loop->is_LongCountedLoopEnd()) {
        ss.print_cr("A LongCountedLoopEnd is the backedge of a CountedLoop.");
        return CheckResult::FAILED;
      }
    }
    return CheckResult::VALID;
  }
};

// CountedLoopEnd -> IfFalse -> SafePoint -> OuterStripMinedLoopEnd[center] -> IfTrue -> OuterStripMinedLoop -> CountedLoop
struct OuterStripMinedLoopInvariants : PatternBasedCheck {
  OuterStripMinedLoopInvariants()
      : PatternBasedCheck(
            And::make(
                new HasExactlyNInputs(2),
                new AtInput(
                    0,
                    And::make(
                        new HasType(&Node::is_SafePoint),
                        new HasAtLeastNInputs(1),
                        new AtInput(
                            0,
                            And::make(
                                new HasType(&Node::is_IfFalse),
                                new HasAtLeastNInputs(1),
                                new AtInput(
                                    0,
                                    new HasType(&Node::is_CountedLoopEnd)))))),
                new AtSingleOutputOfType(
                    &Node::is_IfTrue,
                    new AtSingleOutputOfType(
                        &Node::is_OuterStripMinedLoop,
                        new AtSingleOutputOfType(
                            &Node::is_CountedLoop,
                            new True()))))) {}
  const char* name() const override {
    return "OuterStripMinedLoopInvariants";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_OuterStripMinedLoopEnd()) {
      return CheckResult::NOT_APPLICABLE;
    }

    return PatternBasedCheck::check(center, steps, path, ss);
  }
};

struct MultiBranchNodeOut : LocalGraphInvariant {
  const char* name() const override {
    return "MultiBranchNodeOut";
  }
  CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream& ss) const override {
    if (!center->is_MultiBranch()) {
      return CheckResult::NOT_APPLICABLE;
    }

    MultiBranchNode* mb = center->as_MultiBranch();
    if (mb->required_outcnt() < static_cast<int>(mb->outcnt())) {
      ss.print_cr("The required_outcnt of a MultiBranch node must be smaller than or equal to its outcnt. But required_outcnt=%d vs. outcnt=%d", mb->required_outcnt(), mb->outcnt());
      return CheckResult::FAILED;
    }

    return CheckResult::VALID;
  }
};

#endif

GraphInvariantChecker* GraphInvariantChecker::make_default() {
  auto* checker = new GraphInvariantChecker();
#ifndef PRODUCT
#define ADD_CHECKER(T) checker->_checks.push(new T())
  ADD_CHECKER(IfProjections);
  ADD_CHECKER(PhiArity);
  ADD_CHECKER(ControlSuccessor);
  ADD_CHECKER(RegionSelfLoop);
  ADD_CHECKER(CountedLoopInvariants);
  ADD_CHECKER(OuterStripMinedLoopInvariants);
  ADD_CHECKER(MultiBranchNodeOut);
#undef ADD_CHECKER
#endif
  return checker;
}

bool GraphInvariantChecker::run(const Compile* C) const {
#ifdef PRODUCT
  return true;
#else

  if (_checks.is_empty()) {
    return true;
  }

  VectorSet visited;
  Node_List worklist;
  worklist.push(C->root());
  Node_List steps;
  GrowableArray<int> path;
  bool success = true;

  while (worklist.size() > 0) {
    Node* center = worklist.pop();
    if (visited.test_set(center->_idx)) {
      continue;
    }
    stringStream ss;
    for (uint i = 0; i < center->req(); i++) {
      Node* in = center->in(i);
      if (in != nullptr) {
        worklist.push(in);
      }
    }
    uint failures = 0;
    for (int i = 0; i < _checks.length(); ++i) {
      stringStream ss2;
      path.clear();
      steps.clear();
      steps.push(center);
      switch (_checks.at(i)->check(center, steps, path, ss2)) {
      case LocalGraphInvariant::CheckResult::FAILED:
        failures++;
        ss.print_cr("# %s:", _checks.at(i)->name());
        ss.print_cr("%s", ss2.freeze());
        break;
      case LocalGraphInvariant::CheckResult::NOT_APPLICABLE:
      case LocalGraphInvariant::CheckResult::VALID:
        break;
      }
    }
    if (failures > 0) {
      success = false;
      ttyLocker ttyl;
      tty->print("%d failure%s for node\n", failures, failures == 1 ? "" : "s");
      center->dump();
      tty->print_cr("%s", ss.freeze());
    }
  }

  return success;
#endif
}
