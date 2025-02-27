/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XPathPredicate_h
#define XPathPredicate_h

#include "XPathExpressionNode.h"
#include "XPathValue.h"

namespace WebCore {

    namespace XPath {
        
        class Number FINAL : public Expression {
        public:
            explicit Number(double);

        private:
            virtual Value evaluate() const OVERRIDE;
            virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }

            Value m_value;
        };

        class StringExpression FINAL : public Expression {
        public:
            explicit StringExpression(String&&);

        private:
            virtual Value evaluate() const OVERRIDE;
            virtual Value::Type resultType() const OVERRIDE { return Value::StringValue; }

            Value m_value;
        };

        class Negative FINAL : public Expression {
        public:
            explicit Negative(std::unique_ptr<Expression>);

        private:
            virtual Value evaluate() const OVERRIDE;
            virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }
        };

        class NumericOp FINAL : public Expression {
        public:
            enum Opcode { OP_Add, OP_Sub, OP_Mul, OP_Div, OP_Mod };
            NumericOp(Opcode, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs);

        private:
            virtual Value evaluate() const OVERRIDE;
            virtual Value::Type resultType() const OVERRIDE { return Value::NumberValue; }

            Opcode m_opcode;
        };

        class EqTestOp FINAL : public Expression {
        public:
            enum Opcode { OP_EQ, OP_NE, OP_GT, OP_LT, OP_GE, OP_LE };
            EqTestOp(Opcode, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs);
            virtual Value evaluate() const OVERRIDE;

        private:
            virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
            bool compare(const Value&, const Value&) const;

            Opcode m_opcode;
        };

        class LogicalOp FINAL : public Expression {
        public:
            enum Opcode { OP_And, OP_Or };
            LogicalOp(Opcode, std::unique_ptr<Expression> lhs, std::unique_ptr<Expression> rhs);

        private:
            virtual Value::Type resultType() const OVERRIDE { return Value::BooleanValue; }
            bool shortCircuitOn() const;
            virtual Value evaluate() const OVERRIDE;

            Opcode m_opcode;
        };

        class Union FINAL : public Expression {
        public:
            Union(std::unique_ptr<Expression>, std::unique_ptr<Expression>);

        private:
            virtual Value evaluate() const OVERRIDE;
            virtual Value::Type resultType() const OVERRIDE { return Value::NodeSetValue; }
        };

        bool evaluatePredicate(const Expression&);
        bool predicateIsContextPositionSensitive(const Expression&);

    }

}

#endif // XPathPredicate_h
