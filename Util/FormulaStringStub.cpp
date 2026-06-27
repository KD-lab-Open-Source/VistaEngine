// FormulaString stub for the cross-platform build.
//
// The real FormulaString.cpp parses arithmetic formulas with Boost.Spirit, an
// old vendored Boost that doesn't build cleanly under clang/C++20. Until it is
// ported, formulas evaluate to a syntax error (callers fall back to defaults).
#include <string>
using namespace std;

#include "FormulaString.h"

FormulaString::EvalResult FormulaString::evaluate(
	float& /*result*/, float /*x*/, LookupFunction /*lookup_function*/,
	ParserCallback /*var_callback*/, ParserCallback /*badvar_callback*/,
	ParserCallback /*op_callback*/) const
{
	return EVAL_SYNTAX_ERROR;
}

void FormulaString::serialize(Archive& /*ar*/) {}
