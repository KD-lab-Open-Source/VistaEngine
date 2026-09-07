#include "stdafx.h"

#include "Serialization/Serialization.h"

#include "FormulaString.h"

#include <cctype>
#include <cstdlib>
#include <string>

// Recursive-descent evaluator for the parameter formulas. Replaces the original
// Boost.Spirit grammar, whose vendored Boost doesn't build under clang/C++20.
//
// Grammar (unchanged from the Spirit version, whitespace skipped between tokens):
//   expression := term (('+' term) | ('-' term))*
//   term       := factor (('*' factor) | ('%' factor) | ('/' factor))*
//   factor     := number | name | 'quoted name' | '(' expression ')'
//                        | '-' factor | '+' factor
//
// A name is either X/x (the parameter's own raw value) or a lookup into the
// parameter table; a quoted name may contain spaces and '*' mask characters.
// Quirks kept deliberately, because the game's data was authored against them:
//   * unary '-'/'+' before a non-literal parse but do NOT negate (a number's
//     sign is consumed by the number itself, so this only shows up in "-X");
//   * division (and '%') by ~zero yields FLT_INF rather than failing.
namespace {

typedef FormulaString::ParserCallback ParserCallback;
typedef FormulaString::LookupFunction LookupFunction;

class Calculator {
public:
	Calculator(const char* text, float x_value, LookupFunction lookup_function,
	           ParserCallback var_func, ParserCallback bad_var_func, ParserCallback op_func)
	: p_(text)
	, x_value_(x_value)
	, lookup_func_(lookup_function)
	, var_func_(var_func)
	, bad_var_func_(bad_var_func)
	, op_func_(op_func)
	, bad_vars_(false)
	, syntax_error_(false)
	{}

	// Parses the whole input; true only if it is syntactically valid and fully
	// consumed (Spirit's parse_info::full).
	bool parse(float& result)
	{
		skipSpace();
		float value = expression();
		skipSpace();
		if(syntax_error_ || *p_)
			return false;
		result = value;
		return true;
	}

	bool haveUndefinedNames() const { return bad_vars_; }

private:
	void skipSpace() { while(*p_ && isspace((unsigned char)*p_)) ++p_; }

	// [SurMap5Qt] Cycle/stack guard: a recursive ParameterValue chain can blow the
	// stack long before Calculator::parse unwinds. The counter caps
	// the expression() depth; the cycle breaks at 512 nested calls and
	// returns 0 (the original Parameters.cpp:282 already returns 0 on
	// a self-cycle — the same value is fine for a P1 -> P2 -> P1
	// cycle where neither ParameterValue detects it). Added for the
	// SurMap5Qt editor world load; delete together with the guard in
	// Units/Parameters.cpp when the cyclic .prm formulas are fixed.
	int& depthCounter()
	{
		static thread_local int t_depth = 0;
		return t_depth;
	}

	float expression()
	{
		int& depth = depthCounter();
		++depth;
		if(depth > 512){
			--depth;
			return 0.f;
		}
		if(depth == 50 || depth == 100 || depth == 200 || depth == 300 || depth == 500){
			fprintf(stderr, "FormulaString: expression() depth=%d near '%.40s'\n", depth, p_);
		}
		float value = term();
		for(;;){
			skipSpace();
			char op = *p_;
			if(op != '+' && op != '-'){
				--depth;
				return value;
			}
			const char* op_pos = p_++;
			float rhs = term();
			if(syntax_error_){
				--depth;
				return 0.f;
			}
			op_func_(op_pos, op_pos + 1);
			value = op == '+' ? value + rhs : value - rhs;
		}
	}

	float term()
	{
		float value = factor();
		for(;;){
			skipSpace();
			char op = *p_;
			if(op != '*' && op != '/' && op != '%')
				return value;
			const char* op_pos = p_++;
			float rhs = factor();
			if(syntax_error_)
				return 0.f;
			op_func_(op_pos, op_pos + 1);
			if(op == '*')
				value *= rhs;
			else if(fabs(rhs) < FLT_COMPARE_TOLERANCE) // matches the original's guard
				value = FLT_INF;
			else if(op == '/')
				value /= rhs;
			else
				value = value * rhs * 0.01f; // '%'
		}
	}

	float factor()
	{
		skipSpace();
		char c = *p_;

		// A number carries its own sign, so try it before the unary operators.
		if(c == '+' || c == '-' || c == '.' || isdigit((unsigned char)c)){
			char* end = 0;
			float value = strtof(p_, &end);
			if(end != p_){
				p_ = end;
				return value;
			}
		}

		if(c == '('){
			++p_;
			float value = expression();
			skipSpace();
			if(*p_ != ')'){
				syntax_error_ = true;
				return 0.f;
			}
			++p_;
			return value;
		}

		if(c == '\''){
			const char* begin = ++p_;
			while(*p_ && *p_ != '\'')
				++p_;
			if(*p_ != '\''){
				syntax_error_ = true;
				return 0.f;
			}
			const char* end = p_++;
			return variable(begin, end);
		}

		if(isalnum((unsigned char)c)){
			const char* begin = p_;
			while(isalnum((unsigned char)*p_))
				++p_;
			return variable(begin, p_);
		}

		// Unary sign on a non-literal: parsed, but deliberately not negated.
		if(c == '-' || c == '+'){
			++p_;
			return factor();
		}

		syntax_error_ = true;
		return 0.f;
	}

	float variable(const char* begin, const char* end)
	{
		std::string name(begin, end);
		if(stricmp(name.c_str(), "X") == 0){
			var_func_(begin, end);
			return x_value_;
		}

		float value;
		if(lookup_func_(name.c_str(), value)){
			var_func_(begin, end);
			return value;
		}

		bad_vars_ = true;
		bad_var_func_(begin, end);
		return 0.f;
	}

	const char* p_;
	float x_value_;

	LookupFunction lookup_func_;
	ParserCallback var_func_;
	ParserCallback bad_var_func_;
	ParserCallback op_func_;

	bool bad_vars_;
	bool syntax_error_;
};

} // namespace

void FormulaString::serialize(Archive& ar)
{
	ar.serialize(formula_, "formula", "^<Формула");
}

FormulaString::EvalResult FormulaString::evaluate(float& result, float x, LookupFunction lookup_function, ParserCallback var_callback, ParserCallback badvar_callback, ParserCallback op_callback) const
{
	Calculator calc(c_str(), x, lookup_function, var_callback, badvar_callback, op_callback);

	float value;
	if(!calc.parse(value))
		return EVAL_SYNTAX_ERROR;

	result = value;
	return calc.haveUndefinedNames() ? EVAL_UNDEFINED_NAME : EVAL_SUCCESS;
}
