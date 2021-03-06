// -*- C++ -*- generated by wxGlade 0.6.3 on Sat Sep  6 18:07:53 2008

#include <wx/wx.h>
#include <wx/image.h>
#ifdef wxUSE_DRAG_AND_DROP
#include <wx/dnd.h>
#endif
// begin wxGlade: ::dependencies
// end wxGlade


#ifndef ATQUE_H
#define ATQUE_H


// begin wxGlade: ::extracode
// end wxGlade


class AtqueWindow: public wxFrame {
public:
    // begin wxGlade: AtqueWindow::ids
    enum {
        MENU_Split = wxID_HIGHEST + 1002,
        MENU_Merge = wxID_HIGHEST + 1003
    };
    // end wxGlade

    AtqueWindow(wxWindow* parent, int id, const wxString& title, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize, long style=wxDEFAULT_FRAME_STYLE);

private:
    // begin wxGlade: AtqueWindow::methods
    void set_properties();
    void do_layout();
    // end wxGlade

protected:
    // begin wxGlade: AtqueWindow::attributes
    wxMenuBar* menuBar;
    wxStaticText* instructions;
    wxPanel* panel_1;
    // end wxGlade

    DECLARE_EVENT_TABLE();

public:
    virtual void OnAbout(wxCommandEvent &event); // wxGlade: <event_handler>
    virtual void OnMerge(wxCommandEvent &event); // wxGlade: <event_handler>
    virtual void OnSplit(wxCommandEvent &event); // wxGlade: <event_handler>
    virtual void OnExit(wxCommandEvent &event); // wxGlade: <event_handler>
    virtual void Split(const wxString& file);
    virtual void Merge(const wxString& folder);
}; // wxGlade: end class

#if wxUSE_DRAG_AND_DROP
class AtqueDnD : public wxFileDropTarget
{
public:
	AtqueDnD(AtqueWindow *window) { window_ = window; }
	virtual bool OnDropFiles(wxCoord x, wxCoord y,
				 const wxArrayString& filenames);
private:
	AtqueWindow* window_;
};
#endif

#endif // ATQUE_H
