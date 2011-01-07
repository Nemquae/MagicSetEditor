//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <data/statistics.hpp>
#include <data/field.hpp>
#include <data/field/choice.hpp>

DECLARE_TYPEOF_COLLECTION(String);
DECLARE_TYPEOF_COLLECTION(StatsDimensionP);
DECLARE_TYPEOF_COLLECTION(ChoiceField::ChoiceP);

// ----------------------------------------------------------------------------- : Statistics dimension

StatsDimension::StatsDimension()
	: automatic (false)
	, numeric   (false)
	, show_empty(false)
{}

StatsDimension::StatsDimension(const Field& field)
	: automatic    (true)
	, name         (field.name)
	, description  (field.description)
	, icon_filename(field.icon_filename)
	, numeric      (false)
	, show_empty   (false)
{
	// choice field?
	const ChoiceField* choice_field = dynamic_cast<const ChoiceField*>(&field);
	if (choice_field) {
		colors = choice_field->choice_colors;
		/*int count = choice_field->choices->lastId();
		for (int i = 0 ; i < count ; ++i) {
			groups.push_back(choice_field->choices->choiceName(i));
		}*/
		// only top level choices
		FOR_EACH_CONST(g, choice_field->choices->choices) {
			groups.push_back(g->name);
		}
		// initialize script, primary_choice(card.{field_name})
		Script& s = script.getScript();
		s.addInstruction(I_GET_VAR,  string_to_variable(_("primary choice")));
		s.addInstruction(I_GET_VAR,  string_to_variable(_("card")));
		s.addInstruction(I_MEMBER_C, field.name);
		s.addInstruction(I_CALL,     1);
		s.addInstruction(I_NOP,      string_to_variable(_("input")));
		s.addInstruction(I_RET);
	} else {
		// initialize script, card.{field_name}
		Script& s = script.getScript();
		s.addInstruction(I_GET_VAR,  string_to_variable(_("card")));
		s.addInstruction(I_MEMBER_C, field.name);
		s.addInstruction(I_RET);
	}
}

IMPLEMENT_REFLECTION_NO_GET_MEMBER(StatsDimension) {
	if (!automatic) {
		REFLECT(name);
		REFLECT(description);
		REFLECT_N("icon", icon_filename);
		REFLECT(script);
		REFLECT(numeric);
		REFLECT(show_empty);
		REFLECT(colors);
		REFLECT(groups);
	}
}

// ----------------------------------------------------------------------------- : Statistics category

StatsCategory::StatsCategory()
	: automatic(false)
	, type(GRAPH_TYPE_BAR)
{}

StatsCategory::StatsCategory(const StatsDimensionP& dim)
	: automatic(true)
	, name         (dim->name)
	, description  (dim->description)
	, icon_filename(dim->icon_filename)
	, dimensions(1, dim)
	, type(GRAPH_TYPE_BAR)
{}

IMPLEMENT_REFLECTION_NO_GET_MEMBER(StatsCategory) {
	if (!automatic) {
		REFLECT(name);
		REFLECT(description);
		REFLECT_N("icon", icon_filename);
		REFLECT(type);
		REFLECT_N("dimensions", dimension_names);
	}
}

void StatsCategory::find_dimensions(const vector<StatsDimensionP>& available) {
	if (!dimensions.empty()) return;
	FOR_EACH_CONST(n, dimension_names) {
		StatsDimensionP dim;
		FOR_EACH_CONST(d, available) {
			if (d->name == n) {
				dim = d;
				break;
			}
		}
		if (!dim) {
			handle_error(_ERROR_1_("dimension not found",dim),false);
		} else {
			dimensions.push_back(dim);
		}
	}
}

// ----------------------------------------------------------------------------- : GraphType (from graph_type.hpp)

IMPLEMENT_REFLECTION_ENUM(GraphType) {
	VALUE_N("bar",         GRAPH_TYPE_BAR);
	VALUE_N("pie",         GRAPH_TYPE_PIE);
	VALUE_N("stack",       GRAPH_TYPE_STACK);
	VALUE_N("scatter",     GRAPH_TYPE_SCATTER);
	VALUE_N("scatter pie", GRAPH_TYPE_SCATTER_PIE);
}