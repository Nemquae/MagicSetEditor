//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2008 Twan van Laarhoven and "coppro"              |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/filtered_card_list.hpp>

DECLARE_TYPEOF_COLLECTION(CardP);

// ----------------------------------------------------------------------------- : FilteredCardList

FilteredCardList::FilteredCardList(Window* parent, int id, long style)
	: CardListBase(parent, id, style)
{}

void FilteredCardList::setFilter(const CardListFilterP& filter) {
	this->filter = filter;
	rebuild();
}

void FilteredCardList::onChangeSet() {
	// clear filter before changing set, the filter might not make sense for a different set
	filter = CardListFilterP();
	CardListBase::onChangeSet();
}

void FilteredCardList::getItems(vector<VoidP>& out) const {
	if (filter) {
		filter->getItems(set->cards,out);
	}
}

void CardListFilter::getItems(const vector<CardP>& cards, vector<VoidP>& out) const {
	FOR_EACH_CONST(c, cards) {
		if (keep(c)) {
			out.push_back(c);
		}
	}
}