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
#include <algorithm>
using namespace tj::show;
using namespace tj::shared::graphics;

/* Expression */
const Pixels Expression::KDefaultOperatorWidth = 24;
const Pixels Expression::KDefaultParameterWidth = 24;

Expression::~Expression() {
}

void Expression::Parse(const std::wstring& expr, strong<Variables> vars) {
}

ref<Expression> Expression::Fold(strong<Variables> v) {
	return this;
}

/* BinaryExpression */
BinaryExpression::BinaryExpression(): _type(Add) {
}

BinaryExpression::~BinaryExpression() {
}

void BinaryExpression::SetType(Binary b) {
	_type = b;
}

void BinaryExpression::SetFirstOperand(ref<Expression> a) {
	_operandA = a;
}

void BinaryExpression::SetSecondOperand(ref<Expression> b) {
	_operandB = b;
}

void BinaryExpression::RemoveChild(ref<Expression> c) {
	if(_operandA==c) _operandA = 0;
	if(_operandB==c) _operandB = 0;
}

bool BinaryExpression::IsConstant() const {
	return (!_operandA || _operandA->IsConstant()) && (!_operandB || _operandB->IsConstant());
}

ref<Expression> BinaryExpression::Fold(strong<Variables> vars) {
	if(_operandA) _operandA = _operandA->Fold(vars);
	if(_operandB) _operandB = _operandB->Fold(vars);

	if(IsConstant()) {
		return GC::Hold(new ConstExpression(Evaluate(vars)));
	}
	return this;
}

Pixels BinaryExpression::GetWidth() {
	Pixels w = Expression::KDefaultOperatorWidth;

	if(_operandA) {
		w += _operandA->GetWidth();
	}
	else {
		w += Expression::KDefaultParameterWidth;
	}

	if(_operandB) {
		w += _operandB->GetWidth();
	}
	else {
		w += Expression::KDefaultParameterWidth;
	}

	return w + 4 +4; // +4 for end parenthesis
}

void Expression::DrawParentheses(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme) {
	// draw parentheses
	SolidBrush lineBr(theme->GetColor(Theme::ColorLine));
	Pen line(&lineBr, 1.0f);

	// left side
	PointF left[4];
	left[0] = PointF((float)a.GetLeft()+4.0f, (float)a.GetTop());
	left[1] = PointF((float)a.GetLeft()+1.0f, (float)a.GetTop());
	left[2] = PointF((float)a.GetLeft()+1.0f, (float)a.GetBottom()-1.0f);
	left[3] = PointF((float)a.GetLeft()+3.0f, (float)a.GetBottom()-1.0f);
	g.DrawCurve(&line, left, 4); // was DrawLines

	// right side
	PointF	right[4];
	right[0] = PointF((float)a.GetRight()-4.0f, (float)a.GetTop());
	right[1] = PointF((float)a.GetRight()-1.0f, (float)a.GetTop());
	right[2] = PointF((float)a.GetRight()-1.0f, (float)a.GetBottom()-1.0f);
	right[3] = PointF((float)a.GetRight()-4.0f, (float)a.GetBottom()-1.0f);
	g.DrawCurve(&line, right, 4); // was DrawLines
}

void BinaryExpression::Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> vars) {
	Area area = a;
	area.Narrow(4,0,4,0); // parenthesis

	if(_operandA) {
		Area first = area;
		first.SetWidth(_operandA->GetWidth());
		_operandA->Paint(g,first,theme, vars);
		area.Narrow(_operandA->GetWidth(),0,0,0);
	}
	else {
		area.Narrow(Expression::KDefaultParameterWidth,0,0,0);
	}

	Area op = area;
	op.SetWidth(Expression::KDefaultOperatorWidth);
	area.Narrow(Expression::KDefaultOperatorWidth,0,0,0);
	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);
	SolidBrush tbr(theme->GetColor(Theme::ColorText));

	std::wstring text = GetOpName();
	LinearGradientBrush opb(PointF(0.0f, float(a.GetTop())), PointF(0.0f, float(a.GetBottom())), theme->GetColor(Theme::ColorActiveStart), theme->GetColor(Theme::ColorActiveEnd));
	g.FillRectangle(&opb, op);
	SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
	g.FillRectangle(&disabled,op);

	g.DrawString(text.c_str(), (int)text.length(), theme->GetGUIFontBold(), op, &sf, &tbr);

	if(_operandB) {
		Area second = area;
		second.SetWidth(_operandB->GetWidth());
		_operandB->Paint(g,second,theme,vars);
	}
	else {
	}

	DrawParentheses(g,a,theme);
}

Any BinaryExpression::Evaluate(strong<Variables> v) {
	if(!_operandA || !_operandB) return Any();

	switch(_type) {
		case Add:
			return _operandA->Evaluate(v) + _operandB->Evaluate(v);
	
		case Subtract:
			return _operandA->Evaluate(v) - _operandB->Evaluate(v);

		case Multiply:
			return _operandA->Evaluate(v) * _operandB->Evaluate(v);

		case Divide: {
			Any a = _operandA->Evaluate(v);
			Any b = _operandB->Evaluate(v);
			if(b==Any(0)) return Any();
			return a / b;
		}

		case Max: {
			Any a = _operandA->Evaluate(v);
			Any b = _operandB->Evaluate(v);

			return (a > b) ? a : b;
		}

		case Min: {
			Any a = _operandA->Evaluate(v);
			Any b = _operandB->Evaluate(v);

			return (a < b) ? a : b;
		}

		case Greater:
			return (_operandA->Evaluate(v) > _operandB->Evaluate(v))?Any(true):Any(false);

		case Smaller:
			return (_operandA->Evaluate(v) < _operandB->Evaluate(v))?Any(true):Any(false);

		case Equals:
			return (_operandA->Evaluate(v) == _operandB->Evaluate(v))?Any(true):Any(false);

		case NotEquals:
			return (_operandA->Evaluate(v) != _operandB->Evaluate(v))?Any(true):Any(false);

		case Modulo: {
			Any a = _operandA->Evaluate(v);
			Any b = _operandB->Evaluate(v);
			if(b==Any(0)) return Any();
			return Any(a % b);
		}

		case Or:
			return bool(_operandA->Evaluate(v)) || bool(_operandB->Evaluate(v));

		case And:
			return bool(_operandA->Evaluate(v)) && bool(_operandB->Evaluate(v));

		case Xor: {
			bool ra = bool(_operandA->Evaluate(v));
			bool rb = bool(_operandB->Evaluate(v));
			return (ra || rb) && !(ra && rb);
		}

		case Field: {
			Any anyTuple = _operandA->Evaluate(v);
			int index = int(_operandB->Evaluate(v));
			ref<Object> tupleObject = anyTuple.GetContainedObject();

			if(tupleObject && tupleObject.IsCastableTo<Tuple>()) {
				ref<Tuple> tuple = tupleObject;
				try {
					Any val = tuple->Get(index);
					return val;
				}
				catch(...) {
					return Any();
				}
			}
			return Any();
		}
		
		default:
			return Any();
	}
}

std::wstring BinaryExpression::GetOpName() const {
	switch(_type) {
		case Add:
			return L"+";
	
		case Subtract:
			return L"-";

		case Multiply:
			return L"\u00D7";

		case Divide:
			return L"\u00F7";

		case Greater:
			return L">";

		case Smaller:
			return L"<";

		case Equals:
			return L"=";

		case NotEquals:
			return L"\u2260";

		case Modulo:
			return L"%";

		case And:
			return L"&&";

		case Or:
			return L"||";

		case Xor:
			return L"^^";

		case Field:
			return L"->";

		case Max:
			return L"max";

		case Min:
			return L"min";
	}

	return L"";
}

void BinaryExpression::OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> vars) {
	Pixels wl = _operandA ? _operandA->GetWidth() : Expression::KDefaultParameterWidth;
	Pixels wr = _operandB ? _operandB->GetWidth() : Expression::KDefaultParameterWidth;

	if(x < wl) {
		if(_operandA) {
			_operandA->OnMouse(ev, x-4,y, ox+4, oy, parent, this, pg, vars);
		}	
		else {
			if(ev==MouseEventLDown) {
				// menu to choose operand A
				_operandA = Expression::ChooseExpression(ox, oy+16, parent); // TODO make 16 a param
			}
		}
	}
	else if(x > (wl + Expression::KDefaultOperatorWidth) && x< (wl+wr+Expression::KDefaultOperatorWidth)) {
		if(_operandB) {
			_operandB->OnMouse(ev,x-wl-Expression::KDefaultOperatorWidth-4, y, ox + 4 + wl + Expression::KDefaultOperatorWidth, oy, parent, this, pg, vars);
		}
		else {
			if(ev==MouseEventLDown) {
				// menu to choose operand B
				_operandB = Expression::ChooseExpression(ox+wl+Expression::KDefaultOperatorWidth, oy+16, parent); // TODO make height a constant or param
			}
		}
	}
	else {
		// me, myself, I
		if(ev==MouseEventLDown) {
			// show my menu to change op
			ContextMenu cm;
			cm.AddItem(TL(expression_op_add), Add, false, _type==Add);
			cm.AddItem(TL(expression_op_subtract), Subtract, false, _type==Subtract);
			cm.AddItem(TL(expression_op_multiply), Multiply, false, _type==Multiply);
			cm.AddItem(TL(expression_op_divide), Divide, false, _type==Divide);
			cm.AddItem(TL(expression_op_modulo), Modulo, false, _type==Modulo);
			cm.AddItem(TL(expression_op_greater), Greater, false, _type==Greater);
			cm.AddItem(TL(expression_op_smaller), Smaller, false, _type==Smaller);
			cm.AddItem(TL(expression_op_equals), Equals, false, _type==Equals);
			cm.AddItem(TL(expression_op_not_equals), NotEquals, false, _type==NotEquals);
			cm.AddItem(TL(expression_op_or), Or, false, _type==Or);
			cm.AddItem(TL(expression_op_and), And, false, _type==And);
			cm.AddItem(TL(expression_op_xor), Xor, false, _type==Xor);
			cm.AddItem(TL(expression_op_max), Max, false, _type==Max);
			cm.AddItem(TL(expression_op_min), Min, false, _type==Min);
			cm.AddItem(TL(expression_op_field), Field, false, _type==Field);

			cm.AddSeparator();
			cm.AddItem(TL(expression_remove_term), 1337,false,false);
			
			Pixels lw = _operandA ? _operandA->GetWidth() : Expression::KDefaultParameterWidth;
			int r = cm.DoContextMenu(parent, ox+lw, oy+16); // TODO make height a constant or parameter somewhere
			if(r>0 && r<_BinaryLast) {
				_type = (Binary)r;
			}
			else if(r==1337) {
				if(pc) pc->RemoveChild(this);
			}
		}
	}

	if(ev==MouseEventLDown || ev==MouseEventLUp) {
		parent->Layout();
		parent->Update();
	}
}

void BinaryExpression::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "type", "binary");
	SaveAttributeSmall(parent, "op", (int)_type);

	if(_operandA) {
		TiXmlElement operand("operand");
		_operandA->Save(&operand);
		parent->InsertEndChild(operand);
	}

	if(_operandB) {
		TiXmlElement operand("operand");
		_operandB->Save(&operand);
		parent->InsertEndChild(operand);
	}
}

void BinaryExpression::Load(TiXmlElement* you) {
	_type = (Binary)LoadAttributeSmall(you, "op", (int)_type);
	TiXmlElement* operand = you->FirstChildElement("operand");
	if(operand!=0) {
		std::wstring type = LoadAttributeSmall<std::wstring>(operand, "type", L"");
		_operandA = Expression::CreateExpressionByType(type);
		_operandA->Load(operand);
		operand = operand->NextSiblingElement("operand");
		if(operand!=0) {
			type = LoadAttributeSmall<std::wstring>(operand, "type", L"");
			_operandB = Expression::CreateExpressionByType(type);
			_operandB->Load(operand);
		}
	}
}

std::wstring BinaryExpression::ToString(strong<Variables> vars) const {
	std::wostringstream wos;
	wos << L'(';
	if(_operandA) {
		wos << _operandA->ToString(vars);
	}
	else {
		wos << L"?";
	}
	wos << L' ' << GetOpName() << L' ';

	if(_operandB) {
		wos << _operandB->ToString(vars);
	}
	else {
		wos << L"?";
	}
	wos << L')';

	return wos.str();
}

bool BinaryExpression::IsComplete(strong<Variables> v) const {
	return _operandA && _operandB && _operandA->IsComplete(v) && _operandB->IsComplete(v);
}

/* VariableExpression */
VariableExpression::VariableExpression(const std::wstring& id): _id(id) {
}

VariableExpression::~VariableExpression() {
}

Pixels VariableExpression::GetWidth() {
	return Expression::KDefaultParameterWidth*3;
}

bool VariableExpression::IsConstant() const {
	return false;
}

void VariableExpression::Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v) {
	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);

	SolidBrush tbr(theme->GetColor(Theme::ColorHighlightStart));
	std::wstring value = ToString(v);
	g.DrawString(value.c_str(), (int)value.length(), theme->GetGUIFont(), (SimpleRectangle<float>)a, &sf, &tbr);
}

Any VariableExpression::Evaluate(strong<Variables> v) {
	ref<Variable> var = v->GetVariableById(_id);
	if(var) {
		return var->GetValue();
	}

	return Any();
}

void VariableExpression::OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> vars) {
	if(ev==MouseEventLDown) {
		ContextMenu cm;
		enum { KCRemove = 1, KCNoVariable = 2, KCVariableProviderStart = 3 };
		cm.AddItem(TL(expression_remove_term), KCRemove, false, false);
		cm.AddSeparator(TL(expression_choose_variable));
		cm.AddItem(TL(expression_no_variable), KCNoVariable, false, _id==L"");

		for(int a=0;a<int(vars->GetVariableCount());a++) {
			ref<Variable> var = vars->GetVariableByIndex(a);
			if(var) {
				cm.AddItem(var->GetName(), a+KCVariableProviderStart, false, _id==var->GetID());
			}
		}

		int r = cm.DoContextMenu(parent,ox,oy+16);
		if(r==KCRemove) {
			if(pc) {
				pc->RemoveChild(this);
			}
		}
		else if(r==KCNoVariable) {
			_id = L"";
		}
		else if(r>=KCVariableProviderStart) {
			int idx = r-KCVariableProviderStart;
			ref<Variable> var = vars->GetVariableByIndex(idx);
			if(var) {
				_id = var->GetID();
			}
			else {
				_id = L"";
			}
		}

		parent->Layout();
		parent->Update();
	}
}

void VariableExpression::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "id", _id);	
	SaveAttributeSmall(parent, "type", "var");
}

void VariableExpression::Load(TiXmlElement* you) {
	_id = LoadAttributeSmall<std::wstring>(you, "id", _id);
}

std::wstring VariableExpression::ToString(strong<Variables> vars) const {
	ref<Variable> var = vars->GetVariableById(_id);
	if(var) {
		std::wstring varName = var->GetName();
		if(varName.find(L' ')!=std::wstring::npos) {
			return L'<' + varName + L'>';
		}
		else {
			return varName;
		}
	}
	return L"?";
}

bool VariableExpression::IsComplete(strong<Variables> vars) const {
	return vars->GetVariableById(_id);
}

void VariableExpression::RemoveChild(ref<Expression> c) {
}

/* ConstExpression */
ConstExpression::ConstExpression(const Any& value): _value(value.ToString()), _desiredType(value.GetType()) {
}

ConstExpression::~ConstExpression() {
}

void ConstExpression::RemoveChild(ref<Expression> c) {
}

bool ConstExpression::IsConstant() const {
	return true;
}

ref<PropertySet> ConstExpression::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(Properties::CreateTypeProperty(TL(expression_constant_type), this, &_desiredType));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(expression_constant), this, &_value, _value)));
	return ps;
}

Pixels ConstExpression::GetWidth() {
	return Expression::KDefaultParameterWidth*3;
}

void ConstExpression::Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v) {
	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);

	SolidBrush tbr(theme->GetColor(Theme::ColorHighlightStart));
	g.DrawString(_value.c_str(), (int)_value.length(), theme->GetGUIFont(), (SimpleRectangle<float>)a, &sf, &tbr);
}

Any ConstExpression::Evaluate(strong<Variables> v) {
	return Any(_desiredType, _value);
}

void ConstExpression::OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v) {
	if(ev==MouseEventLDown) {
		ContextMenu cm;
		enum { KCRemove = 1, KCValue };
		cm.AddItem(TL(expression_change_constant), KCValue, true, false);
		cm.AddItem(TL(expression_remove_term), KCRemove, false, false);

		int r = cm.DoContextMenu(parent,ox,oy+16);
		if(r==KCRemove) {
			if(pc) {
				pc->RemoveChild(this);
			}
		}
		else if(r==KCValue) {
			pg->Inspect(this);
		}

		parent->Layout();
		parent->Update();
	}
}

bool ConstExpression::IsComplete(strong<Variables> v) const {
	return true;
}

void ConstExpression::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "type", "const");
	SaveAttributeSmall(parent, "value", _value);
	SaveAttributeSmall(parent, "value-type", (int)_desiredType);
}

void ConstExpression::Load(TiXmlElement* you) {
	_value = LoadAttributeSmall<std::wstring>(you, "value", _value);
	_desiredType = (Any::Type)LoadAttributeSmall<int>(you, "value-type", (int)Any::TypeInteger);
}

std::wstring ConstExpression::ToString(strong<Variables> v) const {
	return _value;
}

/* NullaryExpression */
Enumeration<NullaryExpression::Nullary> NullaryExpression::NullaryEnumeration;

void Enumeration<NullaryExpression::Nullary>::InitializeMapping() {
	Add(NullaryExpression::NullaryNull,			L"null",		L"nullary_null");
	Add(NullaryExpression::NullaryHours,			L"hours",		L"nullary_hours");
	Add(NullaryExpression::NullaryMinutes,		L"minutes",		L"nullary_minutes");
	Add(NullaryExpression::NullarySeconds,		L"seconds",		L"nullary_seconds");
	Add(NullaryExpression::NullaryDayOfMonth,	L"dayofmonth",	L"nullary_day_of_month");
	Add(NullaryExpression::NullaryDayOfWeek,		L"dayofweek",	L"nullary_day_of_week");
	Add(NullaryExpression::NullaryNameOfMonth,	L"nameofmonth",	L"nullary_name_of_month");
	Add(NullaryExpression::NullaryNameOfDay,		L"nameofday",	L"nullary_name_of_day");
	Add(NullaryExpression::NullaryMonth,			L"month",		L"nullary_month");
	Add(NullaryExpression::NullaryYear,			L"year",		L"nullary_year");
	Add(NullaryExpression::NullaryRandom,		L"random",		L"nullary_random");
	Add(NullaryExpression::NullaryPi,			L"pi",			L"nullary_pi");
	Add(NullaryExpression::NullaryE,				L"e",			L"nullary_euler");
}

NullaryExpression::NullaryExpression(Nullary n): _func(n) {
}

NullaryExpression::~NullaryExpression() {
}

void NullaryExpression::RemoveChild(ref<Expression> c) {
}

bool NullaryExpression::IsConstant() const {
	if(_func==NullaryNull || _func==NullaryPi || _func==NullaryE) {
		return true;
	}
	return false;
}

ref<PropertySet> NullaryExpression::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(NullaryEnumeration.CreateSelectionProperty(TL(nullary_function), this, &_func));
	return ps;
}

Pixels NullaryExpression::GetWidth() {
	return Expression::KDefaultParameterWidth*3;
}

void NullaryExpression::Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> v) {
	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);

	SolidBrush tbr(theme->GetColor(Theme::ColorHighlightStart));
	std::wstring value = NullaryEnumeration.GetFriendlyName(_func);
	g.DrawString(value.c_str(), (int)value.length(), theme->GetGUIFont(), (SimpleRectangle<float>)a, &sf, &tbr);
}

Any NullaryExpression::Evaluate(strong<Variables> v) {
	if(_func==NullaryRandom) {
		return Any(double(Util::RandomFloat()));
	}
	else {
		// All date/time functions
		Date date;

		switch(_func) {
			case NullaryHours:
				return Any((int)date.GetHours());

			case NullarySeconds:
				return Any((int)date.GetSeconds());

			case NullaryMinutes:
				return Any((int)date.GetMinutes());

			case NullaryDayOfMonth:
				return Any((int)date.GetDayOfMonth());

			case NullaryDayOfWeek:
				return Any((int)date.GetDayOfWeek());

			case NullaryMonth:
				return Any((int)date.GetMonth());

			case NullaryYear:
				return Any((int)date.GetYear());

			case NullaryNameOfDay:
				return Date::GetFriendlyDayName(date.GetDayOfWeek());

			case NullaryNameOfMonth:
				return Date::GetFriendlyMonthName(date.GetMonth());

			case NullaryPi:
				return Any(3.14159);

			case NullaryE:
				return Any(2.718281828);
		}
	}

	return Any();
}

void NullaryExpression::OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> v) {
	if(ev==MouseEventLDown) {
		ContextMenu cm;
		enum { KCRemove = 1, KCValue };
		cm.AddItem(TL(expression_change_function), KCValue, true, false);
		cm.AddItem(TL(expression_remove_term), KCRemove, false, false);

		int r = cm.DoContextMenu(parent,ox,oy+16);
		if(r==KCRemove) {
			if(pc) {
				pc->RemoveChild(this);
			}
		}
		else if(r==KCValue) {
			pg->Inspect(this);
		}

		parent->Layout();
		parent->Update();
	}
}

bool NullaryExpression::IsComplete(strong<Variables> v) const {
	return true;
}

void NullaryExpression::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "type", "nullary");
	SaveAttributeSmall(parent, "function", NullaryEnumeration.Serialize(_func));
}

void NullaryExpression::Load(TiXmlElement* you) {
	_func = NullaryEnumeration.Unserialize(LoadAttributeSmall<std::wstring>(you, "function", L""), NullaryNull);
}

std::wstring NullaryExpression::ToString(strong<Variables> v) const {
	return L"[" + NullaryEnumeration.Serialize(_func) + L"]";
}

/* UnaryExpression */
UnaryExpression::UnaryExpression() {
	_type = Negate;
}

UnaryExpression::~UnaryExpression() {
}

Pixels UnaryExpression::GetWidth() {
	return Expression::KDefaultOperatorWidth + (_operand ? _operand->GetWidth() : (Expression::KDefaultParameterWidth)) + 4 + 4; // +4 for end parenthesis
}

void UnaryExpression::RemoveChild(ref<Expression> c) {
	if(_operand==c) _operand = 0;
}

void UnaryExpression::Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> vars) {
	Area op = a;
	op.SetWidth(Expression::KDefaultOperatorWidth);
	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);
	SolidBrush tbr(theme->GetColor(Theme::ColorText));

	std::wstring text = GetOpName();
	LinearGradientBrush opb(PointF(0.0f, float(a.GetTop())), PointF(0.0f, float(a.GetBottom())), theme->GetColor(Theme::ColorActiveStart), theme->GetColor(Theme::ColorActiveEnd));
	g.FillRectangle(&opb, op);
	SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
	g.FillRectangle(&disabled,op);
	g.DrawString(text.c_str(), (int)text.length(), theme->GetGUIFontBold(), op, &sf, &tbr);

	Area param = a;
	param.Narrow(Expression::KDefaultOperatorWidth + 4,0,4,0);
	
	if(_operand) {
		_operand->Paint(g,param,theme,vars);
	}	

	DrawParentheses(g,param,theme);
}

std::wstring UnaryExpression::GetOpName() const {
	switch(_type) {
		case Negate:
			return L"¬";

		case NegateNumber:
			return L"-";

		case Abs:
			return L"abs";

		case Sin:
			return L"sin";

		case Cos:
			return L"cos";

		case Tan:
			return L"tan";
		
		case Acos:
			return L"acos";

		case Atan:
			return L"atan";

		case Asin:
			return L"asin";

		case Sqrt:
			return L"\u221A";

		case Log:
			return L"log";
	}

	return L"";
}

bool UnaryExpression::IsConstant() const {
	return !_operand || _operand->IsConstant();
}

ref<Expression> UnaryExpression::Fold(strong<Variables> vars) {
	if(_operand) {
		_operand = _operand->Fold(vars);
	}

	if(IsConstant()) {
		return GC::Hold(new ConstExpression(Evaluate(vars)));
	}
	return this;
}

void UnaryExpression::SetType(Unary u) {
	_type = u;
}

void UnaryExpression::SetOperand(ref<Expression> c) {
	_operand = c;
}

Any UnaryExpression::Evaluate(strong<Variables> v) {
	if(!_operand) {
		Throw(L"Could not evaluate condition; unary condition has no operand", ExceptionTypeError);
	}

	switch(_type) {
		case Negate:
			return !_operand->Evaluate(v);

		case NegateNumber:
			return -_operand->Evaluate(v);

		case Abs:
			return _operand->Evaluate(v).Abs();

		case Sin:
			return Any(sin(double(_operand->Evaluate(v))));

		case Cos:
			return Any(cos(double(_operand->Evaluate(v))));

		case Tan:
			return Any(tan(double(_operand->Evaluate(v))));
		
		case Acos:
			return Any(acos(double(_operand->Evaluate(v))));

		case Atan:
			return Any(atan(double(_operand->Evaluate(v))));

		case Asin:
			return Any(asin(double(_operand->Evaluate(v))));

		case Sqrt:
			return Any(sqrt(double(_operand->Evaluate(v))));

		case Log:
			return Any(log(double(_operand->Evaluate(v))));

		default:
			return _operand->Evaluate(v);
	}
}

void UnaryExpression::OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels ox, Pixels oy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> vars) {
	if(x<Expression::KDefaultOperatorWidth) {
		// meant for me
		if(ev==MouseEventLDown) {
			// show my menu to change op
			ContextMenu cm;
			cm.AddItem(TL(expression_op_negate), Negate, false, _type==Negate);
			cm.AddItem(TL(expression_op_negate_number), NegateNumber, false, _type==NegateNumber);
			cm.AddItem(TL(expression_op_abs), Abs, false, _type==Abs);

			cm.AddItem(TL(expression_op_sin), Sin, false, _type==Sin);
			cm.AddItem(TL(expression_op_cos), Cos, false, _type==Cos);
			cm.AddItem(TL(expression_op_tan), Tan, false, _type==Tan);
			cm.AddItem(TL(expression_op_asin), Asin, false, _type==Asin);
			cm.AddItem(TL(expression_op_acos), Acos, false, _type==Acos);
			cm.AddItem(TL(expression_op_atan), Atan, false, _type==Atan);
			cm.AddItem(TL(expression_op_sqrt), Sqrt, false, _type==Sqrt);
			cm.AddItem(TL(expression_op_log), Log, false, _type==Log);
			
			cm.AddSeparator();
			cm.AddItem(TL(expression_remove_term), 1337, false, false);
			
			int r = cm.DoContextMenu(parent, ox, oy+16); // TODO make height a constant or parameter somewhere
			if(r>0 && r<_UnaryLast) {
				_type = (Unary)r;
			}
			else if(r==1337) {
				if(pc) pc->RemoveChild(this);
			}
		}
	}
	else {
		// meant for operand
		if(_operand) {
			_operand->OnMouse(ev, x-Expression::KDefaultOperatorWidth-4, y, ox+Expression::KDefaultOperatorWidth+4,oy, parent,this, pg, vars);
		}
		else {
			if(ev==MouseEventLDown) {
				_operand = Expression::ChooseExpression(ox+Expression::KDefaultOperatorWidth, oy+16, parent);
			}
		}
	}

	if(ev==MouseEventLDown || ev==MouseEventLUp) {
		parent->Layout();
		parent->Update();
	}
}

void UnaryExpression::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "type", "unary");
	SaveAttributeSmall(parent, "op", (int)_type);

	if(_operand) {
		TiXmlElement operand("operand");
		_operand->Save(&operand);
		parent->InsertEndChild(operand);
	}
}

void UnaryExpression::Load(TiXmlElement* you) {
	_type = (Unary)LoadAttributeSmall<int>(you, "op", (int)_type);

	TiXmlElement* operand = you->FirstChildElement("operand");
	if(operand!=0) {
		std::wstring type = LoadAttributeSmall<std::wstring>(operand, "type", L"");
		_operand = Expression::CreateExpressionByType(type);
		if(_operand) {
			_operand->Load(operand);
		}
	}
}
std::wstring UnaryExpression::ToString(strong<Variables> vars) const {
	std::wstring op = GetOpName();

	if(_operand) {
		return op + L"("+ _operand->ToString(vars) + L")";	
	}

	return L"";
}

bool UnaryExpression::IsComplete(strong<Variables> vars) const {
	return _operand && _operand->IsComplete(vars);
}

/* NullExpression */
NullExpression::NullExpression() {
}

NullExpression::~NullExpression() {
}

bool NullExpression::HasChild() const {
	return _operand;
}

void NullExpression::RemoveChild(ref<Expression> c) {
	if(_operand==c) _operand = 0;
}

Pixels NullExpression::GetWidth() {
	return (_operand ? (_operand->GetWidth()): Expression::KDefaultOperatorWidth) + 4; //+4 for end parenthesis
}

void NullExpression::Paint(tj::shared::graphics::Graphics& g, const Area& a, ref<Theme> theme, strong<Variables> vars) {
	if(_operand) {
		Area operand = a;
		operand.SetWidth(_operand->GetWidth());
		_operand->Paint(g,operand,theme, vars);
	}
}

Any NullExpression::Evaluate(strong<Variables> v) {
	if(_operand) {
		return _operand->Evaluate(v);
	}
	return Any();
}

bool NullExpression::IsConstant() const {
	return !_operand || _operand->IsConstant();
}

void NullExpression::OnMouse(MouseEvent ev, Pixels x, Pixels y, Pixels offx, Pixels offy, ref<Wnd> parent, ref<Expression> pc, ref<PropertyGridWnd> pg, strong<Variables> vars) {
	if(_operand) {
		_operand->OnMouse(ev,x,y,offx,offy, parent,this, pg, vars);
	}
	else {
		if(ev==MouseEventLDown) {
			// my own menu to choose a new operand
			_operand = Expression::ChooseExpression(offx,offy+16,parent);
		}
	}
}

bool NullExpression::IsComplete(strong<Variables> vars) const {
	return _operand && _operand->IsComplete(vars);
}

void NullExpression::Save(TiXmlElement* parent) {
	if(_operand) {
		TiXmlElement operand("operand");
		_operand->Save(&operand);
		parent->InsertEndChild(operand);
	}
}

void NullExpression::Load(TiXmlElement* you) {
	TiXmlElement* operand = you->FirstChildElement("operand");
	if(operand!=0) {
		std::wstring type = LoadAttributeSmall<std::wstring>(operand, "type", L"");
		_operand = Expression::CreateExpressionByType(type);
		if(_operand) {
			_operand->Load(operand);
		}
	}
}

std::wstring NullExpression::ToString(strong<Variables> vars) const {
	if(_operand) {
		return _operand->ToString(vars);
	}
	return L"";
}

ref<Expression> Expression::CreateExpressionByType(const std::wstring& type) {
	if(type==L"const") {
		return GC::Hold(new ConstExpression());
	}
	else if(type==L"nullary") {
		return GC::Hold(new NullaryExpression());
	}
	else if(type==L"unary") {
		return GC::Hold(new UnaryExpression());
	}
	else if(type==L"binary") {
		return GC::Hold(new BinaryExpression());
	}
	else if(type==L"var") {
		return GC::Hold(new VariableExpression());
	}
	else {
		return 0;
	}
}

ref<Expression> Expression::ChooseExpression(Pixels x, Pixels y, ref<Wnd> parent) {
	ContextMenu cm;
	enum { KCConstant = 1, KCVariable, KCNullary, KCUnaryStart, KCNegate, KCAbs, KCNegateNumber, KCBinaryStart, KCAdd, KCField, KCSubtract, KCMultiply, KCDivide, KCModulo, KCGreater, KCSmaller, KCEquals, KCNotEquals, KCOr, KCAnd, KCXor, };

	cm.AddItem(TL(expression_variable), KCVariable);
	cm.AddItem(TL(expression_constant), KCConstant);
	cm.AddItem(TL(expression_nullary), KCNullary);
	cm.AddItem(TL(expression_field), KCField);

	// Binary operators
	strong<SubMenuItem> binaries = GC::Hold(new SubMenuItem(TL(expression_arithmetic), false, MenuItem::NotChecked, null));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_add), KCAdd)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_subtract), KCSubtract)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_multiply), KCMultiply)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_divide), KCDivide)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_modulo), KCModulo)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_greater), KCGreater)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_smaller), KCSmaller)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_equals), KCEquals)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_not_equals), KCNotEquals)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_abs), KCAbs)));
	binaries->AddItem(GC::Hold(new MenuItem(TL(expression_op_negate_number), KCNegateNumber)));
	cm.AddItem(binaries);

	strong<SubMenuItem> logics = GC::Hold(new SubMenuItem(TL(expression_logic), false, MenuItem::NotChecked, null));
	logics->AddItem(GC::Hold(new MenuItem(TL(expression_op_or), KCOr)));
	logics->AddItem(GC::Hold(new MenuItem(TL(expression_op_and), KCAnd)));
	logics->AddItem(GC::Hold(new MenuItem(TL(expression_op_xor), KCXor)));
	logics->AddItem(GC::Hold(new MenuItem(TL(expression_op_negate), KCNegate)));
	cm.AddItem(logics);

	int r = cm.DoContextMenu(parent, x, y);
	ref<Expression> cond = 0;

	if(r==KCConstant) {
		cond = GC::Hold(new ConstExpression(Any(int(0))));
	}
	else if(r==KCVariable) {
		cond = GC::Hold(new VariableExpression());
	}
	else if(r==KCNullary) {
		cond = GC::Hold(new NullaryExpression());
	}
	else if(r>KCUnaryStart && r < KCBinaryStart) {
		ref<UnaryExpression> uc = GC::Hold(new UnaryExpression());
		switch(r) {
			case KCNegate:
				uc->SetType(UnaryExpression::Negate);
				break;
			
			case KCNegateNumber:
				uc->SetType(UnaryExpression::NegateNumber);
				break;

			case KCAbs:
				uc->SetType(UnaryExpression::Abs);
				break;
		}
		cond = uc;
	}
	else if(r>KCBinaryStart) {
		ref<BinaryExpression> bc = GC::Hold(new BinaryExpression());
		switch(r) {
			case KCField:
				bc->SetType(BinaryExpression::Field);
				break;

			case KCAdd:
				bc->SetType(BinaryExpression::Add);
				break;

			case KCSubtract:
				bc->SetType(BinaryExpression::Subtract);
				break;

			case KCMultiply:
				bc->SetType(BinaryExpression::Multiply);
				break;
			case KCDivide:
				bc->SetType(BinaryExpression::Divide);
				break;
			case KCModulo:
				bc->SetType(BinaryExpression::Modulo);
				break;
			case KCGreater:
				bc->SetType(BinaryExpression::Greater);
				break;
			case KCSmaller:
				bc->SetType(BinaryExpression::Smaller);
				break;

			case KCEquals:
				bc->SetType(BinaryExpression::Equals);
				break;

			case KCNotEquals:
				bc->SetType(BinaryExpression::NotEquals);
				break;

			case KCOr:
				bc->SetType(BinaryExpression::Or);
				break;

			case KCAnd:
				bc->SetType(BinaryExpression::And);
				break;

			case KCXor:
				bc->SetType(BinaryExpression::Xor);
				break;
		}
		cond = bc;
	}

	return cond;
}

/* ExpressionPopupWnd */
class ExpressionPopupWnd: public PopupWnd {
	public:
		ExpressionPopupWnd(ref<Expression> root, strong<Variables> vars);
		virtual ~ExpressionPopupWnd();
		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
		virtual void Update();
		virtual void OnActivate(bool ac);
		virtual void OnSize(const Area& ns);
		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
		virtual void Layout();

		virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp) {
			if(msg==WM_MOUSEACTIVATE) {
				return MA_ACTIVATE;
			}
			return PopupWnd::Message(msg,wp,lp);
		}

	protected:
		const static Pixels KMargin = 10;
		Pixels _lastX;
		strong<Variables> _variables;
		ref<PropertyGridWnd> _pg;
		ref<Expression> _root;
};

/* ExpressionPropertyWnd */
class ExpressionPropertyWnd: public ChildWnd {
	public:
		ExpressionPropertyWnd(ref<Expression> c, strong<Variables> vars): ChildWnd(L""), _variables(vars), _conditionIcon(L"icons/condition.png") {
			assert(c);
			_condition = c;
			SetWantMouseLeave(true);
		}

		virtual ~ExpressionPropertyWnd() {
		}

		virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp) {
			if(msg==WM_PARENTNOTIFY) {
				Repaint();
			}

			return ChildWnd::Message(msg,wp,lp);
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			Area rc = GetClientArea();
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			g.FillRectangle(&back, rc);

			if(IsMouseOver()) {
				theme->DrawToolbarBackground(g, 0.0f, 0.0f, float(rc.GetWidth()), float(rc.GetHeight()));
			}

			_conditionIcon.Paint(g, Area(0,0,16,16));
			std::wstring info = _condition->ToString(_variables);
			SolidBrush tbr(_condition->IsComplete(_variables)?theme->GetColor(Theme::ColorActiveStart):theme->GetColor(Theme::ColorHighlightStart));
			g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFont(), PointF(20.0f, 2.0f), &tbr);
		}

	protected:	
		void DoPopup() {
			if(!_popup) {
				_popup = GC::Hold(new ExpressionPopupWnd(_condition, _variables));
			}

			Area rc = GetClientArea();
			_popup->SetSize(300,140);
			_popup->PopupAt(rc.GetLeft(), rc.GetBottom(), this);
			ModalLoop ml;
			ml.Enter(_popup->GetWindow(), false);
			_popup->Show(false);
		}

		void DoManualPopup() {
			class ManualPopupData: public Inspectable {
				public:
					ManualPopupData(const std::wstring& expr): _expr(expr) {}
					virtual ~ManualPopupData() {}
					virtual ref<PropertySet> GetProperties() {
						ref<PropertySet> ps = GC::Hold(new PropertySet());
						ps->Add(GC::Hold(new TextProperty(TL(expression_expression), this, &_expr, 200)));
						return ps;
					}

					std::wstring _expr;
			};

			ref<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(TL(expression), TL(expression_expression_question)));
			ref<ManualPopupData> data = GC::Hold(new ManualPopupData(_condition->ToString(_variables)));
			pdw->GetPropertyGrid()->Inspect(data);
			while(true) {
				if(pdw->DoModal(this)) {
					try {
						// Parse the expression and set it
						_condition->Parse(data->_expr, _variables);
					}
					catch(const Exception& e) {
						Alert::Show(TL(error), e.ToString(), Alert::TypeError);
						continue;
					}
					break;
				}
				else {
					break;
				}
			}
			Repaint();
		}

		virtual void OnSize(const Area& ns) {
			Repaint();
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			if(ev==MouseEventMove||ev==MouseEventLeave) {
				Repaint();
			}
			else if(ev==MouseEventLUp) {
				DoPopup();
				Repaint();
			}
			else if(ev==MouseEventRUp) {
				DoManualPopup();
				Repaint();
			}
		}

		Icon _conditionIcon;
		strong<Variables> _variables;
		ref<Expression> _condition;
		ref<ExpressionPopupWnd> _popup;
};

/* ExpressionProperty */
ExpressionProperty::ExpressionProperty(const std::wstring& name, strong<Expression> c, strong<Variables> vars): Property(name,false), _condition(c), _variables(vars) {
}

ExpressionProperty::~ExpressionProperty() {
}

ref<Wnd> ExpressionProperty::GetWindow() {
	if(!_wnd) {
		_wnd = GC::Hold(new ExpressionPropertyWnd(_condition, _variables));
	}
	return _wnd;
}

void ExpressionProperty::Update() {
	if(_wnd) {
		_wnd->Update();
	}
}

/* ExpressionPopupWnd */
ExpressionPopupWnd::ExpressionPopupWnd(ref<Expression> root, strong<Variables> vars): _root(root), _variables(vars), _lastX(26) {
	SetHorizontallyScrollable(true);
	_pg = GC::Hold(new PropertyGridWnd(false));
	Add(_pg);
}

void ExpressionPopupWnd::Update() {
	Repaint();
}

ExpressionPopupWnd::~ExpressionPopupWnd() {
}

void ExpressionPopupWnd::Layout() {
	Area rc = GetClientArea();
	SetHorizontalScrollInfo(Range<int>(0,_root->GetWidth() + KMargin*2), rc.GetWidth()); 
	rc.Narrow(KMargin,3*KMargin+16,KMargin,KMargin);
	_pg->Fill(LayoutFill,rc);
}

void ExpressionPopupWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back, rc);

	Area trc = rc;
	trc.SetHeight(36);
	LinearGradientBrush tbbr(PointF(0.0f, (float)trc.GetTop()), PointF(0.0f, (float)trc.GetBottom()), Theme::ChangeAlpha(theme->GetColor(Theme::ColorActiveStart), 0), Theme::ChangeAlpha(theme->GetColor(Theme::ColorActiveEnd),100));
	g.FillRectangle(&tbbr, trc);

	Area arc = rc;
	arc.Narrow(KMargin,KMargin,KMargin, KMargin);
	arc.SetWidth(_root->GetWidth());
	arc.SetHeight(16);
	arc.SetX(arc.GetX()-GetHorizontalPos());
	_root->Paint(g,arc,theme, _variables);

	// Draw arrow
	if(_lastX>=0) {
		Pixels lineH = 36;
		PointF tri[3];
		tri[0] = PointF((float)_lastX-6.0f, (float)lineH);
		tri[1] = PointF((float)_lastX, lineH-8.0f);
		tri[2] = PointF((float)_lastX+6.0f, (float)lineH);
		g.FillPolygon(&back, tri, 3, FillModeWinding);
	}

	// Draw border
	rc.Narrow(0,0,1,1);
	SolidBrush border(theme->GetColor(Theme::ColorActiveStart));
	Pen pn(&border, 1.0f);
	g.DrawRectangle(&pn, rc);
}

void ExpressionPopupWnd::OnActivate(bool ac) {
	Layout();
	Update();
	PopupWnd::OnActivate(ac);
}

void ExpressionPopupWnd::OnSize(const Area& ns) {
	Layout();
	Update();
	PopupWnd::OnSize(ns);
}

void ExpressionPopupWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(y<KMargin+16) {
		if(ev==MouseEventLDown) {
			_pg->Inspect(0);
			//_lastX = -1;
			Repaint();
		}
		_root->OnMouse(ev, x-KMargin+GetHorizontalPos(), y-KMargin, KMargin-GetHorizontalPos(), KMargin,this,0, _pg, _variables);

		if(ev==MouseEventLDown) {
			//_lastX = x;
		}
		Update();
	}

	if(ev==MouseEventLUp) {
		Layout();
		Repaint();
	}
}

/* Assignment */
Assignment::Assignment() {
	_value = GC::Hold(new NullExpression());
}
Assignment::~Assignment() {
}

void Assignment::SetVariableId(const std::wstring& id) {
	_id = id;
}

ref<Expression> Assignment::GetExpression() {
	return _value;
}

const std::wstring& Assignment::GetVariableId() const {
	return _id;
}

void Assignment::Load(TiXmlElement* t) {
	_id = LoadAttributeSmall(t, "id", _id);

	TiXmlElement* value = t->FirstChildElement("value");
	if(value!=0) {
		_value->Load(value);
	}
}

void Assignment::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "id", _id);
	TiXmlElement condition("value");
	_value->Save(&condition);
	parent->InsertEndChild(condition);
}

/* Assignments */
Assignments::Assignments() {
}

Assignments::~Assignments() {
}

void Assignments::Add(ref<Assignment> as) {
	if(as) _assignments.push_back(as);
}

bool Assignments::IsEmpty() const {
	return _assignments.size()==0;
}

void Assignments::Save(TiXmlElement* t) {
	std::vector< ref<Assignment> >::iterator it = _assignments.begin();
	while(it!=_assignments.end()) {
		ref<Assignment> as = *it;
		TiXmlElement assign("assign");
		as->Save(&assign);
		t->InsertEndChild(assign);
		++it;
	}
}

void Assignments::Load(TiXmlElement* t) {
	TiXmlElement* assign = t->FirstChildElement("assign");

	while(assign!=0) {
		ref<Assignment> as = GC::Hold(new Assignment());
		as->Load(assign);
		Add(as);
		assign = assign->NextSiblingElement("assign");
	}
}

void Assignments::MoveUp(ref<Assignment> c) {
	std::vector< ref<Assignment> >::iterator it = std::find(_assignments.begin(), _assignments.end(), c);
	if(it!=_assignments.end() && it!=_assignments.begin()) {
		ref<Assignment> a = *it;
		*it = *(it-1);
		*(it-1) = a;
	}
}

void Assignments::MoveDown(ref<Assignment> c) {
	std::vector< ref<Assignment> >::iterator it = std::find(_assignments.begin(), _assignments.end(), c);
	if(it!=_assignments.end() && (it+1)!=_assignments.end()) {
		ref<Assignment> a = *it;
		*it = *(it+1);
		*(it+1) = a;
	}
}

ref<Assignment> Assignments::GetAssignmentByIndex(int id) {
	if(_assignments.size()>((unsigned int)id)) {
		return _assignments.at(id);
	}
	return 0;
}

void Assignments::Remove(ref<Assignment> a) {
	std::vector< ref<Assignment> >::iterator it = _assignments.begin();
	while(it!=_assignments.end()) {
		ref<Assignment> as = *it;
		if(as==a) {
			it = _assignments.erase(it);
		}
		else {
			++it;
		}
	}
}

int Assignments::GetAssignmentCount() const {
	return (int)_assignments.size();
}

/* AssignmentsListWnd */
class AssignmentsListWnd: public ListWnd {
	public:
		AssignmentsListWnd(ref<Assignments> as, ref<PropertyGridWnd> pg, strong<Variables> vars): _as(as), _pg(pg), _variables(vars) {
			AddColumn(TL(variable_name), KCVariable, 0.2f);
			AddColumn(TL(variable_value), KCValue, 0.8f);
		}

		virtual ~AssignmentsListWnd() {
		}

		virtual int GetItemCount() {
			return _as->GetAssignmentCount();
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			if(ev==MouseEventLDown) {
				_pg->Inspect(0);
			}
			ListWnd::OnMouse(ev,x,y);
		}

		virtual void OnClickItem(int id, int c, Pixels x, Pixels y) {
			SetSelectedRow(id);
			if(c==KCVariable) {
				ref<Assignment> as = _as->GetAssignmentByIndex(id);
				if(as) {
					Area row = GetRowArea(id);
					Pixels ox = Pixels(GetColumnX(c)*row.GetWidth());
					Pixels oy = row.GetBottom();

					// Choose variable menu
					ref<Variable> chosen = _variables->DoChoosePopup(ox,oy,this);
					if(chosen) {
						as->SetVariableId(chosen->GetID());
					}
				}
				Repaint();
			}
			else if(c==KCValue) {
				ref<Assignment> as = _as->GetAssignmentByIndex(id);
				if(as) {
					ref<Expression> condition = as->GetExpression();
					if(condition) {
						Area row = GetRowArea(id);
						Pixels ox = Pixels(GetColumnX(c)*row.GetWidth());
						Pixels oy = row.GetTop();
						condition->OnMouse(MouseEventLDown, x, y, ox, oy, this, 0, _pg, _variables); // TODO provide propertygrid
						condition->OnMouse(MouseEventLUp, x, y, ox, oy, this, 0, _pg, _variables); // TODO provide propertygrid
						Repaint();
					}
				}
			}
		}

		virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
			ref<Assignment> as = _as->GetAssignmentByIndex(id);
			if(as) {
				strong<Theme> theme = ThemeManager::GetTheme();
				StringFormat sf;
				sf.SetAlignment(StringAlignmentFar);
				SolidBrush tbr(theme->GetColor(Theme::ColorText));

				ref<Variable> var = _variables->GetVariableById(as->GetVariableId());
				if(var) {
					DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KCVariable, row, var->GetName()+L" =");
				}

				ref<Expression> value = as->GetExpression();
				if(value) {
					Area cr = row;
					cr.SetX(Pixels(cr.GetWidth()*GetColumnX(KCValue)));
					cr.SetWidth(value->GetWidth());
					SolidBrush cbr(theme->GetColor(Theme::ColorDisabledOverlay));
					g.FillRectangle(&cbr, cr);
					value->Paint(g, cr, theme, _variables);
				}
			}
		}

	protected:
		enum {
			KCVariable=1,
			KCValue,
		};
		strong<Variables> _variables;
		ref<Assignments> _as;
		ref<PropertyGridWnd> _pg;
};

class AssignmentsPopupWnd;

class AssignmentsToolbarWnd: public ToolbarWnd {
	public:
		AssignmentsToolbarWnd(AssignmentsPopupWnd* pw): _pw(pw) {
			Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(assignment_add), false)));
			Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(assignment_remove), false)));
			Add(GC::Hold(new ToolbarItem(KCUp, L"icons/up.png", TL(assignment_up), false)));
			Add(GC::Hold(new ToolbarItem(KCDown, L"icons/down.png", TL(assignment_down), false)));
		}

		virtual ~AssignmentsToolbarWnd() {
		}

		virtual void OnCommand(ref<ToolbarItem> ti) {
			OnCommand(ti->GetCommand());
		}

		virtual void OnCommand(int c);

	protected:
		enum { KCAdd = 1, KCRemove, KCUp, KCDown };
		AssignmentsPopupWnd* _pw;
};

/* AssignmentsPopupWnd */
class AssignmentsPopupWnd: public PopupWnd {
	friend class AssignmentsToolbarWnd;

	public:
		AssignmentsPopupWnd(ref<Assignments> as, strong<Variables> vars): _assignments(as) {
			_tools = GC::Hold(new AssignmentsToolbarWnd(this));
			Add(_tools);

			_pg = GC::Hold(new PropertyGridWnd(false));
			Add(_pg);

			_list = GC::Hold(new AssignmentsListWnd(as,_pg, vars));
			Add(_list);
		}

		virtual ~AssignmentsPopupWnd() {
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			Area rc = GetClientArea();
			g.FillRectangle(&back,rc);

			rc.Narrow(0,0,1,1);
			SolidBrush border(theme->GetColor(Theme::ColorActiveStart));
			Pen pn(&border, 1.0f);
			g.DrawRectangle(&pn, rc);
		}

		virtual void Update() {
			_list->Repaint();
		}

		virtual void Layout() {
			Area rc = GetClientArea();
			rc.Narrow(1,1,1,1);
			if(_tools) _tools->Fill(LayoutTop, rc);
			if(_list && _pg) {
				_list->Move(rc.GetLeft(), rc.GetTop(), rc.GetWidth(), rc.GetHeight()-64);
				_pg->Move(rc.GetLeft(), rc.GetBottom()-64, rc.GetWidth(), 64);
			}
		}

		virtual void OnSize(const Area& ns) {
			Layout();
		}

	protected:
		ref<AssignmentsListWnd> _list;
		ref<AssignmentsToolbarWnd> _tools;
		ref<PropertyGridWnd> _pg;
		ref<Assignments> _assignments;
};

void AssignmentsToolbarWnd::OnCommand(int c) {
	if(c==KCAdd) {
		_pw->_assignments->Add(GC::Hold(new Assignment()));
	}
	else if(c==KCRemove) {
		ref<Assignment> as = _pw->_assignments->GetAssignmentByIndex(_pw->_list->GetSelectedRow());
		if(as) {
			_pw->_assignments->Remove(as);
		}
	}
	else if(c==KCUp || c==KCDown) {
		ref<Assignment> as = _pw->_assignments->GetAssignmentByIndex(_pw->_list->GetSelectedRow());
		if(as) {
			if(c==KCUp) {
				_pw->_assignments->MoveUp(as);
			}
			else {
				_pw->_assignments->MoveDown(as);
			}
		}
	}
	_pw->Update();
}

/* AssignmentsPropertyWnd */
class AssignmentsPropertyWnd: public ChildWnd {
	public:
		AssignmentsPropertyWnd(ref<Assignments> as, strong<Variables> vars): ChildWnd(L""), _variables(vars), _assignIcon(L"icons/assign.png") {
			_as = as;
			SetWantMouseLeave(true);
		}

		virtual ~AssignmentsPropertyWnd() {
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			Area rc = GetClientArea();
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			g.FillRectangle(&back, rc);

			if(IsMouseOver()) {
				theme->DrawToolbarBackground(g, 0.0f, 0.0f, float(rc.GetWidth()), float(rc.GetHeight()));
			}

			_assignIcon.Paint(g, Area(0,0,16,16));
			std::wstring info = L"["+Stringify(_as->GetAssignmentCount())+L"]";
			SolidBrush tbr(theme->GetColor(Theme::ColorActiveStart));
			g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFont(), PointF(20.0f, 2.0f), &tbr);
		}

	protected:	
		virtual void OnSize(const Area& ns) {
			Repaint();
		}

		void DoPopup() {
			if(!_popup) {
				_popup = GC::Hold(new AssignmentsPopupWnd(_as, _variables));
			}

			Area rc = GetClientArea();
			_popup->SetSize(300,200);
			_popup->PopupAt(rc.GetLeft(), rc.GetBottom(), this);
			ModalLoop ml;
			ml.Enter(_popup->GetWindow(), false);
			_popup->Show(false);
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			if(ev==MouseEventMove||ev==MouseEventLeave) {
				Repaint();
			}
			else if(ev==MouseEventLUp) {
				DoPopup();
				Repaint();
			}
		}

		strong<Variables> _variables;
		ref<Assignments> _as;
		ref<AssignmentsPopupWnd> _popup;
		Icon _assignIcon;
};

/* AssignmentsProperty */
AssignmentsProperty::AssignmentsProperty(const std::wstring& name, ref<Assignments> c, strong<Variables> vars): Property(name), _variables(vars) {
	assert(c);
	_assignments = c;
}

AssignmentsProperty::~AssignmentsProperty() {
}

ref<Wnd> AssignmentsProperty::GetWindow() {
	if(!_wnd) {
		_wnd = GC::Hold(new AssignmentsPropertyWnd(_assignments, _variables));
	}
	return _wnd;
}

void AssignmentsProperty::Update() {
	if(_wnd) {
		_wnd->Update();
	}
}