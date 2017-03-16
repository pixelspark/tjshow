/* TJShow (C) Tommy van der Vorst, Pixelspark, 2005-2017.
 * 
 * This file is part of TJShow. TJShow is free software: you 
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later 
 * version.
 * 
 * TJShow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TJShow.  If not, see <http://www.gnu.org/licenses/>. */
#include "../include/internal/tjshow.h"
using namespace tj::show;

#pragma warning(push)
#pragma warning(disable: 4800 4503 4244) // small thingy in Spirit header file, decorated names too long
#pragma inline_depth(255)
#pragma inline_recursion(on)
#undef WIN32_LEAN_AND_MEAN // Defined in file_iterator.ipp
#include <boost/spirit.hpp>
#include <boost/spirit/core.hpp>
using namespace boost::spirit;

class ExpressionGrammar : public grammar<ExpressionGrammar> {
	public:
		ExpressionGrammar(strong<Variables> vars): _vars(vars) {
		}

		virtual ~ExpressionGrammar() {
		}

		ref<Expression> GetRoot() {
			if(_stack.size()>0) {
				return _stack[0];
			}
			return null;
		}

		template <typename ScannerT> struct definition {
			definition(ExpressionGrammar const& self) {
				distinct_parser<wchar_t> keyword_p(L"a-zA-Z0-9_");

				integer =	lexeme_d[ int_p[PushConstant<int>(self._stack)] ];
				real	=	lexeme_d[ real_p[PushConstant<double>(self._stack)] ];
				nullValue =	ch_p(L'?')[PushNull(self._stack)];
				stringValue = lexeme_d[confix_p('"', ((*c_escape_ch_p)[PushString(self._stack)]), '"')];

				identifier=	lexeme_d[(alpha_p >> *(alnum_p|ch_p('_')))][PushVariable(self._stack, self._vars)]
							| lexeme_d[ ch_p(L'<') >> (alpha_p >> *(alnum_p|ch_p(L'_')|ch_p(L' ')))[PushVariable(self._stack, self._vars)] >> ch_p(L'>') ]; 
				
				intrinsic = ch_p('[') >> lexeme_d[alpha_p >> *(alnum_p|ch_p(L'_')|ch_p(L' '))][PushNullary(self._stack)] >> ch_p(L']');

				factor =	eps_p(real_p) >> real
							| integer
							| intrinsic
							| stringValue
							| (keyword_p(L"sin") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Sin)] >> ch_p(L')'))
							| (keyword_p(L"cos") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Cos)] >> ch_p(L')'))
							| (keyword_p(L"tan") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Tan)] >> ch_p(L')'))
							| (keyword_p(L"asin") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Asin)] >> ch_p(L')'))
							| (keyword_p(L"acos") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Acos)] >> ch_p(L')'))
							| (keyword_p(L"atan") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Atan)] >> ch_p(')'))
							| (keyword_p(L"abs") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Abs)] >> ch_p(')'))
							| (keyword_p(L"log") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Log)] >> ch_p(')'))
							| (keyword_p(L"sqrt") >> ch_p(L'(') >> condition[FoldUnary(self._stack, UnaryExpression::Sqrt)] >> ch_p(')'))
							| (ch_p(L'\u221A') >> condition[FoldUnary(self._stack, UnaryExpression::Sqrt)])
							| identifier
							| nullValue
							| (L'(' >> condition >> L')')
							| (L'-' >> factor)[FoldUnary(self._stack, UnaryExpression::NegateNumber)]
							| ((ch_p(L'!') | ch_p(L'¬')) >> factor)[FoldUnary(self._stack, UnaryExpression::Negate)]
							;

				fieldFactor = factor >> *(fieldOperator);
				term =		fieldFactor >> *(multiplyOperator | divideOperator | moduloOperator);
				addedTerm = term >> *(addOperator | subtractOperator);
				comparedTerm = addedTerm >> *(gtOperator | ltOperator | equalsOperator | notEqualsOperator);
				logicalTerm = comparedTerm >> *(andOperator | orOperator | xorOperator);
				condition =	logicalTerm;

				// Binary rules
				addOperator = (L'+' >> term)[FoldBinary(self._stack, BinaryExpression::Add)];
				subtractOperator = (L'-' >> term)[FoldBinary(self._stack, BinaryExpression::Subtract)];
				multiplyOperator = ((ch_p(L'*') | ch_p(L'\u00D7')) >> term)[FoldBinary(self._stack, BinaryExpression::Multiply)];
				divideOperator = ((ch_p(L'/') | ch_p(L'\u00F7')) >> term)[FoldBinary(self._stack, BinaryExpression::Divide)];
				moduloOperator = (L'%' >> term)[FoldBinary(self._stack, BinaryExpression::Modulo)];
				andOperator = (lexeme_d[((ch_p(L'&') >> ch_p(L'&'))|str_p(L"\r\n"))] >> term)[FoldBinary(self._stack, BinaryExpression::And)];
				orOperator = (lexeme_d[ch_p(L'|') >> ch_p(L'|')] >> term)[FoldBinary(self._stack, BinaryExpression::Or)];
				xorOperator = (lexeme_d[ch_p(L'^') >> ch_p(L'^')] >> term)[FoldBinary(self._stack, BinaryExpression::Xor)];
				fieldOperator = (keyword_p(L"->") >> term)[FoldBinary(self._stack, BinaryExpression::Field)];
				ltOperator = (L'<' >> term)[FoldBinary(self._stack, BinaryExpression::Smaller)];
				gtOperator = (L'>' >> term)[FoldBinary(self._stack, BinaryExpression::Greater)];
				equalsOperator = (lexeme_d[ch_p(L'=') >> *ch_p(L'=')] >> term)[FoldBinary(self._stack, BinaryExpression::Equals)];
				notEqualsOperator = (lexeme_d[(ch_p(L'!') >> ch_p(L'=')) | ch_p(L'\u2260')] >> term)[FoldBinary(self._stack, BinaryExpression::NotEquals)];
			}
			
			rule<ScannerT> const& start() const { 
				return condition;
			}

			// Basic rules
			rule<ScannerT> integer, factor, term, condition, comparedTerm, logicalTerm, fieldFactor, addedTerm, identifier, nullValue, intrinsic, stringValue, real, fieldOperator;

			// Binary rules
			rule<ScannerT> multiplyOperator, divideOperator, addOperator, moduloOperator, subtractOperator, orOperator, andOperator, xorOperator, ltOperator, gtOperator, equalsOperator, notEqualsOperator;
		};

	protected:
		template<typename T> struct PushConstant {
			PushConstant(std::deque< strong<Expression> >& stack): _stack(stack) {
			}

			template<typename T> inline void operator()(const T& val) const {		
				_stack.push_back(strong<Expression>(GC::Hold(new ConstExpression(Any(val)))));
			}

			mutable std::deque< strong<Expression> >& _stack;
		};

		struct PushNull {
			PushNull(std::deque< strong<Expression> >& stack): _stack(stack) {
			}

			template<typename T> inline void operator()(const T& val) const {		
				_stack.push_back(strong<Expression>(GC::Hold(new NullaryExpression(NullaryExpression::NullaryNull))));
			}

			mutable std::deque< strong<Expression> >& _stack;
		};

		struct PushVariable {
			PushVariable(std::deque< strong<Expression> >& stack, strong<Variables> vars): _vars(vars), _stack(stack) {
			}

			template<typename T> inline void operator()(const T& start, const T& end) const {
				std::wstring varName(start, end);
				ref<Variable> var = _vars->GetVariableByName(varName);
				if(var) {
					_stack.push_back(ref<Expression>(GC::Hold(new VariableExpression(var->GetID()))));
				}
				else {
					_stack.push_back(ref<Expression>(GC::Hold(new NullExpression())));
				}
			}

			mutable std::deque< strong<Expression> >& _stack;
			mutable strong<Variables> _vars;
		};

		struct PushNullary {
			PushNullary(std::deque< strong<Expression> >& stack): _stack(stack) {
			}

			template<typename T> inline void operator()(const T& start, const T& end) const {
				std::wstring varName(start, end);
				NullaryExpression::Nullary type = NullaryExpression::NullaryEnumeration.Unserialize(varName, NullaryExpression::NullaryNull);
				_stack.push_back(ref<Expression>(GC::Hold(new NullaryExpression(type))));
			}

			mutable std::deque< strong<Expression> >& _stack;
		};

		struct PushString {
			PushString(std::deque< strong<Expression> >& stack): _stack(stack) {
			}

			template<typename T> inline void operator()(const T& start, const T& end) const {
				std::wstring str(start, end);
				_stack.push_back(ref<Expression>(GC::Hold(new ConstExpression(str))));
			}

			mutable std::deque< strong<Expression> >& _stack;
		};

		struct FoldBinary {
			FoldBinary(std::deque< strong<Expression> >& stack, const BinaryExpression::Binary& type): _stack(stack), _type(type) {
			}

			template<typename T> inline void operator()(const T& val) const {		
				Fold();
			}

			template<typename T> inline void operator()(const T&, const T&) const {
				Fold();
			}

			void Fold() const {
				if(_stack.size()<2) {
					Throw(L"Cannot fold to binary condition when there is nothing on the stack!", ExceptionTypeError);
				}

				strong<Expression> last = *(_stack.rbegin()); _stack.pop_back();
				strong<Expression> first = *(_stack.rbegin()); _stack.pop_back();
				ref<BinaryExpression> bc = GC::Hold(new BinaryExpression());
				bc->SetType(_type);
				bc->SetFirstOperand(first);
				bc->SetSecondOperand(last);
				_stack.push_back(ref<Expression>(bc));
			}

			mutable std::deque< strong<Expression> >& _stack;
			const BinaryExpression::Binary _type;
		};

		struct FoldUnary {
			FoldUnary(std::deque< strong<Expression> >& stack, const UnaryExpression::Unary& type): _stack(stack), _type(type) {
			}

			template<typename T> inline void operator()(const T& val) const {		
				Fold();
			}

			template<typename T> inline void operator()(const T&, const T&) const {
				Fold();
			}

			void Fold() const {
				if(_stack.size()<1) {
					Throw(L"Cannot fold to binary condition when there is nothing on the stack!", ExceptionTypeError);
				}

				strong<Expression> first = *(_stack.rbegin()); _stack.pop_back();
				ref<UnaryExpression> bc = GC::Hold(new UnaryExpression());
				bc->SetType(_type);
				bc->SetOperand(first);
				_stack.push_back(ref<Expression>(bc));
			}

			mutable std::deque< strong<Expression> >& _stack;
			const UnaryExpression::Unary _type;
		};


		mutable std::deque< strong<Expression> > _stack;
		mutable strong<Variables> _vars;
	};


void NullExpression::Parse(const std::wstring& expr, strong<Variables> vars) {
	ExpressionGrammar sparser(vars);
	parse_info<const wchar_t*> info = parse(expr.c_str(), sparser, space_p);
	if(!info.full) {
		Throw((std::wstring(L"Parsing stopped at: ")+info.stop).c_str(), ExceptionTypeError);
	}
	_operand = sparser.GetRoot()->Fold(vars);
}

#pragma warning(pop)