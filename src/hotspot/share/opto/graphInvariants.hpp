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

#ifndef SHARE_OPTO_GRAPHINVARIANTS_HPP
#define SHARE_OPTO_GRAPHINVARIANTS_HPP

#include "node.hpp"
#include "memory/allocation.hpp"

class LocalGraphInvariant : public ResourceObj {
public:
  enum class CheckResult {
    VALID,
    FAILED,
    NOT_APPLICABLE,
  };

  virtual const char* name() const = 0;
  virtual CheckResult check(const Node* center, Node_List& steps, GrowableArray<int>& path, stringStream&) const = 0;
};

class GraphInvariantChecker : public ResourceObj {
  GrowableArray<const LocalGraphInvariant*> _checks;

public:
  static GraphInvariantChecker* make_default();
  bool run(const Compile*) const;
};

#endif // SHARE_OPTO_GRAPHINVARIANTS_HPP