#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

template <uint32 OP>
void CCodeGen_Wasm::Emit_Fpu_MemMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	m_functionStream.Write8(OP);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::PushRelativeSingle(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_F32_LOAD);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::Emit_Fp_Cmp_AnyMemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);
	PrepareSymbolUse(src2);

	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_functionStream.Write8(Wasm::INST_F32_EQ);
		break;
	case CONDITION_BL:
		m_functionStream.Write8(Wasm::INST_F32_LT);
		break;
	case CONDITION_BE:
		m_functionStream.Write8(Wasm::INST_F32_LE);
		break;
	default:
		assert(false);
		break;
	}

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_Mov_MemSRelI32(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(src1->m_type == SYM_FP_REL_INT32);

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_F32_CONVERT_I32_S);

	CommitSymbol(dst);
}

void CCodeGen_Wasm::Emit_Fp_ToIntTrunc_MemMem(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();

	assert(dst->m_type == SYM_FP_REL_SINGLE);

	PrepareSymbolDef(dst);
	PrepareSymbolUse(src1);

	m_functionStream.Write8(Wasm::INST_I32_TRUNC_F32_S);

	//Can't use CommitSymbol here since dst is a FP_SINGLE
	m_functionStream.Write8(Wasm::INST_I32_STORE);
	m_functionStream.Write8(0x02);
	m_functionStream.Write8(0x00);
}

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_fpuConstMatchers[] =
{
	{ OP_FP_ADD,         MATCH_MEMORY_FP_SINGLE,      MATCH_MEMORY_FP_SINGLE,  MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_ADD>       },
	{ OP_FP_SUB,         MATCH_MEMORY_FP_SINGLE,      MATCH_MEMORY_FP_SINGLE,  MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_SUB>       },
	{ OP_FP_MUL,         MATCH_MEMORY_FP_SINGLE,      MATCH_MEMORY_FP_SINGLE,  MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_MUL>       },
	{ OP_FP_DIV,         MATCH_MEMORY_FP_SINGLE,      MATCH_MEMORY_FP_SINGLE,  MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fpu_MemMemMem<Wasm::INST_F32_DIV>       },

	{ OP_FP_CMP,         MATCH_ANY,                   MATCH_MEMORY_FP_SINGLE,  MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_Cmp_AnyMemMem                        },

	{ OP_MOV,            MATCH_MEMORY_FP_SINGLE,      MATCH_RELATIVE_FP_INT32, MATCH_NIL,               MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_Mov_MemSRelI32                       },
	{ OP_FP_TOINT_TRUNC, MATCH_MEMORY_FP_SINGLE,      MATCH_MEMORY_FP_SINGLE,  MATCH_NIL,               MATCH_NIL,      &CCodeGen_Wasm::Emit_Fp_ToIntTrunc_MemMem                    },

	{ OP_MOV,            MATCH_NIL,                   MATCH_NIL,               MATCH_NIL,               MATCH_NIL,      nullptr                                                      },
};