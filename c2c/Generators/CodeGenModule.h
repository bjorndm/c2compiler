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

#ifndef CODEGEN_MODULE_H
#define CODEGEN_MODULE_H

#include <string>
#include <vector>

#include <llvm/IR/IRBuilder.h>

namespace llvm {
class Module;
class LLVMContext;
class Type;
class Function;
}

namespace C2 {

class AST;
class Decl;
class VarDecl;
class Expr;
class Type;
class Package;
class StringLiteral;

// generates LLVM Module from (multiple) ASTs
class CodeGenModule {
public:
    CodeGenModule(const Package* pkg_);
    ~CodeGenModule();

    void addEntry(AST& ast) { entries.push_back(&ast); }
    void generate();
    bool verify();
    void write(const std::string& target, const std::string& name);
    void dump();

    llvm::Type* ConvertType(const C2::Type* type);
    llvm::Function* createExternal(const Package* P, const std::string& name);

    const Package* getPackage() const { return pkg; }
    llvm::Module* getModule() const { return module; }
    llvm::LLVMContext& getContext() const { return context; }
    llvm::IRBuilder<> getBuilder() const { return builder; }

    llvm::Value* EvaluateExprAsBool(const Expr *E);
private:
    void EmitGlobalVariable(VarDecl* V);
    void EmitTopLevelDecl(Decl* D);

    llvm::Constant* EvaluateExprAsConstant(const Expr *E);
    llvm::Constant* GetConstantArrayFromStringLiteral(const StringLiteral* E);

    const Package* pkg;

    typedef std::vector<AST*> Entries;
    typedef Entries::iterator EntriesIter;
    Entries entries;

    llvm::LLVMContext& context;
    llvm::Module* module;
    llvm::IRBuilder<> builder;

    CodeGenModule(const CodeGenModule&);
    CodeGenModule& operator= (const CodeGenModule&);
};

}

#endif

