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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/tjdashboard.h"
#include "../../include/internal/view/tjcueproperty.h"

#include <iomanip>

using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

namespace tj {
	namespace show {
		namespace view {
			namespace widgets {
				/** TestWidget **/
				class TestWidget: public Widget {
					public:
						TestWidget() {
							SetPreferredSize(Area(0,0,64,64));
							GetLayoutSizing().Set(LayoutResizeRetainAspectRatio,false);
						}

						virtual ~TestWidget() {
						}

						virtual void Save(TiXmlElement* el) {
						}

						virtual void Load(TiXmlElement* el) {
						}

						virtual void Paint(graphics::Graphics& g, strong<Theme> theme) {
							Area rc = GetClientArea();
							SolidBrush red(Color(255,0,0));
							g.FillRectangle(&red, rc);
						}
				};

				class TriggerWidget: public Widget {
					public:
						TriggerWidget(): _color(1.0f, 0.0f, 0.4f), _down(false), _disableOnCondition(false), _disableOnCueCondition(false), _condition(GC::Hold(new NullExpression())) {
							SetPreferredSize(Area(0,0,32,24));
							GetLayoutSizing().Set(LayoutResizeRetainAspectRatio,false);
						}

						virtual ~TriggerWidget() {
						}

						virtual void Save(TiXmlElement* el) {
							SaveAttributeSmall(el, "text", _text);
							SaveAttributeSmall(el, "disable-on-condition", _disableOnCondition);
							SaveAttributeSmall(el, "disable-on-cue-condition", _disableOnCueCondition);
							TiXmlElement color("color");
							_color.Save(&color);
							el->InsertEndChild(color);

							TiXmlElement trigger("trigger");
							SaveAttributeSmall(&trigger, "timeline", _tlid);
							SaveAttributeSmall(&trigger, "cue", _cid);
							el->InsertEndChild(trigger);

							TiXmlElement condition("condition");
							_condition->Save(&condition);
							el->InsertEndChild(condition);
						}

						virtual void Load(TiXmlElement* el) {
							_text = LoadAttributeSmall(el, "text", _text);
							_disableOnCondition = LoadAttributeSmall(el, "disable-on-condition", _disableOnCondition);
							_disableOnCueCondition = LoadAttributeSmall(el, "disable-on-cue-condition", _disableOnCueCondition);
							TiXmlElement* color = el->FirstChildElement("color");
							if(color) {
								_color.Load(color);
							}

							TiXmlElement* trigger = el->FirstChildElement("trigger");
							if(trigger!=0) {
								_cid = LoadAttributeSmall(trigger, "cue", _cid);
								_tlid = LoadAttributeSmall(trigger, "timeline", _tlid);
							}

							TiXmlElement* condition = el->FirstChildElement("condition");
							if(condition!=0) {
								_condition->Load(condition);
							}
						}

						virtual bool IsEnabled() {
							if(_disableOnCondition) {
								bool result = _condition->Evaluate(Application::Instance()->GetModel()->GetVariables());
								if(!result) {
									return false;
								}
							}

							if(_disableOnCueCondition) {
								ref<Instance> inst = Application::Instance()->GetInstances()->GetInstanceByTimelineID(_tlid);
								if(inst) {
									strong<CueList> cl = inst->GetCueList();
									ref<Cue> cue = cl->GetCueByID(_cid);
									if(cue) {
										ref<Expression> co = cue->GetCondition();
										if(co) {
											bool result = cue->GetCondition()->Evaluate(inst->GetVariables());
											if(!result) {
												return false;
											}
										}
									}
								}
							}

							return true;
						}

						virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
							if(ev==MouseEventLDown) {
								if(IsEnabled()) {
									_down = true;
									Repaint();

									ref<Instance> inst = Application::Instance()->GetInstances()->GetInstanceByTimelineID(_tlid);
									if(inst) {
										strong<CueList> cl = inst->GetCueList();
										ref<Cue> cue = cl->GetCueByID(_cid);
										if(cue) {
											inst->Trigger(cue);
										}
									}
								}
							}
							else if(ev==MouseEventLUp) {
								_down = false;
								Repaint();
							}
						}

						virtual ref<PropertySet> GetProperties() {
							ref<PropertySet> ps = Widget::GetProperties();
							ps->Add(GC::Hold(new PropertySeparator(TL(dashboard_widget_trigger))));
							ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dashboard_widget_trigger_title), this, &_text, L"")));
							ps->Add(GC::Hold(new ColorProperty(TL(dashboard_widget_trigger_color), this, &_color)));
							ps->Add(GC::Hold(new DistantCueProperty(TL(dashboard_widget_trigger_cue), null, &_tlid, &_cid)));

							ps->Add(GC::Hold(new PropertySeparator(TL(dashboard_widget_trigger_disable))));
							ps->Add(GC::Hold(new GenericProperty<bool>(TL(dashboard_widget_trigger_disable_on_condition), this, &_disableOnCondition, _disableOnCondition)));
							ps->Add(GC::Hold(new ExpressionProperty(TL(dashboard_widget_trigger_condition), _condition, Application::Instance()->GetModel()->GetVariables())));
							ps->Add(GC::Hold(new GenericProperty<bool>(TL(dashboard_widget_trigger_disable_on_cue_condition), this, &_disableOnCueCondition, _disableOnCueCondition)));


							return ps;
						}

						virtual void Paint(graphics::Graphics& g, strong<Theme> theme) {
							bool enabled = IsEnabled();
							Area rc = GetClientArea();
							rc.Narrow(1,1,1,1);
							SolidBrush disabledOverlay(theme->GetColor(Theme::ColorDisabledOverlay));
							g.FillRoundRectangle(&disabledOverlay, rc, 7.0f);

							SolidBrush borderBrush(_color);
							Pen borderPen(&borderBrush, 1.0f);
							LinearGradientBrush colorBrush(PointF(0.0f, rc.GetTop()-3.0f), PointF(0.0f, rc.GetBottom()+3.0f), Theme::ChangeAlpha(_color, 100), _color);
							LinearGradientBrush colorDownBrush(PointF(0.0f, rc.GetBottom()+3.0f), PointF(0.0f, rc.GetTop()-3.0f), Theme::ChangeAlpha(_color, 100), _color);
							
							g.FillRoundRectangle(_down ? (&colorDownBrush) : (&colorBrush), rc, 4.0f);

							StringFormat sf;
							sf.SetAlignment(StringAlignmentCenter);
							SolidBrush textBrush(theme->GetColor(Theme::ColorBackground));
							g.DrawString(_text.c_str(), (int)_text.length(), theme->GetGUIFontBold(), rc, &sf, &textBrush);	

							if(!enabled) {
								g.FillRoundRectangle(&disabledOverlay, rc, 7.0f);
							}

							g.DrawRoundRectangle(&borderPen, rc, 4.0f);
						}

						RGBColor _color;
						std::wstring _text;
						bool _down;
						bool _disableOnCondition;
						bool _disableOnCueCondition;
						strong<Expression> _condition;

						// Target cue information
						std::wstring _tlid;
						std::wstring _cid;
				};

				class StatusWidget: public Widget {
					public:
						StatusWidget():
							_expression(GC::Hold(new NullExpression())),
							_backgroundIcon(L"icons/widgets/status/background.png"),
							_redIcon(L"icons/widgets/status/red-overlay.png"),
							_orangeIcon(L"icons/widgets/status/orange-overlay.png"),
							_greenIcon(L"icons/widgets/status/green-overlay.png"),
							_minGreenValue(1.0f), _maxGreenValue(100.0f),
							_minOrangeValue(0.0f), _maxOrangeValue(1.0f),
							_minRedValue(-100.0f), _maxRedValue(0.0f)
							{
								SetPreferredSize(Area(0,0,48,96));
						}

						virtual ~StatusWidget() {
						}

						virtual ref<PropertySet> GetProperties() {
							ref<PropertySet> ps = Widget::GetProperties();
							ps->Add(GC::Hold(new PropertySeparator(TL(dashboard_widget_status))));
							ps->Add(GC::Hold(new ExpressionProperty(TL(dashboard_widget_status_expression), _expression, Application::Instance()->GetInstances()->GetRootInstance()->GetVariables())));

							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_status_min_red_value), this, &_minRedValue, _minRedValue)));
							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_status_max_red_value), this, &_maxRedValue, _maxRedValue)));

							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_status_min_orange_value), this, &_minOrangeValue, _minOrangeValue)));
							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_status_max_orange_value), this, &_maxOrangeValue, _maxOrangeValue)));
							
							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_status_min_green_value), this, &_minGreenValue, _minGreenValue)));
							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_status_max_green_value), this, &_maxGreenValue, _maxGreenValue)));
							
							return ps;
						}

						virtual void Save(TiXmlElement* el) {
							SaveAttributeSmall(el, "min-green", _minGreenValue);
							SaveAttributeSmall(el, "max-green", _maxGreenValue);
							SaveAttributeSmall(el, "min-orange", _minOrangeValue);
							SaveAttributeSmall(el, "max-orange", _maxOrangeValue);
							SaveAttributeSmall(el, "min-red", _minRedValue);
							SaveAttributeSmall(el, "max-red", _maxRedValue);


							TiXmlElement expressionElement("value");
							_expression->Save(&expressionElement);
							el->InsertEndChild(expressionElement);
						}

						virtual void Load(TiXmlElement* el) {
							_minGreenValue = LoadAttributeSmall(el, "min-green", _minGreenValue);
							_maxGreenValue = LoadAttributeSmall(el, "max-green", _maxGreenValue);
							_minOrangeValue = LoadAttributeSmall(el, "min-orange", _minOrangeValue);
							_maxOrangeValue = LoadAttributeSmall(el, "max-orange", _maxOrangeValue);
							_minRedValue = LoadAttributeSmall(el, "min-red", _minRedValue);
							_maxRedValue = LoadAttributeSmall(el, "max-red", _maxRedValue);

							TiXmlElement* value = el->FirstChildElement("value");
							if(value!=0) {
								_expression->Load(value);
							}
						}

						virtual void Paint(graphics::Graphics& g, strong<Theme> theme) {
							Area rc = GetClientArea();
							_backgroundIcon.Paint(g, rc);

							// Retrieve value
							Any expressionValue = _expression->Evaluate(Application::Instance()->GetInstances()->GetRootInstance()->GetVariables());
							float displayValue = expressionValue;

							if(displayValue >= _minGreenValue && displayValue < _maxGreenValue) {
								// Draw green
								_greenIcon.Paint(g,rc);
							}
							else if(displayValue >= _minOrangeValue && displayValue < _maxOrangeValue) {
								_orangeIcon.Paint(g,rc);
							}
							else if(displayValue >= _minRedValue && displayValue < _maxRedValue) {
								_redIcon.Paint(g,rc);
							}
						}

					protected:
						strong<Expression> _expression;
						Icon _backgroundIcon, _redIcon, _greenIcon, _orangeIcon;
						float _minGreenValue;
						float _maxGreenValue;
						float _minRedValue;
						float _maxRedValue;
						float _minOrangeValue;
						float _maxOrangeValue;

				};

				/** MeterWidget **/
				class MeterWidget: public Widget {
					public:
						MeterWidget(): 
						  _expression(GC::Hold(new NullExpression())),
						  _arrowIcon(L"icons/widgets/meter/arrow.png"), 
						  _backgroundIcon(L"icons/widgets/meter/background.png"),
						  _showValue(false),
						  _minValue(0.0f),
						  _maxValue(100.0f) {
							SetPreferredSize(Area(0,0,48,48));
						}

						virtual ~MeterWidget() {
						}

						virtual ref<PropertySet> GetProperties() {
							ref<PropertySet> ps = Widget::GetProperties();
							ps->Add(GC::Hold(new PropertySeparator(TL(dashboard_widget_meter))));
							ps->Add(GC::Hold(new ExpressionProperty(TL(dashboard_widget_meter_expression), _expression, Application::Instance()->GetInstances()->GetRootInstance()->GetVariables())));
							ps->Add(GC::Hold(new GenericProperty<bool>(TL(dashboard_widget_meter_show_value), this, &_showValue, _showValue)));
							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_meter_minimum_value), this, &_minValue, _minValue)));
							ps->Add(GC::Hold(new GenericProperty<float>(TL(dashboard_widget_meter_maximum_value), this, &_maxValue, _maxValue)));
							return ps;
						}

						virtual void Save(TiXmlElement* el) {
							SaveAttributeSmall(el, "show-value", _showValue);
							SaveAttributeSmall(el, "min-value", _minValue);
							SaveAttributeSmall(el, "max-value", _maxValue);
							TiXmlElement expressionElement("value");
							_expression->Save(&expressionElement);
							el->InsertEndChild(expressionElement);
						}

						virtual void Load(TiXmlElement* el) {
							_showValue = LoadAttributeSmall(el, "show-value", _showValue);
							_minValue = LoadAttributeSmall(el, "min-value", _minValue);
							_maxValue = LoadAttributeSmall(el, "max-value", _maxValue);

							TiXmlElement* value = el->FirstChildElement("value");
							if(value!=0) {
								_expression->Load(value);
							}
						}

						virtual void Paint(graphics::Graphics& g, strong<Theme> theme) {
							Area rc = GetClientArea();
							_backgroundIcon.Paint(g, rc);

							// Retrieve value
							Any expressionValue = _expression->Evaluate(Application::Instance()->GetInstances()->GetRootInstance()->GetVariables());
							float displayValue = expressionValue;

							// Draw value arrow
							float angle = Clamp(displayValue, _minValue, _maxValue) / (_maxValue-_minValue) * 270.0f;
							GraphicsContainer gc = g.BeginContainer();
							g.TranslateTransform(float(rc.GetLeft()+rc.GetWidth()/2), float(rc.GetTop()+rc.GetHeight()/2));
							g.RotateTransform(angle);
							_arrowIcon.Paint(g, Area(-rc.GetWidth()/2, -rc.GetHeight()/2, rc.GetWidth(), rc.GetHeight()));
							g.EndContainer(gc);

							if(_showValue) {
								std::wostringstream wos;
								wos << std::setprecision(2) << displayValue;
								std::wstring valueShown = wos.str();
								StringFormat sf;
								sf.SetAlignment(StringAlignmentCenter);
								SolidBrush tbr(theme->GetColor(Theme::ColorText));
								Area textRC(rc.GetLeft(), rc.GetBottom()-20, rc.GetWidth(), 20);
								g.DrawString(valueShown.c_str(), (int)valueShown.length(), theme->GetGUIFontBold(), textRC, &sf, &tbr);
							}
						}

					protected:
						strong<Expression> _expression;
						Icon _backgroundIcon, _arrowIcon;
						float _minValue, _maxValue;
						bool _showValue;
				};
			}
		}
	}
}

using namespace tj::show::view::widgets;

/** WidgetFactory **/
WidgetFactory::~WidgetFactory() {
}

ref<Widget> WidgetFactory::DoCreateWidgetMenu(ref<Wnd> wnd, Pixels x, Pixels y) {
	ContextMenu cm;
	cm.AddSeparator(TL(dashboard_widget_add));
	std::deque< ref<Prototype<Widget> > > options;

	std::map<std::wstring, ref<Prototype<Widget> > >::iterator it = _prototypes.begin();
	while(it!=_prototypes.end()) {
		ref<Prototype<Widget> > pr = it->second;
		if(pr) {
			cm.AddItem(Language::Get(pr->GetFriendlyName()), options.size(), false, false);
			options.push_back(pr);
		}
		++it;
	}

	ref<MenuItem> mi = cm.DoContextMenuByItem(wnd, x, y);
	if(mi) {
		int c = mi->GetCommandCode();
		if(c>=0 && c<int(options.size())) {
			ref< Prototype<Widget> > prw = options.at(c);
			if(prw) {
				return prw->CreateInstance();
			}
		}
	}
	return null;
}

ref<WidgetFactory> WidgetFactory::_instance;

strong<WidgetFactory> WidgetFactory::Instance() {
	if(!_instance) {
		_instance = GC::Hold(new WidgetFactory());
		
		// Friendly names of the prototypes are TL'ed in DoCreateWidgetMenu, so we can support language switching at runtime
		_instance->RegisterPrototype(L"meter",GC::Hold(new SubclassedPrototype<MeterWidget, Widget>(L"dashboard_widget_meter")));
		_instance->RegisterPrototype(L"status",GC::Hold(new SubclassedPrototype<StatusWidget, Widget>(L"dashboard_widget_status")));
		_instance->RegisterPrototype(L"trigger-button", GC::Hold(new SubclassedPrototype<TriggerWidget, Widget>(L"dashboard_widget_trigger")));
		
		//_instance->RegisterPrototype(L"test",GC::Hold(new SubclassedPrototype<TestWidget, Widget>(L"TEST")));
	}
	return _instance;
}