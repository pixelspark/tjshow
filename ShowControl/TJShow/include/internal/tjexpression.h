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
#ifndef _TJEXPRESSION_H
#define _TJEXPRESSION_H

namespace tj {
	namespace show {
		class Variables;

		class Expression: public virtual Object, public Serializable {
			public:
				virtual ~Expression();
				virtual Pixels GetWidth() = 0;
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> vars) = 0;
				virtual Any Evaluate(strong<Variables> v) = 0;
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels offx, Pixels offy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> vars) = 0;
				virtual void Save(TiXmlElement* parent) = 0;
				virtual void Load(TiXmlElement* you) = 0;
				virtual std::wstring ToString(strong<Variables> vars) const = 0;
				virtual bool IsComplete(strong<Variables> vars) const = 0;
				virtual bool IsConstant() const = 0;
				virtual ref<Expression> Fold(strong<Variables> vars);
				virtual void RemoveChild(ref<Expression> c) = 0;
				virtual void Parse(const std::wstring& expr, strong<Variables> vars);

				static ref<Expression> CreateExpressionByType(const std::wstring& type);
				
			protected:
				
				static void DrawParentheses(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme);
				static ref<Expression> ChooseExpression(Pixels x, Pixels y, ref<Wnd> parent);

				const static Pixels KDefaultOperatorWidth;
				const static Pixels KDefaultParameterWidth;
		};

		class NullExpression: public Expression {
			public:
				NullExpression();
				virtual ~NullExpression();
				virtual Pixels GetWidth();
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> vars);
				virtual Any Evaluate(strong<Variables> v);
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> vars);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring ToString(strong<Variables> vars) const;
				virtual bool IsComplete(strong<Variables> vars) const;
				virtual bool HasChild() const;
				virtual void RemoveChild(ref<Expression> c);
				virtual void Parse(const std::wstring& expr, strong<Variables> vars);
				virtual bool IsConstant() const;

			protected:
				ref<Expression> _operand;
		};

		class ExpressionProperty: public Property {
			public:
				ExpressionProperty(const std::wstring& name, strong<Expression> c, strong<Variables> vars);
				virtual ~ExpressionProperty();
				virtual ref<Wnd> GetWindow();
				virtual void Update();

			protected:
				strong<Expression> _condition;
				strong<Variables> _variables;
				ref<Wnd> _wnd;
		};

		class Assignment: public virtual Object, public Serializable {
			public:
				Assignment();
				virtual ~Assignment();
				virtual void SetVariableId(const std::wstring& id);
				virtual ref<Expression> GetExpression();
				virtual const std::wstring& GetVariableId() const;

				virtual void Load(TiXmlElement* t);
				virtual void Save(TiXmlElement* parent);

			protected:
				std::wstring _id;
				ref<Expression> _value;
		};

		class Assignments: public virtual Object, public Serializable {
			friend class VariableList;

			public:
				Assignments();
				virtual ~Assignments();
				virtual void Add(ref<Assignment> as);
				virtual void Load(TiXmlElement* t);
				virtual void Save(TiXmlElement* t);
				virtual bool IsEmpty() const;
				virtual ref<Assignment> GetAssignmentByIndex(int id);
				virtual void Remove(ref<Assignment> a);
				virtual int GetAssignmentCount() const;
				virtual void MoveUp(ref<Assignment> a);
				virtual void MoveDown(ref<Assignment> a);

			protected:
				std::vector< ref<Assignment> > _assignments;
		};

		class AssignmentsProperty: public Property {
			public:
				AssignmentsProperty(const std::wstring& name, ref<Assignments> c, strong<Variables> vars);
				virtual ~AssignmentsProperty();
				virtual ref<Wnd> GetWindow();
				virtual void Update();

			protected:
				ref<Assignments> _assignments;
				strong<Variables> _variables;
				ref<Wnd> _wnd;
		};

		class ConstExpression: public Expression, public Inspectable {
			public:
				ConstExpression(const Any& value = Any());
				virtual ~ConstExpression();
				virtual Pixels GetWidth();
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v);
				virtual Any Evaluate(strong<Variables> v);
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring ToString(strong<Variables> v) const;
				virtual bool IsComplete(strong<Variables> v) const;
				virtual void RemoveChild(ref<Expression> c);
				virtual ref<PropertySet> GetProperties();
				virtual bool IsConstant() const;

			protected:
				Any::Type _desiredType;
				std::wstring _value;
		};

		class NullaryExpression: public Expression, public Inspectable {
			public:
				enum Nullary {
					NullaryNull,
					NullaryHours,
					NullaryMinutes,
					NullarySeconds,
					NullaryDayOfMonth,
					NullaryDayOfWeek,
					NullaryNameOfDay,
					NullaryNameOfMonth,
					NullaryMonth,
					NullaryYear,
					NullaryRandom,
					NullaryPi,
					NullaryE,
					_LastNullary,
				};

				static Enumeration<Nullary> NullaryEnumeration;

				NullaryExpression(Nullary type = NullaryNull);
				virtual ~NullaryExpression();
				virtual Pixels GetWidth();
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v);
				virtual Any Evaluate(strong<Variables> v);
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring ToString(strong<Variables> v) const;
				virtual bool IsComplete(strong<Variables> v) const;
				virtual void RemoveChild(ref<Expression> c);
				virtual ref<PropertySet> GetProperties();
				virtual bool IsConstant() const;

			protected:
				Nullary _func;
		};

		class VariableExpression: public Expression {
			public:
				VariableExpression(const std::wstring& id = L"");
				virtual ~VariableExpression();
				virtual Pixels GetWidth();
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v);
				virtual Any Evaluate(strong<Variables> v);
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring ToString(strong<Variables> v) const;
				virtual bool IsComplete(strong<Variables> v) const;
				virtual void RemoveChild(ref<Expression> c);
				virtual bool IsConstant() const;

			protected:
				std::wstring _id;
		};

		class UnaryExpression: public Expression {
			public:
				enum Unary {
					Negate = 1,
					Abs,
					NegateNumber,
					Sin,
					Cos,
					Tan,
					Acos,
					Atan,
					Asin,
					Sqrt,
					Log,
					_UnaryLast,
				};


				UnaryExpression();
				virtual ~UnaryExpression();
				virtual Pixels GetWidth();
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> vars);
				virtual Any Evaluate(strong<Variables> v);
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring ToString(strong<Variables> v) const;
				virtual bool IsComplete(strong<Variables> v) const;
				virtual void SetType(Unary u);
				virtual void SetOperand(ref<Expression> c);
				virtual void RemoveChild(ref<Expression> c);
				virtual bool IsConstant() const;
				virtual ref<Expression> Fold(strong<Variables> vars);

			protected:
				std::wstring GetOpName() const;
				Unary _type;
				ref<Expression> _operand;
		};

		class BinaryExpression: public Expression {
			public:
				enum Binary {
					Add = 1,
					Subtract,
					Multiply,
					Divide,
					Modulo,
					Greater,
					Smaller,
					Equals,
					NotEquals,
					Or,
					And,
					Xor,
					Field,
					Max,
					Min,
					_BinaryLast,
				};

				BinaryExpression();
				virtual ~BinaryExpression();
				virtual Pixels GetWidth();
				virtual void Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v);
				virtual Any Evaluate(strong<Variables> v);
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring ToString(strong<Variables> v) const;
				virtual bool IsComplete(strong<Variables> v) const;
				virtual void SetType(Binary b);
				virtual void SetFirstOperand(ref<Expression> a);
				virtual void SetSecondOperand(ref<Expression> b);
				virtual bool IsConstant() const;
				virtual ref<Expression> Fold(strong<Variables> vars);

			protected:
				virtual void RemoveChild(ref<Expression> c);
				std::wstring GetOpName() const;
				Binary _type;
				ref<Expression> _operandA, _operandB;
		};
	}
}

#endif