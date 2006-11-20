//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <render/text/viewer.hpp>
#include <algorithm>

DECLARE_TYPEOF_COLLECTION(TextViewer::Line);
DECLARE_TYPEOF_COLLECTION(double);

// ----------------------------------------------------------------------------- : Line

struct TextViewer::Line {
	size_t         start;		///< Index of the first character in this line
	vector<double> positions;	///< x position of each character in this line, gives the number of characters + 1, never empty
	double         top;			///< y position of (the top of) this line
	double         line_height;	///< The height of this line in pixels
	bool           separator_after;	///< Is there a saparator after this line?
	
	Line()
		: start(0), top(0), line_height(0), separator_after(false)
	{}
	
	/// The position (just beyond) the bottom of this line
	double bottom() const { return top + line_height; }
	/// The width of this line
	double width()  const { return positions.back() - positions.front(); }
	/// Index just beyond the last character on this line
	size_t end() const { return start + positions.size() - 1; }
	/// Find the index of the character at the given position on this line
	/** Always returns a value in the range [start..end()) */
	size_t posToIndex(double x) const;
	
	/// Is this line visible using the given rectangle?
	bool visible(const Rotation& rot) const {
		return top + line_height > 0 && top < rot.getInternalSize().height;
	}
	
	/// Draws a selection indicator on this line from start to end
	/** start and end need not be in this line */
	void drawSelection(RotatedDC& dc, size_t start, size_t end);
};

size_t TextViewer::Line::posToIndex(double x) const {
	// largest index with pos <= x
	vector<double>::const_iterator it1 = lower_bound(positions.begin(), positions.end(), x);
	if (it1 == positions.end()) return end();
	// first index with pos > x
	vector<double>::const_iterator it2 = it1 + 1;
	if (it2 == positions.end()) return it1 - positions.begin();
	if (x - *it1 <= *it2 - x)   return it1 - positions.begin(); // it1 is closer
	else                        return it2 - positions.begin(); // it2 is closer
}

// ----------------------------------------------------------------------------- : TextViewer

// can't be declared in header because we need to know sizeof(Line)
TextViewer:: TextViewer() {}
TextViewer::~TextViewer() {}

// ----------------------------------------------------------------------------- : Drawing

void TextViewer::draw(RotatedDC& dc, const String& text, const TextStyle& style, Context& ctx, DrawWhat what) {
	Rotater r(dc, Rotation(style.angle, style.getRect()));
	if (lines.empty()) {
		// not prepared yet
		prepareElements(text, style, ctx);
		prepareLines(dc, text, style);
	}
	// Draw the text  line by line
	FOR_EACH(l, lines) {
		if (l.visible(dc)) {
			RealRect rect(l.positions.front(), l.top, l.width(), l.line_height);
			elements.draw(dc, scale, rect, &*l.positions.begin(), what, l.start, l.end());
		}
	}
}

void TextViewer::drawSelection(RotatedDC& dc, const TextStyle& style, size_t sel_start, size_t sel_end) {
	Rotater r(dc, Rotation(style.angle, style.getRect()));
	if (sel_start == sel_end) return;
	if (sel_end <  sel_start) swap(sel_start, sel_end);
	dc.SetBrush(*wxBLACK_BRUSH);
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetLogicalFunction(wxINVERT);
	FOR_EACH(l, lines) {
		l.drawSelection(dc, sel_start, sel_end);
	}
	dc.SetLogicalFunction(wxCOPY);
}

void TextViewer::Line::drawSelection(RotatedDC& dc, size_t sel_start, size_t sel_end) {
	if (!visible(dc)) return;
	if (sel_start < end() && sel_end > start) {
		double x1 = positions[sel_start];
		double x2 = positions[max(end(), sel_end)];
		dc.DrawRectangle(RealRect(x1, top, x2 - x1, line_height));
	}
}

void TextViewer::reset() {
	elements.clear();
	lines.clear();
}

// ----------------------------------------------------------------------------- : Positions

size_t TextViewer::lineStart(size_t index) const {
	if (lines.empty()) return 0;
	return findLine(index).start;
}

size_t TextViewer::lineEnd(size_t index) const {
	if (lines.empty()) return 0;
	return findLine(index).end();
}

const TextViewer::Line& TextViewer::findLine(size_t index) const {
	FOR_EACH_CONST(l, lines) {
		if (l.end() > index) return l;
	}
	return lines.front();
}

// ----------------------------------------------------------------------------- : Elements

void TextViewer::prepareElements(const String& text, const TextStyle& style, Context& ctx) {
	if (style.always_symbol) {
		elements.elements.clear();
		elements.elements.push_back(new_shared5<SymbolTextElement>(text, 0, text.size(), style.symbol_font, &ctx));
	} else {
		elements.fromString(text, 0, text.size(), style, ctx);
	}
}


// ----------------------------------------------------------------------------- : Layout

void TextViewer::prepareLines(RotatedDC& dc, const String& text, const TextStyle& style) {
	scale = 1;
	prepareLinesScale(dc, text, style, false);
}

bool TextViewer::prepareLinesScale(RotatedDC& dc, const String& text, const TextStyle& style, bool stop_if_too_long) {
	// Try to layout the text at the current scale
	// find character sizes
	vector<CharInfo> chars;
	elements.getCharInfo(dc, scale, 0, text.size(), chars);
	// first line
	lines.clear();
	Line line;
	// size of the line so far
	RealSize line_size(lineLeft(dc, style, 0), 0);
	line.positions.push_back(line_size.width);
	// The word we are currently reading
	RealSize       word_size;
	vector<double> positions_word; // positios for this word
	size_t         word_start = 0;
	// For each character ...
	for(size_t i = 0 ; i < chars.size() ; ++i) {
		CharInfo& c = chars[i];
		// Should we break?
		bool   break_now    = false;
		bool   accept_word  = false; // the current word should be added to the line
		bool   hide_breaker = true;  // hide the \n or _(' ') that caused a line break
		double line_height_multiplier = 1; // multiplier for line height for next line top
		if (c.break_after == BREAK_HARD) {
			break_now   = true;
			accept_word = true;
			line_height_multiplier = style.line_height_hard;
		} else if (c.break_after == BREAK_LINE) {
			line.separator_after = true;
			break_now   = true;
			accept_word = true;
			line_height_multiplier = style.line_height_line;
		} else if (c.break_after == BREAK_SOFT && style.field().multi_line) {
			// Soft break == end of word
			accept_word = true;
		}
		// Add size of the character
		word_size = addHorizontal(word_size, c.size);
		positions_word.push_back(word_size.width);
		// Did the word become too long?
		if (style.field().multi_line && !break_now) {
			double max_width = lineRight(dc, style, line.top);
			if (word_start == line.start && word_size.width > max_width) {
				// single word on this line; the word is too long
				if (stop_if_too_long) {
					return false; // just give up
				} else {
					// force a word break
					break_now = true;
					accept_word = true;
					hide_breaker = false;
					line_height_multiplier = style.line_height_soft;
				}
			} else if (line_size.width + word_size.width > max_width) {
				// line would become too long, break before the current word
				break_now = true;
				line_height_multiplier = style.line_height_soft;
			}
		}
		// Ending the current word
		if (accept_word) {
			// move word pos to line
			FOR_EACH(p, positions_word) {
				line.positions.push_back(line_size.width + p);
			}
			// add size; next word
			line_size = addHorizontal(line_size, word_size);
			word_size = RealSize(0, 0);
			word_start = i + 1;
			positions_word.clear();
		}
		// Breaking (ending the current line)
		if (break_now) {
			// remove the _('\n') or _(' ') that caused the break
			if (hide_breaker && line.positions.size() > 1) {
				line.positions.pop_back();
			}
			// height of the line
			if (line_size.height < 0.01 && !lines.empty()) {
				// if a line has 0 height, use the height of the line above it, but at most once
			} else {
				line.line_height = line_size.height;
			}
			// push
			lines.push_back(line);
			// reset line object for next line
			line.top += line.line_height * line_height_multiplier;
			line.start = word_start;
			line.positions.clear();
			if (line.separator_after) line.line_height = 0;
			line.separator_after = false;
			// reset line_size
			line_size = RealSize(lineLeft(dc, style, line.top), 0);
			line.positions.push_back(line_size.width); // start position
		}
	}
	// the last word
	FOR_EACH(p, positions_word) {
		line.positions.push_back(line_size.width + p);
	}
	line_size = addHorizontal(line_size, word_size);
	// the last line
	if (line_size.height < 0.01 && !lines.empty()) {
		// if a line has 0 height, use the height of the line above it, but at most once
	} else {
		line.line_height = line_size.height;
	}
	lines.push_back(line);
	return true;
}

double TextViewer::lineLeft(RotatedDC& dc, const TextStyle& style, double y) {
	return 0 + style.padding_left;
//	return style.mask.rowLeft(y, dc.getInternalSize()) + style.padding_left;
}
double TextViewer::lineRight(RotatedDC& dc, const TextStyle& style, double y) {
	return style.width - style.padding_right;
//	return style.mask.rowRight(y, dc.getInternalSize()) - style.padding_right;
}
ContourMask::ContourMask() {} // MOVEME //@@
ContourMask::~ContourMask() {}