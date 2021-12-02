#include "Jitter_CodeGen_Wasm.h"
#include "WasmDefs.h"
#include "WasmModuleBuilder.h"

using namespace Jitter;

void CCodeGen_Wasm::PushRelative64(CSymbol* symbol)
{
	PushRelativeAddress(symbol);

	m_functionStream.Write8(Wasm::INST_I64_LOAD);
	m_functionStream.Write8(0x03);
	m_functionStream.Write8(0x00);
}

void CCodeGen_Wasm::PrepareSymbol64Use(CSymbol* symbol)
{
	switch (symbol->m_type)
	{
	case SYM_RELATIVE64:
		PushRelative64(symbol);
		break;
	case SYM_CONSTANT64:
		m_functionStream.Write8(Wasm::INST_I64_CONST);
		CWasmModuleBuilder::WriteSLeb128(m_functionStream, symbol->GetConstant64());
		break;
	default:
		assert(false);
		break;
	}
}

void CCodeGen_Wasm::Emit_Cmp64_MemMemAny(const STATEMENT& statement)
{
	auto dst = statement.dst->GetSymbol().get();
	auto src1 = statement.src1->GetSymbol().get();
	auto src2 = statement.src2->GetSymbol().get();

	PrepareSymbolDef(dst);
	PrepareSymbol64Use(src1);
	PrepareSymbol64Use(src2);

	switch(statement.jmpCondition)
	{
	case CONDITION_EQ:
		m_functionStream.Write8(Wasm::INST_I64_EQ);
		break;
	case CONDITION_NE:
		m_functionStream.Write8(Wasm::INST_I64_NE);
		break;
	case CONDITION_BL:
		m_functionStream.Write8(Wasm::INST_I64_LT_U);
		break;
	case CONDITION_AB:
		m_functionStream.Write8(Wasm::INST_I64_GT_U);
		break;
	case CONDITION_LT:
		m_functionStream.Write8(Wasm::INST_I64_LT_S);
		break;
	case CONDITION_LE:
		m_functionStream.Write8(Wasm::INST_I64_LE_S);
		break;
	case CONDITION_GT:
		m_functionStream.Write8(Wasm::INST_I64_GT_S);
		break;
	default:
		assert(false);
		break;
	}

	CommitSymbol(dst);
}

CCodeGen_Wasm::CONSTMATCHER CCodeGen_Wasm::g_64ConstMatchers[] =
{
	{ OP_CMP64,          MATCH_MEMORY,         MATCH_MEMORY64,       MATCH_MEMORY64,      MATCH_NIL, &CCodeGen_Wasm::Emit_Cmp64_MemMemAny                     },
	{ OP_CMP64,          MATCH_MEMORY,         MATCH_MEMORY64,       MATCH_CONSTANT64,    MATCH_NIL, &CCodeGen_Wasm::Emit_Cmp64_MemMemAny                     },

	{ OP_MOV,            MATCH_NIL,            MATCH_NIL,            MATCH_NIL,           MATCH_NIL, nullptr                                                  },
};
