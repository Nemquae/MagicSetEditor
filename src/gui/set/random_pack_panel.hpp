//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2008 Twan van Laarhoven and "coppro"              |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#ifndef HEADER_GUI_SET_RANDOM_PACK_PANEL
#define HEADER_GUI_SET_RANDOM_PACK_PANEL

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/set/panel.hpp>

class CardViewer;
class FilteredCardList;

// ----------------------------------------------------------------------------- : RandomPackPanel

/// A SetWindowPanel for creating random booster packs
class RandomPackPanel : public SetWindowPanel {
  public:
	RandomPackPanel(Window* parent, int id);
	
	// --------------------------------------------------- : UI
	
	virtual void onChangeSet();
	
	virtual void initUI   (wxToolBar* tb, wxMenuBar* mb);
	virtual void destroyUI(wxToolBar* tb, wxMenuBar* mb);
	virtual void onUpdateUI(wxUpdateUIEvent&);
	virtual void onCommand(int id);
	
	// --------------------------------------------------- : Clipboard
	
	virtual bool canCopy()  const;
	virtual void doCopy();
	
  private:
	CardViewer*       preview;		///< Card preview
	FilteredCardList* card_list;	///< The list of cards
};

// ----------------------------------------------------------------------------- : EOF
#endif