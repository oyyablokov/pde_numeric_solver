/*
    Copyright (c) 2017 Oleg Yablokov

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

**/

#include "pde_settings.h"
#include <QVector2D>
#include <algorithm>
#include <cassert>

PdeSettings::PdeSettings()
{
    set_coords_type(CoordsType::Cartesian, 2);
    set_boundaries();
}

PdeSettings::PdeSettings(const PdeSettings& other)
{
    m_CoordsType = other.m_CoordsType;
    m_Dim = other.m_Dim;
    c = other.c;
    m = other.m;
    m_Coords = other.m_Coords;
    V1_str = other.V1_str;
	V2_str = other.V2_str;
	f_str = other.f_str;
}

PdeSettings::PdeSettings(CoordsType coords_type, int dim)
{
    if (dim == -1) dim = m_Dim;
    set_coords_type(coords_type, dim);
    set_boundaries();
}

PdeSettings::PdeSettings(QVariantMap& map)
{
    reset(map);
}

PdeSettings::~PdeSettings()
{
}

void PdeSettings::set_coords_type(CoordsType new_type, int new_dim)
{
    if ((new_type == m_CoordsType) && (new_dim == -1)) return;

    if (new_dim == -1) new_dim = m_Dim;
    else m_Dim = new_dim;

    m_Coords.clear();
    m_CoordsType = new_type;

    set_boundaries();
	set_defaults();
}

void PdeSettings::set_coords_dim(int new_dim)
{
    set_coords_type(m_CoordsType, new_dim);
}

const PdeSettings::CoordGridSet_t* PdeSettings::get_coord_by_label(QString label) const
{
    for (auto& coord: m_Coords)
    {
        if (coord.label == label) return &coord;
    }
    return NULL;
}

float PdeSettings::evaluate_expression(QString expression, QVector2D x, double t) const
{
	if ((expression == "") || (expression == "0")) return 0;

    //TODO: why problems with declaring QScriptEngine instance in class?
    QScriptEngine m_ScriptEngine1;

	if (!std::isnan(t)) expression.replace("T", QString::number(t));

    if (m_CoordsType == CoordsType::Polar)
    {
        expression.replace("R", QString::number(x[0] / (m * m)));
    }
    else if (m_CoordsType == CoordsType::Cartesian)
    {
        float R = qSqrt(x[0] * x[0] + x[1] * x[1]);
        expression.replace("x", QString::number(x[0] / m));
        expression.replace("y", QString::number(x[1] / m));
        expression.replace("R", QString::number(R / (m * m)));
    }
    else throw("Wrong coords type");

    expression.replace("sqrt", "Math.sqrt");
    expression.replace("sin", "Math.sin");
    expression.replace("cos", "Math.cos");
    expression.replace("tan", "Math.tan");	
    expression.replace("abs", "Math.abs");
    expression.replace("pow", "Math.pow");
    expression.replace("max", "Math.max");
    expression.replace("min", "Math.min");
    expression.replace("PI", "Math.PI");
    expression.replace("E", "Math.E");
    expression.replace("--", "-");

    return float(m_ScriptEngine1.evaluate(expression).toNumber());
}

float PdeSettings::V1(QVector2D x) const
{
    return evaluate_expression(V1_str, x);
}

float PdeSettings::V2(QVector2D x) const
{
    return evaluate_expression(V2_str, x);
}

float PdeSettings::f(QVector2D x, double t) const
{
	return evaluate_expression(f_str, x, t);
}

void PdeSettings::reset(QVariantMap& map)
{
    m_Coords.clear();

    if (map.contains("V1")) V1_str = map["V1"].value<QString>();
	if (map.contains("V2")) V2_str = map["V2"].value<QString>();
	if (map.contains("f")) f_str = map["f"].value<QString>();

    if (map.contains("c")) c = map["c"].value<float>();
    if (map.contains("m")) m = map["m"].value<float>();

	if (map.contains("CoordsType"))
	{
		if (map["CoordsType"].value<QString>() == "Cartesian") m_CoordsType = CoordsType::Cartesian;
		else if (map["CoordsType"].value<QString>() == "Polar") m_CoordsType = CoordsType::Polar;
	}

    QString key, label;
    bool coord_with_current_label_exists = false;
    for(QVariantMap::const_iterator iter = map.begin(); iter != map.end(); ++iter)
    {
        key = iter.key();
        label = "";

        //search for a label (if exsists):
        if (key.contains("count")) label = QString(key).replace("count", "");
        if (key.contains("step")) label = QString(key).replace("step", "");
        if (key.contains("min")) label = QString(key).replace("min", "");
        if (key.contains("max")) label = QString(key).replace("max", "");
        if (label.isEmpty()) continue;

        coord_with_current_label_exists = false;
        for(auto& coord : m_Coords)
        {
            if (coord.label == label)
            {
                coord_with_current_label_exists = true;

                if (key.contains("count")) coord.count = iter.value().value<int>();
                else if (key.contains("step")) coord.step = iter.value().value<float>();
                else if (key.contains("min")) coord.min = iter.value().value<float>();
                else if (key.contains("max")) coord.max = iter.value().value<float>();
                else throw("Error when parsing QVariantMap");

                break;
            }
        }
        if (!coord_with_current_label_exists)
        {
            if (key.contains("count")) m_Coords.push_back(CoordGridSet_t(iter.value().value<int>(), 0.1, 0, 1, label));
            else if (key.contains("step")) m_Coords.push_back(CoordGridSet_t(10, iter.value().value<float>(), 0, 1, label));
            else if (key.contains("min")) m_Coords.push_back(CoordGridSet_t(10, 0.1, iter.value().value<float>(), 1, label));
            else if (key.contains("max")) m_Coords.push_back(CoordGridSet_t(10, 0.1, 0, iter.value().value<float>(), label));
            else throw("Error when parsing QVariantMap");
        }
    }

    //ensure the grid fits the area
    set_boundaries();
}

void PdeSettings::set_defaults()
{
	if (m_CoordsType == CoordsType::Polar)
	{
		V1_str = "30 * pow(E, -R*R)";
		V2_str = "pow(E, -R*R)";
		f_str = "0";
		m_Coords.push_back(CoordGridSet_t(200, 0.05, 0, 10, "R"));
		for (int i = 1; i < m_Dim; ++i)
		{
			m_Coords.push_back(CoordGridSet_t(40, 0.1, 0, 1, "F" + QString::number(i)));
		}
		m_Coords.push_back(CoordGridSet_t(400, 0.02, 0, 8, "T"));
	}
	else if (m_CoordsType == CoordsType::Cartesian)
	{
		V1_str = "10 * pow(E, -(abs(x)+abs(y))/5)*sin((abs(x)+abs(y)))";
		V2_str = "0";
		f_str = "0";
		for (int i = 1; i < m_Dim + 1; ++i)
		{
			m_Coords.push_back(CoordGridSet_t(50, 0.2, 0, 1, "X" + QString::number(i)));
		}
		m_Coords.push_back(CoordGridSet_t(100, 0.001, 0, 1, "T"));
	}
	else throw("Wrong coords type");
}

QVariantMap PdeSettings::toQVariantMap() const
{
    QVariantMap map;

    map.insert("V1", V1_str);
	if (m_CoordsType != CoordsType::Cartesian) map.insert("V2", V2_str);
	map.insert("f", f_str);

	map.insert("m", m);
	map.insert("c", c);

    for (auto& coord : m_Coords)
    {
        map.insert("count" + coord.label, coord.count);
        map.insert("step" + coord.label, coord.step);
    }

	if (m_CoordsType == CoordsType::Cartesian) map.insert("CoordsType", "Cartesian");
	else if (m_CoordsType == CoordsType::Polar) map.insert("CoordsType", "Polar");

    return map;
}

QVariantMap PdeSettings::getQVariantMapToolTips() const
{
    QVariantMap map;

    map.insert("V1", "The initial function u(x, 0)");
	map.insert("V2", "The initial function 𝛿u/𝛿t(x, 0) (if used)");
	map.insert("f", "The right part of the equation.");

    map.insert("c", "A constant (e.g. for the heat equation: 𝛿u/𝛿t = c^2 * Δu)");
    map.insert("m", "The scale coefficient for V1 and V2 functions (i.e. V1(x) -> V1(x / m) and the same for V2)");

    for (auto& coord : m_Coords)
    {
        map.insert("count" + coord.label, "The number of nodes along the " + coord.label + " axis");
        map.insert("step" + coord.label, "The step along the " + coord.label + " axis");
    }

    return map;
}

void PdeSettings::set_boundaries()
{
    for(auto& coord : m_Coords)
    {
        if ((coord.label == "T") || (m_CoordsType == CoordsType::Polar))
        {
            coord.max = coord.step * coord.count;
            coord.min = 0;
        }
        else
        {
            coord.max = (coord.step * coord.count) / 2;
            coord.min = -coord.max;
        }
    }
}
