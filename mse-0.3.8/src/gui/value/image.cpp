//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2008 Twan van Laarhoven and "coppro"              |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/value/image.hpp>
#include <gui/image_slice_window.hpp>
#include <data/format/clipboard.hpp>
#include <data/action/value.hpp>
#include <wx/clipbrd.h>

// ----------------------------------------------------------------------------- : ImageValueEditor

IMPLEMENT_VALUE_EDITOR(Image) {}

bool ImageValueEditor::onLeftDClick(const RealPoint&, wxMouseEvent&) {
	String filename = wxFileSelector(_("Open image file"), settings.default_image_dir, _(""), _(""),
		                             _("All images|*.bmp;*.jpg;*.png;*.gif|Windows bitmaps (*.bmp)|*.bmp|JPEG images (*.jpg;*.jpeg)|*.jpg;*.jpeg|PNG images (*.png)|*.png|GIF images (*.gif)|*.gif|TIFF images (*.tif;*.tiff)|*.tif;*.tiff"),
		                             wxOPEN, wxGetTopLevelParent(&editor()));
	if (!filename.empty()) {
		settings.default_image_dir = wxPathOnly(filename);
		sliceImage(wxImage(filename));
	}
	return true;
}

void ImageValueEditor::sliceImage(const Image& image) {
	if (!image.Ok()) return;
	// mask
	GeneratedImage::Options options((int)style().width, (int)style().height, &viewer.getStylePackage(), &viewer.getLocalPackage());
	AlphaMask mask;
	style().mask.getNoCache(options,mask);
	// slice
	ImageSliceWindow s(wxGetTopLevelParent(&editor()), image, style().getSize(), mask);
	// clicked ok?
	if (s.ShowModal() == wxID_OK) {
		// store the image into the set
		FileName new_image_file = getLocalPackage().newFileName(field().name,_("")); // a new unique name in the package
		Image img = s.getImage();
		img.SaveFile(getLocalPackage().nameOut(new_image_file), img.HasAlpha() ? wxBITMAP_TYPE_PNG : wxBITMAP_TYPE_JPEG);
		addAction(value_action(valueP(), new_image_file));
	}
}

// ----------------------------------------------------------------------------- : Clipboard

bool ImageValueEditor::canCopy() const {
	return !value().filename.empty();
}

bool ImageValueEditor::canPaste() const {
	return wxTheClipboard->IsSupported(wxDF_BITMAP) &&
	      !wxTheClipboard->IsSupported(CardsDataObject::format); // we don't want to (accidentally) paste card images
}

bool ImageValueEditor::doCopy() {
	// load image
	InputStreamP image_file = getLocalPackage().openIn(value().filename);
	Image image;
	if (!image.LoadFile(*image_file)) return false;
	// set data
	if (!wxTheClipboard->Open()) return false;
	bool ok = wxTheClipboard->SetData(new wxBitmapDataObject(image));
	wxTheClipboard->Close();
	return ok;
}

bool ImageValueEditor::doPaste() {
	// get bitmap
	if (!wxTheClipboard->Open()) return false;
	wxBitmapDataObject data;
	bool ok = wxTheClipboard->GetData(data);
	wxTheClipboard->Close();
	if (!ok)  return false;
	// slice
	sliceImage(data.GetBitmap().ConvertToImage());
	return true;
}

bool ImageValueEditor::doDelete() {
	addAction(value_action(valueP(), FileName()));
	return true;
}


bool ImageValueEditor::onChar(wxKeyEvent& ev) {
	if (ev.AltDown() || ev.ShiftDown() || ev.ControlDown()) return false;
	switch (ev.GetKeyCode()) {
		case WXK_DELETE:
			doDelete();
			return true;
		default:
			return false;
	}
}