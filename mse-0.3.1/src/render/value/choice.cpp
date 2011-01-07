//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <render/value/choice.hpp>
#include <render/card/viewer.hpp>
#include <data/stylesheet.hpp>

// ----------------------------------------------------------------------------- : ChoiceValueViewer

void ChoiceValueViewer::draw(RotatedDC& dc) {
	drawFieldBorder(dc);
	if (style().render_style & RENDER_HIDDEN) return;
	if (value().value().empty()) return;
	double margin = 0;
	if (style().render_style & RENDER_IMAGE) {
		// draw image
		map<String,ScriptableImage>::iterator it = style().choice_images.find(cannocial_name_form(value().value()));
		if (it != style().choice_images.end()) {
			ScriptableImage& img = it->second;
			ScriptImageP i;
			if (nativeLook()) {
				i = img.update(viewer.getContext(), *viewer.stylesheet, 16, 16, ASPECT_BORDER, false);
			} else if(style().render_style & RENDER_TEXT) {
				// also drawing text
				i = img.update(viewer.getContext(), *viewer.stylesheet, 0, 0);
			} else {
				i = img.update(viewer.getContext(), *viewer.stylesheet,
						(int) dc.trS(style().width), (int) dc.trS(style().height),
						style().alignment == ALIGN_STRETCH ? ASPECT_STRETCH : ASPECT_FIT
					);
			}
			if (i) {
				// apply mask?
				style().loadMask(*viewer.stylesheet);
				if (style().mask.Ok()) {
					set_alpha(i->image, style().mask);
				}
				// draw
				dc.DrawImage(i->image,
					align_in_rect(style().alignment, RealSize(i->image.GetWidth(), i->image.GetHeight()), style().getRect()),
					i->combine == COMBINE_NORMAL ? style().combine : i->combine
				);
				margin = dc.trInvS(i->image.GetWidth()) + 1;
			}
		}
	}
	if (style().render_style & RENDER_TEXT) {
		// draw text
		dc.DrawText(tr(*viewer.stylesheet, value().value(), capitalize(value().value())),
			align_in_rect(ALIGN_MIDDLE_LEFT, RealSize(0, dc.GetCharHeight()), style().getRect()) + RealSize(margin, 0)
		);
	}
}