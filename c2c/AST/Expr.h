/* Copyright 2013 Bas van den Berg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXPR_H
#define EXPR_H

#include <string>
#include <vector>

#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/APFloat.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/AST/OperationKinds.h>

#include "AST/OwningVector.h"
#include "AST/Stmt.h"
#include "AST/Type.h"

namespace C2 {

class StringBuilder;
class Package;
class Decl;
class VarDecl;

enum ExprKind {
    EXPR_INTEGER_LITERAL=0,
    EXPR_FLOAT_LITERAL,
    EXPR_BOOL_LITERAL,
    EXPR_CHAR_LITERAL,
    EXPR_STRING_LITERAL,
    EXPR_NIL,
    EXPR_IDENTIFIER,
    EXPR_TYPE,
    EXPR_CALL,
    EXPR_INITLIST,
    EXPR_DECL,
    EXPR_BINOP,
    EXPR_CONDOP,
    EXPR_UNARYOP,
    EXPR_BUILTIN,
    EXPR_ARRAYSUBSCRIPT,
    EXPR_MEMBER,
    EXPR_PAREN,
};


class Expr : public Stmt {
public:
    Expr(ExprKind k);
    virtual ~Expr();
    // from Stmt
    static bool classof(const Stmt* S) {
        return S->getKind() == STMT_EXPR;
    }

    ExprKind getKind() const {
        return static_cast<ExprKind>(StmtBits.eKind);
    }

    virtual clang::SourceRange getSourceRange() const {
        return clang::SourceRange();
    }
private:
    Expr(const Expr&);
    Expr& operator= (const Expr&);
};


typedef std::vector<C2::Expr*> ExprList;

class IntegerLiteral : public Expr {
public:
    IntegerLiteral(SourceLocation loc_, const llvm::APInt& V)
        : Expr(EXPR_INTEGER_LITERAL)
        , Value(V), loc(loc_) {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_INTEGER_LITERAL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }

    llvm::APInt Value;
private:
    clang::SourceLocation loc;
};


class FloatingLiteral : public Expr {
public:
    FloatingLiteral(SourceLocation loc_, const llvm::APFloat& V)
        : Expr(EXPR_FLOAT_LITERAL)
        , Value(V), loc(loc_) {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_FLOAT_LITERAL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }

    llvm::APFloat Value;
    clang::SourceLocation loc;
};


class BooleanLiteral : public Expr {
public:
    BooleanLiteral(SourceLocation loc_, bool val)
        : Expr(EXPR_BOOL_LITERAL)
        , loc(loc_)
    {
        StmtBits.BoolLiteralValue = val;
    }
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_BOOL_LITERAL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }
    bool getValue() const { return StmtBits.BoolLiteralValue; }

    clang::SourceLocation loc;
};


class CharacterLiteral : public Expr {
public:
    CharacterLiteral(SourceLocation loc_, unsigned val)
        : Expr(EXPR_CHAR_LITERAL)
        , value(val), loc(loc_) {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_CHAR_LITERAL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }

    // TODO use StmtBits (need to use union then)
    unsigned value;
    clang::SourceLocation loc;
};


class StringLiteral : public Expr {
public:
    StringLiteral(SourceLocation loc_, const std::string& val)
        : Expr(EXPR_STRING_LITERAL)
        , value(val), loc(loc_) {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_STRING_LITERAL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }

    std::string value;
    clang::SourceLocation loc;
};


class NilExpr : public Expr {
public:
    NilExpr(SourceLocation loc_)
        : Expr(EXPR_NIL)
        , loc(loc_) {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_NIL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }

    clang::SourceLocation loc;
};


class IdentifierExpr : public Expr {
public:
    IdentifierExpr(SourceLocation loc_, const std::string& name_)
        : Expr(EXPR_IDENTIFIER)
        , name(name_), decl(0), loc(loc_) {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_IDENTIFIER;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return loc; }

    const std::string& getName() const { return name; }
    void setDecl(Decl* decl_) { decl = decl_; }
    Decl* getDecl() const { return decl; }
private:
    std::string name;
    Decl* decl;   // set during analysis
    clang::SourceLocation loc;
};


class TypeExpr : public Expr {
public:
    TypeExpr(QualType& QT_)
        : Expr(EXPR_TYPE)
        , QT(QT_)
    {}
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_TYPE;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const {
        SourceLocation loc;
        return loc;
    }
    QualType& getType() { return QT; }
    void setType(QualType& QT_) { QT = QT_; }

    void setLocalQualifier() { StmtBits.TypeExprIsLocal = true; }
    bool hasLocalQualifier() const { return StmtBits.TypeExprIsLocal; }
private:
    QualType QT;
};


class CallExpr : public Expr {
public:
    CallExpr(Expr* Fn_)
        : Expr(EXPR_CALL)
        , Fn(Fn_)
    {}
    virtual ~CallExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_CALL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return Fn->getLocation(); }

    void addArg(Expr* arg);

    Expr* getFn() const { return Fn; }
    Expr* getArg(unsigned i) const { return args[i]; }
    unsigned numArgs() const { return args.size(); }
private:
    // TODO add R/LParen
    Expr* Fn;
    typedef OwningVector<Expr> Args;
    Args args;
};


class InitListExpr : public Expr {
public:
    InitListExpr(SourceLocation lbraceLoc, SourceLocation rbraceLoc, ExprList& values_);
    virtual ~InitListExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_INITLIST;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return leftBrace; }

    ExprList& getValues() { return values; }
private:
    SourceLocation leftBrace;
    SourceLocation rightBrace;
    ExprList values;
};


class DeclExpr : public Expr {
public:
    DeclExpr(VarDecl* decl_);
    virtual ~DeclExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_DECL;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;

    const std::string& getName() const;
    virtual SourceLocation getLocation() const;
    QualType getType() const;
    Expr* getInitValue() const;
    bool hasLocalQualifier() const;
    VarDecl* getDecl() const { return decl; }
private:
    VarDecl* decl;
};


class BinaryOperator : public Expr {
public:
    typedef clang::BinaryOperatorKind Opcode;
    static const char* OpCode2str(clang::BinaryOperatorKind opc);

    BinaryOperator(Expr* lhs, Expr* rhs, Opcode opc, SourceLocation opLoc);
    virtual ~BinaryOperator();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_BINOP;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return opLoc; }

    Expr* getLHS() const { return lhs; }
    Expr* getRHS() const { return rhs; }
    Opcode getOpcode() const { return opc; }
private:
    SourceLocation opLoc;
    Opcode opc;
    Expr* lhs;
    Expr* rhs;
};


class ConditionalOperator : public Expr {
public:
    ConditionalOperator(SourceLocation questionLoc, SourceLocation colonLoc,
                    Expr* cond_, Expr* lhs_, Expr* rhs_);
    virtual ~ConditionalOperator();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_CONDOP;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return cond->getLocation(); }

    Expr* getCond() const { return cond; }
    Expr* getLHS() const { return lhs; }
    Expr* getRHS() const { return rhs; }
private:
    SourceLocation QuestionLoc;
    SourceLocation ColonLoc;
    Expr* cond;
    Expr* lhs;
    Expr* rhs;
};


class UnaryOperator : public Expr {
public:
    typedef clang::UnaryOperatorKind Opcode;
    static const char* OpCode2str(clang::UnaryOperatorKind opc);

    UnaryOperator(SourceLocation opLoc_, Opcode opc, Expr* val_);
    virtual ~UnaryOperator();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_UNARYOP;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return opLoc; }

    Expr* getExpr() const { return val; }
    Opcode getOpcode() const { return opc; }
    SourceLocation getOpLoc() const { return opLoc; }
private:
    SourceLocation opLoc;
    Opcode opc;
    Expr* val;
};


class BuiltinExpr : public Expr {
public:
    BuiltinExpr(SourceLocation Loc_, Expr* expr_, bool isSizeof_)
        : Expr(EXPR_BUILTIN)
        , Loc(Loc_)
        , expr(expr_)
    {
        StmtBits.BuiltInIsSizeOf = isSizeof_;
    }
    virtual ~BuiltinExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_BUILTIN;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return Loc; }

    Expr* getExpr() const { return expr; }
    bool isSizeof() const { return StmtBits.BuiltInIsSizeOf; }
private:
    SourceLocation Loc;
    Expr* expr;
};


class ArraySubscriptExpr : public Expr {
public:
    ArraySubscriptExpr(SourceLocation RLoc_, Expr* Base_, Expr* Idx_);
    virtual ~ArraySubscriptExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_ARRAYSUBSCRIPT;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return base->getLocation(); }

    Expr* getBase() const { return base; }
    Expr* getIndex() const { return idx; }
private:
    SourceLocation RLoc;
    Expr* base;
    Expr* idx;
};


class MemberExpr : public Expr {
public:
    MemberExpr(Expr* Base_, bool isArrow_, IdentifierExpr* Member_)
        : Expr(EXPR_MEMBER)
        , Base(Base_)
        , Member(Member_)
    {
        StmtBits.MemberExprIsArrow = isArrow_;
    }
    virtual ~MemberExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_MEMBER;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return Base->getLocation(); }

    Expr* getBase() const { return Base; }
    IdentifierExpr* getMember() const { return Member; }
    bool isArrow() const { return StmtBits.MemberExprIsArrow; }
    void setPkgPrefix(bool v) { StmtBits.MemberExprIsPkgPrefix = v; }
    bool isPkgPrefix() const { return StmtBits.MemberExprIsPkgPrefix; }

    // NOTE: uses static var
    const char* getFullName() const;
private:
    Expr* Base;
    IdentifierExpr* Member;
};


class ParenExpr : public Expr {
public:
    ParenExpr(SourceLocation l, SourceLocation r, Expr* val)
        : Expr(EXPR_PAREN)
        , L(l), R(r), Val(val)
    {}
    virtual ~ParenExpr();
    static bool classof(const Expr* E) {
        return E->getKind() == EXPR_PAREN;
    }
    virtual void print(StringBuilder& buffer, unsigned indent) const;
    virtual SourceLocation getLocation() const { return L; }

    Expr* getExpr() const { return Val; }
    virtual clang::SourceRange getSourceRange() const { return clang::SourceRange(L, R); }
    SourceLocation getLParen() const { return L; }
    SourceLocation getRParen() const { return R; }
private:
    SourceLocation L, R;
    Expr* Val;
};


template <class T> static inline bool isa(const Expr* E) {
    return T::classof(E);
}

template <class T> static inline T* dyncast(Expr* E) {
    if (isa<T>(E)) return static_cast<T*>(E);
    return 0;
}

template <class T> static inline const T* dyncast(const Expr* E) {
    if (isa<T>(E)) return static_cast<const T*>(E);
    return 0;
}

//#define CAST_DEBUG
#ifdef CAST_DEBUG
#include <assert.h>
#endif

template <class T> static inline T* cast(Expr* E) {
#ifdef CAST_DEBUG
    assert(isa<T>(E));
#endif
    return static_cast<T*>(E);
}

template <class T> static inline const T* cast(const Expr* E) {
#ifdef CAST_DEBUG
    assert(isa<T>(E));
#endif
    return static_cast<const T*>(E);
}

}

#endif

