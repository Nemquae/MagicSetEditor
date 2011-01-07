//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2008 Twan van Laarhoven and "coppro"              |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/util.hpp>
#include <util/error.hpp>
#include <util/rotation.hpp>
#include <wx/renderer.h>
#include <wx/stdpaths.h>

#if wxUSE_UXTHEME && defined(__WXMSW__)
	#include <wx/msw/uxtheme.h>
	#include <tmschema.h>
	#include <shlobj.h>
	#include <wx/mstream.h>
#endif

// ----------------------------------------------------------------------------- : Window related

// Id of the control that has the focus, or -1 if no control has the focus
int focused_control(const Window* window) {
	Window* focused_window = wxWindow::FindFocus();
	// is this window actually inside this panel?
	if (focused_window && wxWindow::FindWindowById(focused_window->GetId(), window) == focused_window) {
		return focused_window->GetId();
	} else {
		return -1; // no window has the focus, or it has a different parent/ancestor
	}
}

// ----------------------------------------------------------------------------- : DC related

/// Fill a DC with a single color
void clearDC(DC& dc, const wxBrush& brush) {
	wxSize size = dc.GetSize();
	wxPoint pos = dc.GetDeviceOrigin(); // don't you love undocumented methods?
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(brush);
	dc.DrawRectangle(-pos.x, -pos.y, size.GetWidth(), size.GetHeight());
}

void clearDC_black(DC& dc) {
	#if !BITMAPS_DEFAULT_BLACK
		// On windows 9x it seems that bitmaps are not black by default
		clearDC(dc, *wxBLACK_BRUSH);
	#endif
}

void draw_checker(RotatedDC& dc, const RealRect& rect) {
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(*wxWHITE_BRUSH);
	dc.DrawRectangle(rect);
	dc.SetBrush(Color(235,235,235));
	const double checker_size = 10;
	int odd = 0;
	for (double y = 0 ; y < rect.height ; y += checker_size) {
		for (double x = odd * checker_size ; x < rect.width ; x += checker_size * 2) {
			dc.DrawRectangle(RealRect(
								rect.x + x,
								rect.y + y,
								min(checker_size, rect.width  - x),
								min(checker_size, rect.height - y)
							));
		}
		odd = 1 - odd;
	}
}

// ----------------------------------------------------------------------------- : Resource related

Image load_resource_image(const String& name) {
	#if defined(__WXMSW__)
		// Load resource
		// based on wxLoadUserResource
		// The image can be in an IMAGE resource, in any file format
		HRSRC hResource = ::FindResource(wxGetInstance(), name, _("IMAGE"));
		if ( hResource == 0 ) throw InternalError(String::Format(_("Resource not found: %s"), name));
		
		HGLOBAL hData = ::LoadResource(wxGetInstance(), hResource);
		if ( hData == 0 ) throw InternalError(String::Format(_("Resource not an image: %s"), name));
		
		char* data = (char *)::LockResource(hData);
		if ( !data ) throw InternalError(String::Format(_("Resource cannot be locked: %s"), name));
		
		int len = ::SizeofResource(wxGetInstance(), hResource);
		wxMemoryInputStream stream(data, len);
		return wxImage(stream);
	#elif defined(__linux__)
		static String path = wxStandardPaths::Get().GetDataDir() + _("/resource/");
		String file = path + name;
		wxImage resource;
		if (wxFileExists(file + _(".png"))) resource.LoadFile(file + _(".png"));
		else if (wxFileExists(file + _(".bmp"))) resource.LoadFile(file + _(".bmp"));
		else if (wxFileExists(file + _(".ico"))) resource.LoadFile(file + _(".ico"));
		else if (wxFileExists(file + _(".cur"))) resource.LoadFile(file + _(".cur"));
		if (!resource.Ok()) handle_error(InternalError(String(_("Cannot find resource file at ")) + file));
		return resource;
	#else
		#error Handling of resource loading needs to be declared.
	#endif
}

wxCursor load_resource_cursor(const String& name) {
	#if defined(__WXMSW__)
		return wxCursor(_("cursor/") + name, wxBITMAP_TYPE_CUR_RESOURCE);
	#else
		return wxCursor(load_resource_image(_("cursor/") + name));
	#endif
}

wxIcon load_resource_icon(const String& name) {
	#if defined(__WXMSW__)
		return wxIcon(_("icon/") + name);
	#else
		static String path = wxStandardPaths::Get().GetDataDir() + _("/resource/icon/");
		return wxIcon(path + name + _(".ico"), wxBITMAP_TYPE_ICO);
	#endif
}

wxBitmap load_resource_tool_image(const String& name) {
	#if defined(__WXMSW__)
		return load_resource_image(_("tool/") + name);
	#else
		return load_resource_image(_("tool/") + name);
	#endif
}

// ----------------------------------------------------------------------------- : Platform look

// Draw a basic 3D border
void draw3DBorder(DC& dc, int x1, int y1, int x2, int y2) {
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW));
	dc.DrawLine(x1, y1, x2, y1);
	dc.DrawLine(x1, y1, x1, y2);
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW));
	dc.DrawLine(x1-1, y1-1, x2+1, y1-1);
	dc.DrawLine(x1-1, y1-1, x1-1, y2+1);
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT));
	dc.DrawLine(x1, y2, x2, y2);
	dc.DrawLine(x2, y1, x2, y2+1);
	dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT));
	dc.DrawLine(x1-1, y2+1, x2+1, y2+1);
	dc.DrawLine(x2+1, y1-1, x2+1, y2+2);
}

void draw_control_border(Window* win, DC& dc, const wxRect& rect) {
	#if wxUSE_UXTHEME && defined(__WXMSW__)
		RECT r;
		wxUxThemeEngine *themeEngine = wxUxThemeEngine::Get();
		if (themeEngine && themeEngine->IsAppThemed()) {
			wxUxThemeHandle hTheme(win, L"EDIT");
			r.left = rect.x -1;
			r.top = rect.y  -1;
			r.right = rect.x + rect.width + 1;
			r.bottom = rect.y + rect.height + 1;
			if (hTheme) {
				wxUxThemeEngine::Get()->DrawThemeBackground(
					(HTHEME)hTheme,
					(HDC)dc.GetHDC(),
					EP_EDITTEXT,
					ETS_NORMAL,
					&r,
					NULL
				);
				return;
			}
		}
		r.left   = rect.x - 2;
		r.top    = rect.y - 2;
		r.right  = rect.x + rect.width  + 2;
		r.bottom = rect.y + rect.height + 2;
		DrawEdge((HDC)dc.GetHDC(), &r, EDGE_SUNKEN, BF_RECT);
	#else
		draw3DBorder(dc, rect.x - 1, rect.y - 1, rect.x + rect.width, rect.y + rect.height);
	#endif
}

// portable, based on wxRendererGeneric::DrawComboBoxDropButton
void draw_menu_arrow(Window* win, DC& dc, const wxRect& rect, bool active) {
	wxPoint pt[] =
		{	wxPoint(0, 0)
		,	wxPoint(4, 4)
		,	wxPoint(0, 8)
		};
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxSystemSettings::GetColour(active ? wxSYS_COLOUR_HIGHLIGHTTEXT : wxSYS_COLOUR_WINDOWTEXT));
	dc.DrawPolygon(3, pt, rect.x + rect.width - 6, rect.y + (rect.height - 9) / 2);
}

void draw_drop_down_arrow(Window* win, DC& dc, const wxRect& rect, bool active) {
	wxRendererNative& rn = wxRendererNative::GetDefault();
	int w = wxSystemSettings::GetMetric(wxSYS_VSCROLL_ARROW_X); // drop down arrow is same size
	if (w == -1) {
		w = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X); // Try just the scrollbar, then.
	}
	rn.DrawComboBoxDropButton(win, dc, 
		wxRect(rect.x + rect.width - w, rect.y, w, rect.height)
		, active ? wxCONTROL_PRESSED : 0);
}

void draw_checkbox(Window* win, DC& dc, const wxRect& rect, bool checked, bool enabled) {
	#if wxUSE_UXTHEME && defined(__WXMSW__)
		// TODO: Windows version?
	#endif
	// portable version
	if (checked) {
		dc.DrawCheckMark(wxRect(rect.x-1,rect.y-1,rect.width+2,rect.height+2));
	}
	dc.SetPen(wxSystemSettings::GetColour(enabled ? wxSYS_COLOUR_WINDOWTEXT: wxSYS_COLOUR_GRAYTEXT));
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
}

void draw_radiobox(Window* win, DC& dc, const wxRect& rect, bool checked, bool enabled) {
	#if wxUSE_UXTHEME && defined(__WXMSW__)
		// TODO: Windows version?
	#endif
	// portable version
	#if 1
		// circle drawing on windows looks absolutely horrible
		// so use rounded rectangles instead
		dc.SetPen(wxSystemSettings::GetColour(enabled ? wxSYS_COLOUR_WINDOWTEXT: wxSYS_COLOUR_GRAYTEXT));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		//dc.DrawEllipse(rect.x, rect.y, rect.width, rect.height);
		dc.DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, rect.width*0.5-1);
		if (checked) {
			dc.SetBrush(wxSystemSettings::GetColour(enabled ? wxSYS_COLOUR_WINDOWTEXT: wxSYS_COLOUR_GRAYTEXT));
			dc.SetPen(*wxTRANSPARENT_PEN);
			//dc.DrawEllipse(rect.x+2,rect.y+2,rect.width-4,rect.height-4);
			dc.DrawRoundedRectangle(rect.x+3, rect.y+3, rect.width-6, rect.height-6, rect.width*0.5-4);
		}
	#endif
}