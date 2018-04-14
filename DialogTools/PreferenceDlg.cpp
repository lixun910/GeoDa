/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <wx/wx.h>
#include <wx/statbmp.h>
#include <wx/filedlg.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/checkbox.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>
#include <wx/hyperlink.h>
#include <wx/tokenzr.h>
#include <wx/stdpaths.h>
#include <wx/progdlg.h>
#include <wx/panel.h>
#include <wx/textfile.h>
#include <wx/regex.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/uri.h>
#include <wx/slider.h>
#include <wx/combobox.h>
#include <wx/notebook.h>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_writer.h>
#include "curl/curl.h"
#include "ogrsf_frmts.h"
#include "cpl_conv.h"

#include "../HLStateInt.h"
#include "../HighlightStateObserver.h"
#include "../ShapeOperations/OGRDataAdapter.h"
#include "../GeneralWxUtils.h"
#include "../GenUtils.h"
#include "../GdaConst.h"
#include "../GdaJson.h"
#include "../logger.h"
#include "PreferenceDlg.h"

using namespace std;
using namespace GdaJson;

////////////////////////////////////////////////////////////////////////////////
//
// PreferenceDlg
//
////////////////////////////////////////////////////////////////////////////////
PreferenceDlg::PreferenceDlg(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
	highlight_state = NULL;
	SetBackgroundColour(*wxWHITE);
	Init();
    SetMinSize(wxSize(550, -1));
}

PreferenceDlg::PreferenceDlg(wxWindow* parent,
	HLStateInt* _highlight_state,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size)
	: wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
	highlight_state = _highlight_state;
	SetBackgroundColour(*wxWHITE);
	Init();
    SetMinSize(wxSize(550, -1));
}

void PreferenceDlg::Init()
{
	ReadFromCache();

	wxNotebook* notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	//  visualization tab
	wxNotebookPage* vis_page = new wxNotebookPage(notebook, -1, wxDefaultPosition, wxSize(560, 620));
#ifdef __WIN32__
	vis_page->SetBackgroundColour(*wxWHITE);
#endif
	notebook->AddPage(vis_page, _("System"));
	wxFlexGridSizer* grid_sizer1 = new wxFlexGridSizer(20, 2, 8, 10);

	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Maps:")), 1);
	grid_sizer1->AddSpacer(10);

	wxString lbl0 = _("Use classic yellow cross-hatching to highlight selection in maps:");
	wxStaticText* lbl_txt0 = new wxStaticText(vis_page, wxID_ANY, lbl0);
	cbox0 = new wxCheckBox(vis_page, XRCID("PREF_USE_CROSSHATCH"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt0, 1, wxEXPAND);
	grid_sizer1->Add(cbox0, 0, wxALIGN_RIGHT);
	cbox0->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnCrossHatch, this);

	wxSize sl_sz(200, -1);
	wxSize txt_sz(35, -1);

	wxString lbl1 = _("Set transparency of highlighted objects in selection:");
	wxStaticText* lbl_txt1 = new wxStaticText(vis_page, wxID_ANY, lbl1);
	wxBoxSizer* box1 = new wxBoxSizer(wxHORIZONTAL);
	slider1 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt1 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER1_TXT"), "",
		wxDefaultPosition, txt_sz, wxTE_READONLY);
	box1->Add(slider1);
	box1->Add(slider_txt1);
	grid_sizer1->Add(lbl_txt1, 1, wxEXPAND);
	grid_sizer1->Add(box1, 0, wxALIGN_RIGHT);
	slider1->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider1, this);

	wxString lbl2 = _("Set transparency of unhighlighted objects in selection:");
	wxStaticText* lbl_txt2 = new wxStaticText(vis_page, wxID_ANY, lbl2);
	wxBoxSizer* box2 = new wxBoxSizer(wxHORIZONTAL);
	slider2 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt2 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER2_TXT"), "",
		wxDefaultPosition, txt_sz, wxTE_READONLY);
	box2->Add(slider2);
	box2->Add(slider_txt2);
	grid_sizer1->Add(lbl_txt2, 1, wxEXPAND);
	grid_sizer1->Add(box2, 0, wxALIGN_RIGHT);
	slider2->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider2, this);

	wxString lbl3 = _("Add basemap automatically:");
	wxStaticText* lbl_txt3 = new wxStaticText(vis_page, wxID_ANY, lbl3);
	//wxStaticText* lbl_txt33 = new wxStaticText(vis_page, wxID_ANY, lbl3);
	cmb33 = new wxComboBox(vis_page, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
	cmb33->Append("No basemap");
	cmb33->Append("Carto Light");
	cmb33->Append("Carto Dark");
	cmb33->Append("Carto Light (No Labels)");
	cmb33->Append("Carto Dark (No Labels)");
	cmb33->Append("Nokia Day");
	cmb33->Append("Nokia Night");
	cmb33->Append("Nokia Hybrid");
	cmb33->Append("Nokia Satellite");
	cmb33->SetSelection(0);
	cmb33->Bind(wxEVT_COMBOBOX, &PreferenceDlg::OnChoice3, this);

	grid_sizer1->Add(lbl_txt3, 1, wxEXPAND);
	grid_sizer1->Add(cmb33, 0, wxALIGN_RIGHT);

	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Plots:")), 1, wxTOP | wxBOTTOM, 10);
	grid_sizer1->AddSpacer(10);

	wxString lbl6 = _("Set transparency of highlighted objects in selection:");
	wxStaticText* lbl_txt6 = new wxStaticText(vis_page, wxID_ANY, lbl6);
	wxBoxSizer* box6 = new wxBoxSizer(wxHORIZONTAL);
	slider6 = new wxSlider(vis_page, XRCID("PREF_SLIDER6"),
		255, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	wxTextCtrl* slider_txt6 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER6_TXT"), "0.0", wxDefaultPosition, txt_sz, wxTE_READONLY);
	lbl_txt6->Hide();
	slider6->Hide();
	slider_txt6->Hide();

	box6->Add(slider6);
	box6->Add(slider_txt6);
	grid_sizer1->Add(lbl_txt6, 1, wxEXPAND);
	grid_sizer1->Add(box6, 0, wxALIGN_RIGHT);
	slider6->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider6, this);
	slider6->Enable(false);

	wxString lbl7 = _("Set transparency of unhighlighted objects in selection:");
	wxStaticText* lbl_txt7 = new wxStaticText(vis_page, wxID_ANY, lbl7);
	wxBoxSizer* box7 = new wxBoxSizer(wxHORIZONTAL);
	slider7 = new wxSlider(vis_page, wxID_ANY,
		0, 0, 255,
		wxDefaultPosition, sl_sz,
		wxSL_HORIZONTAL);
	slider_txt7 = new wxTextCtrl(vis_page, XRCID("PREF_SLIDER7_TXT"), "", wxDefaultPosition, txt_sz, wxTE_READONLY);
	box7->Add(slider7);
	box7->Add(slider_txt7);
	grid_sizer1->Add(lbl_txt7, 1, wxEXPAND);
	grid_sizer1->Add(box7, 0, wxALIGN_RIGHT);
	slider7->Bind(wxEVT_SLIDER, &PreferenceDlg::OnSlider7, this);


	grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("System:")), 1,
		wxTOP | wxBOTTOM, 10);
	grid_sizer1->AddSpacer(10);


    wxString lbl113 = _("Language:");
    wxStaticText* lbl_txt113 = new wxStaticText(vis_page, wxID_ANY, lbl113);
    cmb113 = new wxComboBox(vis_page, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);
    cmb113->Append("English");
    cmb113->Append("Chinese");
    cmb113->Bind(wxEVT_COMBOBOX, &PreferenceDlg::OnChooseLanguage, this);
    
    grid_sizer1->Add(lbl_txt113, 1, wxEXPAND);
    grid_sizer1->Add(cmb113, 0, wxALIGN_RIGHT);
    
	wxString lbl8 = _("Show Recent/Sample Data panel in Connect Datasource Dialog:");
	wxStaticText* lbl_txt8 = new wxStaticText(vis_page, wxID_ANY, lbl8);
	cbox8 = new wxCheckBox(vis_page, XRCID("PREF_SHOW_RECENT"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt8, 1, wxEXPAND);
	grid_sizer1->Add(cbox8, 0, wxALIGN_RIGHT);
	cbox8->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnShowRecent, this);

	wxString lbl9 = _("Show CSV Configuration in Merge Data Dialog:");
	wxStaticText* lbl_txt9 = new wxStaticText(vis_page, wxID_ANY, lbl9);
	cbox9 = new wxCheckBox(vis_page, XRCID("PREF_SHOW_CSV_IN_MERGE"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt9, 1, wxEXPAND);
	grid_sizer1->Add(cbox9, 0, wxALIGN_RIGHT);
	cbox9->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnShowCsvInMerge, this);

	wxString lbl10 = _("Enable High DPI/Retina support (Mac only):");
	wxStaticText* lbl_txt10 = new wxStaticText(vis_page, wxID_ANY, lbl10);
	cbox10 = new wxCheckBox(vis_page, XRCID("PREF_ENABLE_HDPI"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt10, 1, wxEXPAND);
	grid_sizer1->Add(cbox10, 0, wxALIGN_RIGHT);
	cbox10->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnEnableHDPISupport, this);

	wxString lbl4 = _("Disable crash detection for bug report:");
	wxStaticText* lbl_txt4 = new wxStaticText(vis_page, wxID_ANY, lbl4);
	cbox4 = new wxCheckBox(vis_page, XRCID("PREF_CRASH_DETECT"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt4, 1, wxEXPAND);
	grid_sizer1->Add(cbox4, 0, wxALIGN_RIGHT);
	cbox4->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDisableCrashDetect, this);

	wxString lbl5 = _("Disable auto upgrade:");
	wxStaticText* lbl_txt5 = new wxStaticText(vis_page, wxID_ANY, lbl5);
	cbox5 = new wxCheckBox(vis_page, XRCID("PREF_AUTO_UPGRADE"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt5, 1, wxEXPAND);
	grid_sizer1->Add(cbox5, 0, wxALIGN_RIGHT);
	cbox5->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnDisableAutoUpgrade, this);

    grid_sizer1->Add(new wxStaticText(vis_page, wxID_ANY, _("Method:")), 1,
                     wxTOP | wxBOTTOM, 10);
    grid_sizer1->AddSpacer(10);
    
	wxString lbl16 = _("Use specified seed:");
	wxStaticText* lbl_txt16 = new wxStaticText(vis_page, wxID_ANY, lbl16);
	cbox6 = new wxCheckBox(vis_page, XRCID("PREF_USE_SPEC_SEED"), "", wxDefaultPosition);
	grid_sizer1->Add(lbl_txt16, 1, wxEXPAND);
	grid_sizer1->Add(cbox6, 0, wxALIGN_RIGHT);
	cbox6->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnUseSpecifiedSeed, this);
   
	wxString lbl17 = _("Set seed for randomization:");
	wxStaticText* lbl_txt17 = new wxStaticText(vis_page, wxID_ANY, lbl17);
	txt_seed = new wxTextCtrl(vis_page, XRCID("PREF_SEED_VALUE"), "", wxDefaultPosition, wxSize(85, -1));
	grid_sizer1->Add(lbl_txt17, 1, wxEXPAND);
	grid_sizer1->Add(txt_seed, 0, wxALIGN_RIGHT);
    txt_seed->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnSeedEnter, this);

	wxString lbl18 = _("Set number of CPU cores manually:");
	wxStaticText* lbl_txt18 = new wxStaticText(vis_page, wxID_ANY, lbl18);
    wxBoxSizer* box18 = new wxBoxSizer(wxHORIZONTAL);
	cbox18 = new wxCheckBox(vis_page, XRCID("PREF_SET_CPU_CORES"), "", wxDefaultPosition);
	txt_cores = new wxTextCtrl(vis_page, XRCID("PREF_TXT_CPU_CORES"), "", wxDefaultPosition, wxSize(85, -1));
    box18->Add(cbox18);
    box18->Add(txt_cores);
	grid_sizer1->Add(lbl_txt18, 1, wxEXPAND);
	grid_sizer1->Add(box18, 0, wxALIGN_RIGHT);
	cbox18->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnSetCPUCores, this);
    txt_cores->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnCPUCoresEnter, this);
    
	wxString lbl19 = _("Stopping criterion for power iteration:");
	wxStaticText* lbl_txt19 = new wxStaticText(vis_page, wxID_ANY, lbl19);
	txt_poweriter_eps = new wxTextCtrl(vis_page, XRCID("PREF_POWER_EPS"), "", wxDefaultPosition, wxSize(85, -1));
	grid_sizer1->Add(lbl_txt19, 1, wxEXPAND);
	grid_sizer1->Add(txt_poweriter_eps, 0, wxALIGN_RIGHT);
    txt_poweriter_eps->Bind(wxEVT_COMMAND_TEXT_UPDATED, &PreferenceDlg::OnPowerEpsEnter, this);
    
	grid_sizer1->AddGrowableCol(0, 1);

	wxBoxSizer *nb_box1 = new wxBoxSizer(wxVERTICAL);
	nb_box1->Add(grid_sizer1, 1, wxEXPAND | wxALL, 20);
	nb_box1->Fit(vis_page);

	vis_page->SetSizer(nb_box1);

	//------------------------------------
	//  datasource (gdal) tab
	wxNotebookPage* gdal_page = new wxNotebookPage(notebook, -1);
#ifdef __WIN32__
	gdal_page->SetBackgroundColour(*wxWHITE);
#endif
	notebook->AddPage(gdal_page, "Data Source");
	wxFlexGridSizer* grid_sizer2 = new wxFlexGridSizer(10, 2, 8, 10);

	wxString lbl21 = _("Hide system table in Postgresql connection:");
	wxStaticText* lbl_txt21 = new wxStaticText(gdal_page, wxID_ANY, lbl21);
	cbox21 = new wxCheckBox(gdal_page, wxID_ANY, "", wxDefaultPosition);
	grid_sizer2->Add(lbl_txt21, 1, wxEXPAND | wxTOP, 10);
	grid_sizer2->Add(cbox21, 0, wxALIGN_RIGHT | wxTOP, 13);
	cbox21->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnHideTablePostGIS, this);


	wxString lbl22 = _("Hide system table in SQLITE connection:");
	wxStaticText* lbl_txt22 = new wxStaticText(gdal_page, wxID_ANY, lbl22);
	cbox22 = new wxCheckBox(gdal_page, wxID_ANY, "", wxDefaultPosition);
	grid_sizer2->Add(lbl_txt22, 1, wxEXPAND);
	grid_sizer2->Add(cbox22, 0, wxALIGN_RIGHT);
	cbox22->Bind(wxEVT_CHECKBOX, &PreferenceDlg::OnHideTableSQLITE, this);

    
	wxString lbl23 = _("Http connection timeout (seconds) for e.g. WFS, Geojson etc.:");
	wxStaticText* lbl_txt23 = new wxStaticText(gdal_page, wxID_ANY, lbl23);
	txt23 = new wxTextCtrl(gdal_page, XRCID("ID_HTTP_TIMEOUT"), "", wxDefaultPosition, txt_sz, wxTE_PROCESS_ENTER);
	grid_sizer2->Add(lbl_txt23, 1, wxEXPAND);
	grid_sizer2->Add(txt23, 0, wxALIGN_RIGHT);
	txt23->Bind(wxEVT_TEXT, &PreferenceDlg::OnTimeoutInput, this);
   
	wxString lbl24 = _("Date/Time formats (using comma to separate formats):");
	wxStaticText* lbl_txt24 = new wxStaticText(gdal_page, wxID_ANY, lbl24);
	txt24 = new wxTextCtrl(gdal_page, XRCID("ID_DATETIME_FORMATS"), "", wxDefaultPosition, wxSize(200, -1), wxTE_PROCESS_ENTER);
	grid_sizer2->Add(lbl_txt24, 1, wxEXPAND);
	grid_sizer2->Add(txt24, 0, wxALIGN_RIGHT);
	txt24->Bind(wxEVT_TEXT, &PreferenceDlg::OnDateTimeInput, this);
    
	grid_sizer2->AddGrowableCol(0, 1);

	wxBoxSizer *nb_box2 = new wxBoxSizer(wxVERTICAL);
	nb_box2->Add(grid_sizer2, 1, wxEXPAND | wxALL, 20);
	nb_box2->Fit(gdal_page);

	gdal_page->SetSizer(nb_box2);

	SetupControls();

	// overall

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

	wxButton *resetButton = new wxButton(this, -1, _("Reset"), wxDefaultPosition, wxSize(70, 30));
	wxButton *closeButton = new wxButton(this, wxID_OK, _("Close"), wxDefaultPosition, wxSize(70, 30));
	resetButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &PreferenceDlg::OnReset, this);

	hbox->Add(resetButton, 1);
	hbox->Add(closeButton, 1, wxLEFT, 5);

	vbox->Add(notebook, 1, wxEXPAND | wxALL, 10);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);

	SetSizer(vbox);
	vbox->Fit(this);
    
	Centre();
	ShowModal();

	Destroy();
}

void PreferenceDlg::OnReset(wxCommandEvent& ev)
{
    GdaConst::gda_ui_language = 0;
    GdaConst::gda_eigen_tol = 1.0E-8;
	GdaConst::gda_set_cpu_cores = true;
	GdaConst::gda_cpu_cores = 8;
	GdaConst::use_cross_hatching = false;
	GdaConst::transparency_highlighted = 255;
	GdaConst::transparency_unhighlighted = 100;
	//GdaConst::transparency_map_on_basemap = 200;
	GdaConst::use_basemap_by_default = false;
	GdaConst::default_basemap_selection = 0;
	GdaConst::hide_sys_table_postgres = false;
	GdaConst::hide_sys_table_sqlite = false;
	GdaConst::disable_crash_detect = false;
	GdaConst::disable_auto_upgrade = false;
	GdaConst::plot_transparency_highlighted = 255;
	GdaConst::plot_transparency_unhighlighted = 50;
	GdaConst::show_recent_sample_connect_ds_dialog = true;
	GdaConst::show_csv_configure_in_merge = false;
	GdaConst::enable_high_dpi_support = true;
    GdaConst::gdal_http_timeout = 5;
    GdaConst::use_gda_user_seed= true;
    GdaConst::gda_user_seed = 123456789;
    GdaConst::gda_datetime_formats_str = "%Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S,%m/%d/%Y %H:%M:%S,%Y-%m-%d,%m/%d/%Y,%Y/%m/%d,%H:%M:%S";
    if (!GdaConst::gda_datetime_formats_str.empty()) {
        wxString patterns = GdaConst::gda_datetime_formats_str;
        wxStringTokenizer tokenizer(patterns, ",");
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken();
            GdaConst::gda_datetime_formats.push_back(token);
        }
        GdaConst::gda_datetime_formats_str = patterns;
    }

	SetupControls();

	OGRDataAdapter& ogr_adapt = OGRDataAdapter::GetInstance();
	ogr_adapt.AddEntry("use_cross_hatching", "0");
	ogr_adapt.AddEntry("transparency_highlighted", "255");
	ogr_adapt.AddEntry("transparency_unhighlighted", "100");
	ogr_adapt.AddEntry("use_basemap_by_default", "0");
	ogr_adapt.AddEntry("default_basemap_selection", "0");
	ogr_adapt.AddEntry("hide_sys_table_postgres", "0");
	ogr_adapt.AddEntry("hide_sys_table_sqlite", "0");
	ogr_adapt.AddEntry("disable_crash_detect", "0");
	ogr_adapt.AddEntry("disable_auto_upgrade", "0");
	ogr_adapt.AddEntry("plot_transparency_highlighted", "255");
	ogr_adapt.AddEntry("plot_transparency_unhighlighted", "50");
	ogr_adapt.AddEntry("show_recent_sample_connect_ds_dialog", "1");
	ogr_adapt.AddEntry("show_csv_configure_in_merge", "0");
	ogr_adapt.AddEntry("enable_high_dpi_support", "1");
	ogr_adapt.AddEntry("gdal_http_timeout", "5");
	ogr_adapt.AddEntry("use_gda_user_seed", "1");
	ogr_adapt.AddEntry("gda_user_seed", "123456789");
	ogr_adapt.AddEntry("gda_datetime_formats_str", "%Y-%m-%d %H:%M:%S,%Y/%m/%d %H:%M:%S,%d.%m.%Y %H:%M:%S,%m/%d/%Y %H:%M:%S,%Y-%m-%d,%m/%d/%Y,%Y/%m/%d,%H:%M:%S");
	ogr_adapt.AddEntry("gda_cpu_cores", "8");
	ogr_adapt.AddEntry("gda_set_cpu_cores", "1");
	ogr_adapt.AddEntry("gda_eigen_tol", "1.0E-8");
    ogr_adapt.AddEntry("gda_ui_language", "0");
}

void PreferenceDlg::SetupControls()
{
	cbox0->SetValue(GdaConst::use_cross_hatching);
	slider1->SetValue(GdaConst::transparency_highlighted);
	wxString t_hl = wxString::Format("%.2f", (255 - GdaConst::transparency_highlighted) / 255.0);
	slider_txt1->SetValue(t_hl);
	slider2->SetValue(GdaConst::transparency_unhighlighted);
	wxString t_uhl = wxString::Format("%.2f", (255 - GdaConst::transparency_unhighlighted) / 255.0);
	slider_txt2->SetValue(t_uhl);
	if (GdaConst::use_basemap_by_default) {
		cmb33->SetSelection(GdaConst::default_basemap_selection);
	}
	else {
		cmb33->SetSelection(0);
	}

	slider7->SetValue(GdaConst::plot_transparency_unhighlighted);
	wxString t_p_hl = wxString::Format("%.2f", (255 - GdaConst::plot_transparency_unhighlighted) / 255.0);
	slider_txt7->SetValue(t_p_hl);

	cbox4->SetValue(GdaConst::disable_crash_detect);
	cbox5->SetValue(GdaConst::disable_auto_upgrade);
	cbox21->SetValue(GdaConst::hide_sys_table_postgres);
	cbox22->SetValue(GdaConst::hide_sys_table_sqlite);
	cbox8->SetValue(GdaConst::show_recent_sample_connect_ds_dialog);
	cbox9->SetValue(GdaConst::show_csv_configure_in_merge);
	cbox10->SetValue(GdaConst::enable_high_dpi_support);
    
    txt23->SetValue(wxString::Format("%d", GdaConst::gdal_http_timeout));
    txt24->SetValue(GdaConst::gda_datetime_formats_str);
    
    cbox6->SetValue(GdaConst::use_gda_user_seed);
    wxString t_seed;
    t_seed << GdaConst::gda_user_seed;
    txt_seed->SetValue(t_seed);
    
    cbox18->SetValue(GdaConst::gda_set_cpu_cores);
    wxString t_cores;
    t_cores << GdaConst::gda_cpu_cores;
    txt_cores->SetValue(t_cores);
    
    wxString t_power_eps;
    t_power_eps << GdaConst::gda_eigen_tol;
    txt_poweriter_eps->SetValue(t_power_eps);
    
    cmb113->SetSelection(GdaConst::gda_ui_language);
}

void PreferenceDlg::ReadFromCache()
{
	vector<string> transp_h = OGRDataAdapter::GetInstance().GetHistory("transparency_highlighted");
	if (!transp_h.empty()) {
		long transp_l = 0;
		wxString transp = transp_h[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::transparency_highlighted = transp_l;
		}
	}
	vector<string> transp_uh = OGRDataAdapter::GetInstance().GetHistory("transparency_unhighlighted");
	if (!transp_uh.empty()) {
		long transp_l = 0;
		wxString transp = transp_uh[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::transparency_unhighlighted = transp_l;
		}
	}
	vector<string> plot_transparency_unhighlighted = OGRDataAdapter::GetInstance().GetHistory("plot_transparency_unhighlighted");
	if (!plot_transparency_unhighlighted.empty()) {
		long transp_l = 0;
		wxString transp = plot_transparency_unhighlighted[0];
		if (transp.ToLong(&transp_l)) {
			GdaConst::plot_transparency_unhighlighted = transp_l;
		}
	}
	vector<string> basemap_sel = OGRDataAdapter::GetInstance().GetHistory("default_basemap_selection");
	if (!basemap_sel.empty()) {
		long sel_l = 0;
		wxString sel = basemap_sel[0];
		if (sel.ToLong(&sel_l)) {
			GdaConst::default_basemap_selection = sel_l;
		}
	}
	vector<string> basemap_default = OGRDataAdapter::GetInstance().GetHistory("use_basemap_by_default");
	if (!basemap_default.empty()) {
		long sel_l = 0;
		wxString sel = basemap_default[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::use_basemap_by_default = true;
			else if (sel_l == 0)
				GdaConst::use_basemap_by_default = false;
		}
	}
	vector<string> crossht_sel = OGRDataAdapter::GetInstance().GetHistory("use_cross_hatching");
	if (!crossht_sel.empty()) {
		long cross_l = 0;
		wxString cross = crossht_sel[0];
		if (cross.ToLong(&cross_l)) {
			if (cross_l == 1)
				GdaConst::use_cross_hatching = true;
			else if (cross_l == 0)
				GdaConst::use_cross_hatching = false;
		}
	}
	vector<string> postgres_sys_sel = OGRDataAdapter::GetInstance().GetHistory("hide_sys_table_postgres");
	if (!postgres_sys_sel.empty()) {
		long sel_l = 0;
		wxString sel = postgres_sys_sel[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::hide_sys_table_postgres = true;
			else if (sel_l == 0)
				GdaConst::hide_sys_table_postgres = false;
		}
	}
	vector<string> hide_sys_table_sqlite = OGRDataAdapter::GetInstance().GetHistory("hide_sys_table_sqlite");
	if (!hide_sys_table_sqlite.empty()) {
		long sel_l = 0;
		wxString sel = hide_sys_table_sqlite[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::hide_sys_table_sqlite = true;
			else if (sel_l == 0)
				GdaConst::hide_sys_table_sqlite = false;
		}
	}
	vector<string> disable_crash_detect = OGRDataAdapter::GetInstance().GetHistory("disable_crash_detect");
	if (!disable_crash_detect.empty()) {
		long sel_l = 0;
		wxString sel = disable_crash_detect[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::disable_crash_detect = true;
			else if (sel_l == 0)
				GdaConst::disable_crash_detect = false;
		}
	}
	vector<string> disable_auto_upgrade = OGRDataAdapter::GetInstance().GetHistory("disable_auto_upgrade");
	if (!disable_auto_upgrade.empty()) {
		long sel_l = 0;
		wxString sel = disable_auto_upgrade[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::disable_auto_upgrade = true;
			else if (sel_l == 0)
				GdaConst::disable_auto_upgrade = false;
		}
	}

	vector<string> show_recent_sample_connect_ds_dialog = OGRDataAdapter::GetInstance().GetHistory("show_recent_sample_connect_ds_dialog");
	if (!show_recent_sample_connect_ds_dialog.empty()) {
		long sel_l = 0;
		wxString sel = show_recent_sample_connect_ds_dialog[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::show_recent_sample_connect_ds_dialog = true;
			else if (sel_l == 0)
				GdaConst::show_recent_sample_connect_ds_dialog = false;
		}
	}

	vector<string> show_csv_configure_in_merge = OGRDataAdapter::GetInstance().GetHistory("show_csv_configure_in_merge");
	if (!show_csv_configure_in_merge.empty()) {
		long sel_l = 0;
		wxString sel = show_csv_configure_in_merge[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::show_csv_configure_in_merge = true;
			else if (sel_l == 0)
				GdaConst::show_csv_configure_in_merge = false;
		}
	}
	vector<string> enable_high_dpi_support = OGRDataAdapter::GetInstance().GetHistory("enable_high_dpi_support");
	if (!enable_high_dpi_support.empty()) {
		long sel_l = 0;
		wxString sel = enable_high_dpi_support[0];
		if (sel.ToLong(&sel_l)) {
			if (sel_l == 1)
				GdaConst::enable_high_dpi_support = true;
			else if (sel_l == 0)
				GdaConst::enable_high_dpi_support = false;
		}
	}
	vector<string> gdal_http_timeout = OGRDataAdapter::GetInstance().GetHistory("gdal_http_timeout");
	if (!gdal_http_timeout.empty()) {
		long sel_l = 0;
		wxString sel = gdal_http_timeout[0];
		if (sel.ToLong(&sel_l)) {
            GdaConst::gdal_http_timeout = sel_l;
		}
	}
    
    vector<string> gda_datetime_formats_str = OGRDataAdapter::GetInstance().GetHistory("gda_datetime_formats_str");
    if (!gda_datetime_formats_str.empty()) {
        wxString patterns = gda_datetime_formats_str[0];
        wxStringTokenizer tokenizer(patterns, ",");
        while ( tokenizer.HasMoreTokens() )
        {
            wxString token = tokenizer.GetNextToken();
            GdaConst::gda_datetime_formats.push_back(token);
        }
        GdaConst::gda_datetime_formats_str = patterns;
    }
    
    vector<string> gda_user_seed = OGRDataAdapter::GetInstance().GetHistory("gda_user_seed");
    if (!gda_user_seed.empty()) {
        long sel_l = 0;
        wxString sel = gda_user_seed[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_user_seed = sel_l;
        }
    }
    vector<string> use_gda_user_seed = OGRDataAdapter::GetInstance().GetHistory("use_gda_user_seed");
    if (!use_gda_user_seed.empty()) {
        long sel_l = 0;
        wxString sel = use_gda_user_seed[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
                GdaConst::use_gda_user_seed = true;
            else if (sel_l == 0)
                GdaConst::use_gda_user_seed = false;
        }
    }
    
    vector<string> gda_set_cpu_cores = OGRDataAdapter::GetInstance().GetHistory("gda_set_cpu_cores");
    if (!gda_set_cpu_cores.empty()) {
        long sel_l = 0;
        wxString sel = gda_set_cpu_cores[0];
        if (sel.ToLong(&sel_l)) {
            if (sel_l == 1)
                GdaConst::gda_set_cpu_cores = true;
            else if (sel_l == 0)
                GdaConst::gda_set_cpu_cores = false;
        }
    }
    vector<string> gda_cpu_cores = OGRDataAdapter::GetInstance().GetHistory("gda_cpu_cores");
    if (!gda_cpu_cores.empty()) {
        long sel_l = 0;
        wxString sel = gda_cpu_cores[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_cpu_cores = sel_l;
        }
    }
    
    vector<string> gda_eigen_tol = OGRDataAdapter::GetInstance().GetHistory("gda_eigen_tol");
    if (!gda_eigen_tol.empty()) {
        double sel_l = 0;
        wxString sel = gda_eigen_tol[0];
        if (sel.ToDouble(&sel_l)) {
            GdaConst::gda_eigen_tol = sel_l;
        }
    }
    
    vector<string> gda_ui_language = OGRDataAdapter::GetInstance().GetHistory("gda_ui_language");
    if (!gda_ui_language.empty()) {
        long sel_l = 0;
        wxString sel = gda_ui_language[0];
        if (sel.ToLong(&sel_l)) {
            GdaConst::gda_ui_language = sel_l;
        }
    }
    
    // following are not in this UI, but still global variable
    vector<string> gda_user_email = OGRDataAdapter::GetInstance().GetHistory("gda_user_email");
    if (!gda_user_email.empty()) {
        wxString email = gda_user_email[0];
        GdaConst::gda_user_email = email;
    }
    
}

void PreferenceDlg::OnChooseLanguage(wxCommandEvent& ev)
{
    int lan_sel = ev.GetSelection();
    GdaConst::gda_ui_language = lan_sel;
    wxString sel_str;
    sel_str << GdaConst::gda_ui_language;
    OGRDataAdapter::GetInstance().AddEntry("gda_ui_language", sel_str.ToStdString());
    wxString msg = _("Please restart GeoDa to apply the language setup.");
    wxMessageDialog dlg(NULL, msg, _("Info"), wxOK | wxICON_ERROR);
    dlg.ShowModal();
}
void PreferenceDlg::OnDateTimeInput(wxCommandEvent& ev)
{
    GdaConst::gda_datetime_formats.clear();
    wxString formats_str = txt24->GetValue();
    wxStringTokenizer tokenizer(formats_str, ",");
    while ( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        GdaConst::gda_datetime_formats.push_back(token);
    }
    GdaConst::gda_datetime_formats_str = formats_str;
    OGRDataAdapter::GetInstance().AddEntry("gda_datetime_formats_str", formats_str.ToStdString());
}

void PreferenceDlg::OnTimeoutInput(wxCommandEvent& ev)
{
    wxString sec_str = txt23->GetValue();
    long sec;
    if (sec_str.ToLong(&sec)) {
        if (sec >= 0) {
            GdaConst::gdal_http_timeout = sec;
            OGRDataAdapter::GetInstance().AddEntry("gdal_http_timeout", sec_str.ToStdString());
            CPLSetConfigOption("GDAL_HTTP_TIMEOUT", sec_str);
        }
    }
}

void PreferenceDlg::OnSlider1(wxCommandEvent& ev)
{
	int val = slider1->GetValue();
	GdaConst::transparency_highlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("transparency_highlighted", transp_str.ToStdString());
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER1_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}
void PreferenceDlg::OnSlider2(wxCommandEvent& ev)
{
	int val = slider2->GetValue();
	GdaConst::transparency_unhighlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("transparency_unhighlighted", transp_str.ToStdString());
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER2_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}
void PreferenceDlg::OnSlider6(wxCommandEvent& ev)
{
	int val = slider6->GetValue();
	GdaConst::plot_transparency_highlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("plot_transparency_highlighted", transp_str.ToStdString());
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER6_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}
void PreferenceDlg::OnSlider7(wxCommandEvent& ev)
{
	int val = slider7->GetValue();
	GdaConst::plot_transparency_unhighlighted = val;
	wxString transp_str;
	transp_str << val;
	OGRDataAdapter::GetInstance().AddEntry("plot_transparency_unhighlighted", transp_str.ToStdString());
	wxTextCtrl* txt_ctl = wxDynamicCast(FindWindow(XRCID("PREF_SLIDER7_TXT")), wxTextCtrl);

	wxString t_hl = wxString::Format("%.2f", (255 - val) / 255.0);
	txt_ctl->SetValue(t_hl);

	if (highlight_state) {
		highlight_state->SetEventType(HLStateInt::transparency);
		highlight_state->notifyObservers();
	}
}

void PreferenceDlg::OnChoice3(wxCommandEvent& ev)
{
	int basemap_sel = ev.GetSelection();
	if (basemap_sel <= 0) {
		GdaConst::use_basemap_by_default = false;
		OGRDataAdapter::GetInstance().AddEntry("use_basemap_by_default", "0");
	}
	else {
		GdaConst::use_basemap_by_default = true;
		GdaConst::default_basemap_selection = basemap_sel;
		wxString sel_str;
		sel_str << GdaConst::default_basemap_selection;
		OGRDataAdapter::GetInstance().AddEntry("use_basemap_by_default", "1");
		OGRDataAdapter::GetInstance().AddEntry("default_basemap_selection", sel_str.ToStdString());
	}
}

void PreferenceDlg::OnCrossHatch(wxCommandEvent& ev)
{
	int crosshatch_sel = ev.GetSelection();
	if (crosshatch_sel == 0) {
		GdaConst::use_cross_hatching = false;
		OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "0");
	}
	else if (crosshatch_sel == 1) {
		GdaConst::use_cross_hatching = true;
		OGRDataAdapter::GetInstance().AddEntry("use_cross_hatching", "1");
	}
	if (highlight_state) {
		highlight_state->notifyObservers();
	}
}

void PreferenceDlg::OnHideTablePostGIS(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::hide_sys_table_postgres = false;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_postgres", "0");
	}
	else {
		GdaConst::hide_sys_table_postgres = true;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_postgres", "1");
	}
}

void PreferenceDlg::OnHideTableSQLITE(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::hide_sys_table_sqlite = false;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_sqlite", "0");
	}
	else {
		GdaConst::hide_sys_table_sqlite = true;
		OGRDataAdapter::GetInstance().AddEntry("hide_sys_table_sqlite", "1");

	}
}
void PreferenceDlg::OnDisableCrashDetect(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::disable_crash_detect = false;
		OGRDataAdapter::GetInstance().AddEntry("disable_crash_detect", "0");
	}
	else {
		GdaConst::disable_crash_detect = true;
		OGRDataAdapter::GetInstance().AddEntry("disable_crash_detect", "1");

	}
}
void PreferenceDlg::OnDisableAutoUpgrade(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::disable_auto_upgrade = false;
		OGRDataAdapter::GetInstance().AddEntry("disable_auto_upgrade", "0");
	}
	else {
		GdaConst::disable_auto_upgrade = true;
		OGRDataAdapter::GetInstance().AddEntry("disable_auto_upgrade", "1");

	}
}
void PreferenceDlg::OnShowRecent(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::show_recent_sample_connect_ds_dialog = false;
		OGRDataAdapter::GetInstance().AddEntry("show_recent_sample_connect_ds_dialog", "0");
	}
	else {
		GdaConst::show_recent_sample_connect_ds_dialog = true;
		OGRDataAdapter::GetInstance().AddEntry("show_recent_sample_connect_ds_dialog", "1");

	}
}
void PreferenceDlg::OnShowCsvInMerge(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::show_csv_configure_in_merge = false;
		OGRDataAdapter::GetInstance().AddEntry("show_csv_configure_in_merge", "0");
	}
	else {
		GdaConst::show_csv_configure_in_merge = true;
		OGRDataAdapter::GetInstance().AddEntry("show_csv_configure_in_merge", "1");
	}
}
void PreferenceDlg::OnEnableHDPISupport(wxCommandEvent& ev)
{
	int sel = ev.GetSelection();
	if (sel == 0) {
		GdaConst::enable_high_dpi_support = false;
		OGRDataAdapter::GetInstance().AddEntry("enable_high_dpi_support", "0");
	}
	else {
		GdaConst::enable_high_dpi_support = true;
		OGRDataAdapter::GetInstance().AddEntry("enable_high_dpi_support", "1");
	}
}
void PreferenceDlg::OnUseSpecifiedSeed(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::use_gda_user_seed = false;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "0");
    }
    else {
        GdaConst::use_gda_user_seed = true;
        OGRDataAdapter::GetInstance().AddEntry("use_gda_user_seed", "1");
    }
}
void PreferenceDlg::OnSeedEnter(wxCommandEvent& ev)
{
    wxString val = txt_seed->GetValue();
    long _val;
    if (val.ToLong(&_val)) {
        GdaConst::gda_user_seed = _val;
        OGRDataAdapter::GetInstance().AddEntry("gda_user_seed", val.ToStdString());
    }
}
void PreferenceDlg::OnSetCPUCores(wxCommandEvent& ev)
{
    int sel = ev.GetSelection();
    if (sel == 0) {
        GdaConst::gda_set_cpu_cores = false;
        OGRDataAdapter::GetInstance().AddEntry("gda_set_cpu_cores", "0");
        txt_cores->Disable();
    }
    else {
        GdaConst::gda_set_cpu_cores = true;
        OGRDataAdapter::GetInstance().AddEntry("gda_set_cpu_cores", "1");
        txt_cores->Enable();
    }
}
void PreferenceDlg::OnCPUCoresEnter(wxCommandEvent& ev)
{
    wxString val = txt_cores->GetValue();
    long _val;
    if (val.ToLong(&_val)) {
        GdaConst::gda_cpu_cores = _val;
        OGRDataAdapter::GetInstance().AddEntry("gda_cpu_cores", val.ToStdString());
    }
}

void PreferenceDlg::OnPowerEpsEnter(wxCommandEvent& ev)
{
    wxString val = txt_poweriter_eps->GetValue();
    double _val;
    if (val.ToDouble(&_val)) {
        GdaConst::gda_eigen_tol = _val;
        OGRDataAdapter::GetInstance().AddEntry("gda_eigen_tol", val.ToStdString());
    }
}
