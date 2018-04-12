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

#include <vector>
#include <boost/foreach.hpp>
#include <boost/random.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/msgdlg.h>
#include <wx/valtext.h>

#include "../GdaConst.h"
#include "../Project.h"
#include "../FramesManager.h"
#include "../DataViewer/DataViewerAddColDlg.h"
#include "../DataViewer/TableInterface.h"
#include "../DataViewer/TableState.h"
#include "../ShapeOperations/WeightsManState.h"
#include "../VarCalc/WeightsManInterface.h"
#include "../logger.h"
#include "RangeSelectionDlg.h"

BEGIN_EVENT_TABLE( RangeSelectionDlg, wxDialog )
    EVT_RADIOBUTTON(XRCID("IDC_RADIO_NEWSELECT"), RangeSelectionDlg::OnSetNewSelect)
    EVT_RADIOBUTTON(XRCID("IDC_RADIO_SUBSELECT"), RangeSelectionDlg::OnSetSubSelect)
    EVT_RADIOBUTTON(XRCID("IDC_RADIO_APPENDSELECT"), RangeSelectionDlg::OnSetAppendSelect)
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE"), RangeSelectionDlg::OnFieldChoice )
	EVT_CHOICE( XRCID("ID_FIELD_CHOICE_TM"), RangeSelectionDlg::OnFieldChoiceTm )
	EVT_TEXT( XRCID("ID_MIN_TEXT"), RangeSelectionDlg::OnRangeTextChange )
	EVT_TEXT( XRCID("ID_MAX_TEXT"), RangeSelectionDlg::OnRangeTextChange )
    EVT_BUTTON( XRCID("ID_SEL_RANGE_BUTTON"), RangeSelectionDlg::OnSelRangeClick )
	EVT_BUTTON( XRCID("ID_SEL_UNDEF_BUTTON"), RangeSelectionDlg::OnSelUndefClick )
	EVT_BUTTON( XRCID("ID_INVERT_SEL_BUTTON"), RangeSelectionDlg::OnInvertSelClick )
	//EVT_BUTTON( XRCID("ID_RANDOM_SEL_BUTTON"), RangeSelectionDlg::OnRandomSelClick )
	EVT_BUTTON( XRCID("ID_CLEAR_SEL_BUTTON"), RangeSelectionDlg::OnClearSelClick )
	EVT_BUTTON( XRCID("ID_ADD_NEIGHS_TO_SEL_BUTTON"), RangeSelectionDlg::OnAddNeighsToSelClick )
	EVT_BUTTON( XRCID("ID_ADD_FIELD"), RangeSelectionDlg::OnAddField )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE"), RangeSelectionDlg::OnSaveFieldChoice )
	EVT_CHOICE( XRCID("ID_SAVE_FIELD_CHOICE_TM"), RangeSelectionDlg::OnSaveFieldChoiceTm )
	EVT_CHECKBOX( XRCID("ID_SEL_CHECK_BOX"), RangeSelectionDlg::OnSelCheckBox )
	EVT_TEXT( XRCID("ID_SEL_VAL_TEXT"), RangeSelectionDlg::OnSelUnselTextChange )
	EVT_CHECKBOX( XRCID("ID_UNSEL_CHECK_BOX"), RangeSelectionDlg::OnUnselCheckBox )
	EVT_TEXT( XRCID("ID_UNSEL_VAL_TEXT"), RangeSelectionDlg::OnSelUnselTextChange )
	EVT_BUTTON( XRCID("ID_APPLY_SAVE_BUTTON"), RangeSelectionDlg::OnApplySaveClick )
	EVT_BUTTON( XRCID("wxID_CLOSE"), RangeSelectionDlg::OnCloseClick )
	
END_EVENT_TABLE()

RangeSelectionDlg::RangeSelectionDlg(wxWindow* parent, Project* _project,
									 FramesManager* _frames_manager,
									 TableState* _table_state,
									 const wxString& title, const wxPoint& pos)
: project(_project), frames_manager(_frames_manager),
table_state(_table_state), table_int(_project->GetTableInt()),
w_man_int(project->GetWManInt()), w_man_state(project->GetWManState()),
current_sel_mcol(wxNOT_FOUND),
m_field_choice(0), m_min_text(0), m_field_static_txt(0), m_field2_static_txt(0),
m_max_text(0), m_sel_range_button(0), m_sel_undef_button(0),
m_invert_sel_button(0), m_random_sel_button(0), m_clear_sel_button(0),
m_num_to_rand_sel_txt(0),
m_add_neighs_to_sel_button(0), m_weights_choice(0),
m_save_field_choice(0), m_sel_check_box(0),
m_sel_val_text(0), m_unsel_check_box(0), m_unsel_val_text(0),
m_apply_save_button(0), all_init(false), m_selection_made(false)
{
    wxLogMessage("Open RangeSelectionDlg");
    
	SetParent(parent);
	RefreshColIdMap();
    CreateControls();
	all_init = true;
	SetTitle(title);
    Centre();
	InitSelectionVars();
	InitSaveVars();
	CheckRangeButtonSettings();
	CheckApplySaveSettings();
	RefreshWeightsIds();
	
	frames_manager->registerObserver(this);
	table_state->registerObserver(this);
	w_man_state->registerObserver(this);
}

RangeSelectionDlg::~RangeSelectionDlg()
{
	frames_manager->removeObserver(this);
	table_state->removeObserver(this);
	w_man_state->removeObserver(this);
}

void RangeSelectionDlg::CreateControls()
{
	wxXmlResource::Get()->LoadDialog(this, GetParent(),
									 "IDD_RANGE_SELECTION_DLG");
	m_field_choice = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE")),
								   wxChoice);

	m_field_choice_tm = wxDynamicCast(FindWindow(XRCID("ID_FIELD_CHOICE_TM")),
									  wxChoice);
	
	m_min_text = wxDynamicCast(FindWindow(XRCID("ID_MIN_TEXT")),
							   wxTextCtrl);
	m_min_text->Clear();
	m_min_text->AppendText("0");
	m_min_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	m_field_static_txt = wxDynamicCast(FindWindow(XRCID("ID_FIELD_STATIC_TXT")),
									   wxStaticText);
	m_field2_static_txt =
		wxDynamicCast(FindWindow(XRCID("ID_FIELD2_STATIC_TXT")), wxStaticText);
	
	m_max_text = wxDynamicCast(FindWindow(XRCID("ID_MAX_TEXT")),
							   wxTextCtrl);
	m_max_text->Clear();
	m_max_text->AppendText("1");
	m_max_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	m_sel_range_button = wxDynamicCast(FindWindow(XRCID("ID_SEL_RANGE_BUTTON")),
									   wxButton);
	m_sel_range_button->Enable(false);
	
	m_sel_undef_button = wxDynamicCast(FindWindow(XRCID("ID_SEL_UNDEF_BUTTON")),
									   wxButton);
	m_sel_undef_button->Enable(false);

	m_clear_sel_button = wxDynamicCast(
						FindWindow(XRCID("ID_CLEAR_SEL_BUTTON")), wxButton);

	m_invert_sel_button = wxDynamicCast(
						FindWindow(XRCID("ID_INVERT_SEL_BUTTON")), wxButton);
	
	//m_num_to_rand_sel_txt = wxDynamicCast(FindWindow(XRCID("ID_NUM_TO_RAND_SEL")), wxTextCtrl);
	//m_num_to_rand_sel_txt->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	// Select 10% of objects by default
	wxString num_to_sel;
	num_to_sel << (project->GetNumRecords() / 10);
	//m_num_to_rand_sel_txt->SetValue(num_to_sel);
	
	//m_random_sel_button = wxDynamicCast(FindWindow(XRCID("ID_RANDOM_SEL_BUTTON")), wxButton);
	
		
	m_add_neighs_to_sel_button = wxDynamicCast(
			   FindWindow(XRCID("ID_ADD_NEIGHS_TO_SEL_BUTTON")), wxButton);
														  
	m_weights_choice = wxDynamicCast(FindWindow(XRCID("ID_WEIGHTS")), wxChoice);
	
	m_save_field_choice =
		wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE")), wxChoice);
	m_save_field_choice_tm =
		wxDynamicCast(FindWindow(XRCID("ID_SAVE_FIELD_CHOICE_TM")), wxChoice);
	
	m_sel_check_box = wxDynamicCast(FindWindow(XRCID("ID_SEL_CHECK_BOX")),
									wxCheckBox); 
	
	m_sel_val_text = wxDynamicCast(FindWindow(XRCID("ID_SEL_VAL_TEXT")),
									wxTextCtrl);
	m_sel_val_text->Clear();
	m_sel_val_text->AppendText("1");
	m_sel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));

	m_unsel_check_box = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_CHECK_BOX")),
									  wxCheckBox); 
	
	m_unsel_val_text = wxDynamicCast(FindWindow(XRCID("ID_UNSEL_VAL_TEXT")),
									 wxTextCtrl);	
	m_unsel_val_text->Clear();
	m_unsel_val_text->AppendText("0");
	m_unsel_val_text->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	
	m_apply_save_button = wxDynamicCast(
		FindWindow(XRCID("ID_APPLY_SAVE_BUTTON")), wxButton);
	m_apply_save_button->Disable();
   

    m_radio_newselect = XRCCTRL(*this, "IDC_RADIO_NEWSELECT", wxRadioButton);
    m_radio_subselect = XRCCTRL(*this, "IDC_RADIO_SUBSELECT", wxRadioButton);
    m_radio_appendselect = XRCCTRL(*this, "IDC_RADIO_APPENDSELECT", wxRadioButton);
}

void RangeSelectionDlg::OnSetNewSelect( wxCommandEvent& event )
{
    
}
void RangeSelectionDlg::OnSetSubSelect( wxCommandEvent& event )
{
    
}
void RangeSelectionDlg::OnSetAppendSelect( wxCommandEvent& event )
{
    
}

void RangeSelectionDlg::OnFieldChoice( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnFieldChoice()");
	InitSelectionVars();
	CheckRangeButtonSettings();
}

void RangeSelectionDlg::OnFieldChoiceTm( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnFieldChoiceTm()");
	InitSelectionVars();
	CheckRangeButtonSettings();
}

void RangeSelectionDlg::OnRangeTextChange( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnRangeTextChange()");
	CheckRangeButtonSettings();
}

void RangeSelectionDlg::OnSelRangeClick( wxCommandEvent& event )
{
	wxLogMessage("Entering RangeSelectionDlg::OnApplySelClick");
	HighlightState& hs = *project->GetHighlightState();
	std::vector<bool>& h = hs.GetHighlight();
    
    int n = table_int->GetNumberRows();
	int nh_cnt = 0;
	int nuh_cnt = 0;
	if (m_field_choice->GetSelection() == wxNOT_FOUND) return;
	int mcol = GetSelColInt();
	int f_tm = GetSelColTmInt();

    bool no_hl = true;
    bool new_select = m_radio_newselect->GetValue();
    bool sub_select = m_radio_subselect->GetValue();
    bool append_select = !sub_select;

    if (new_select) {
        for (int i=0; i<n; i++) {
            h[i] = false;
        }
    }
    
    for (int i=0; i<n; i++) {
        if (h[i] == true) {
            no_hl = false;
            break;
        }
    }
    std::vector<bool> cur_sel(n);
    

	double min_dval = 0;
	m_min_text->GetValue().ToDouble(&min_dval);
	double max_dval = 1;
	m_max_text->GetValue().ToDouble(&max_dval);
	std::vector<bool> undefined;
	table_int->GetColUndefined(mcol, f_tm, undefined);
	if (table_int->GetColType(mcol) == GdaConst::long64_type) {
		wxInt64 min_ival = ceil(min_dval);
		wxInt64 max_ival = floor(max_dval);
		std::vector<wxInt64> vec;
		table_int->GetColData(mcol, f_tm, vec);
		for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
			if (!undefined[i] && min_ival <= vec[i] && vec[i] <= max_ival) {
                cur_sel[i] = true;
			} else {
                cur_sel[i] = false;
            }
		}
	} else if (table_int->GetColType(mcol) == GdaConst::double_type) {
		std::vector<double> vec;
		table_int->GetColData(mcol, f_tm, vec);
		for (int i=0, iend=table_int->GetNumberRows(); i<iend; i++) {
			if (!undefined[i] && min_dval <= vec[i] && vec[i] <= max_dval) {
                cur_sel[i] = true;
			} else {
                cur_sel[i] = false;
			}
		}
	} else {
		wxString msg("Selected field is should be numeric.");
		wxMessageDialog dlg (this, msg, _("Error"), wxOK | wxICON_ERROR);
		dlg.ShowModal();
		return;
	}
    bool update_flag = false;
    for (int i=0; i<n; i++) {
        if (no_hl) {
            if (cur_sel[i] == true) {
                h[i] = true;
                update_flag = true;
            }
        } else {
            if (sub_select) {
                if (h[i] == true && cur_sel[i] == false) {
                    h[i] = false;
                    update_flag = true;
                }
            } else if (append_select) {
                if (h[i] == false && cur_sel[i] == true) {
                    h[i] = true;
                    update_flag = true;
                }
            }
        }
    }

	if (update_flag) {
		hs.SetEventType(HLStateInt::delta);
		hs.notifyObservers();
	}
	current_sel_mcol = mcol;
	m_selection_made = true;
	CheckApplySaveSettings();

}

void RangeSelectionDlg::OnSelUndefClick( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnSelUndefClick()");
	HighlightState& hs = *project->GetHighlightState();
	std::vector<bool>& h = hs.GetHighlight();
    bool selection_changed = false;

	if (m_field_choice->GetSelection() == wxNOT_FOUND) return;
	int mcol = GetSelColInt();
	int f_tm = GetSelColTmInt();
	
	std::vector<bool> undefined;
	table_int->GetColUndefined(mcol, f_tm, undefined);
	for (int i=0, iend=h.size(); i<iend; i++) {
        if (undefined[i]) {
            if (!h[i]) {
                h[i] = true;
                selection_changed = true;
            }
        } else {
            if (h[i]) {
                h[i] = false;
                selection_changed = true;
            }
        }
	}
	if (selection_changed) {
		hs.SetEventType(HLStateInt::delta);
		hs.notifyObservers();
	}
	m_selection_made = true;
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnRandomSelClick( wxCommandEvent& event )
{

}

void RangeSelectionDlg::OnClearSelClick( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnClearSelClick()");
	HighlightState& hs = *project->GetHighlightState();
	hs.SetEventType(HLStateInt::unhighlight_all);
	hs.notifyObservers();
	m_selection_made = true;
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnInvertSelClick( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnInvertSelClick()");
	HighlightState& hs = *project->GetHighlightState();
	hs.SetEventType(HLStateInt::invert);
	hs.notifyObservers();
	m_selection_made = true;
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnAddNeighsToSelClick( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnAddNeighsToSelClick()");
	project->AddNeighborsToSelection(GetWeightsId());
}

void RangeSelectionDlg::OnAddField( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnAddField()");
	DataViewerAddColDlg dlg(project, this, true, true, "SELECT",GdaConst::long64_type);
	if (dlg.ShowModal() != wxID_OK) return;
	int col = dlg.GetColId();
	if (table_int->GetColType(col) != GdaConst::long64_type &&
		table_int->GetColType(col) != GdaConst::double_type) return;

	// DataViewerAddColDlg will result in TableState::notify being
	// called upon success, which means RangeSelectionDlg::update
	// will have been called by this execution point.
	
	int sel = m_save_field_choice->FindString(table_int->GetColName(col));
	if (sel != wxNOT_FOUND) m_save_field_choice->SetSelection(sel);
	
	InitSaveVars(); // call again in case selection changed
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSaveFieldChoice( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnSaveFieldChoice()");
	InitSaveVars();
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSaveFieldChoiceTm( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnSaveFieldChoiceTm()");
	InitSaveVars();
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSelCheckBox( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnSelCheckBox()");
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnUnselCheckBox( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnUnselCheckBox()");
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnSelUnselTextChange( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnSelUnselTextChange()");
	CheckApplySaveSettings();
}

void RangeSelectionDlg::OnApplySaveClick( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnApplySaveClick()");
	 // The Apply button is only enable when Selected / Unselected values
	 // are valid (only when checked), and at least one checkbox is
	 // selected.  The Target Variable is not empty, but has not been
	 // checked for validity.
	
	int write_col = GetSaveColInt();
	
	TableState* ts = project->GetTableState();
	wxString grp_nm = table_int->GetColName(write_col);
	if (!Project::CanModifyGrpAndShowMsgIfNot(ts, grp_nm)) return;
	
	bool sel_checked = m_sel_check_box->GetValue() == 1;
	bool unsel_checked = m_unsel_check_box->GetValue() == 1;
	
	double sel_c = 0;
    wxString sel_c_str = m_sel_val_text->GetValue();
	if (sel_checked && !sel_c_str.IsEmpty()) {
		sel_c_str.Trim(false); sel_c_str.Trim(true);
		sel_c_str.ToDouble(&sel_c);
	}
	double unsel_c = 0;
    wxString unsel_c_str = m_unsel_val_text->GetValue();
	if (unsel_checked && !unsel_c_str.IsEmpty()) {
		unsel_c_str.Trim(false); unsel_c_str.Trim(true);
		unsel_c_str.ToDouble(&unsel_c);
	}
	
	int sf_tm = GetSaveColTmInt();
	
	std::vector<bool>& h = project->GetHighlightState()->GetHighlight();
	// write_col now refers to a valid field in grid base, so write out
	// results to that field.
	int obs = h.size();
	std::vector<bool> undefined;
	if (table_int->GetColType(write_col) == GdaConst::long64_type) {
		wxInt64 sel_c_i = sel_c;
		wxInt64 unsel_c_i = unsel_c;
		std::vector<wxInt64> t(table_int->GetNumberRows());
		table_int->GetColData(write_col, sf_tm, t);
		table_int->GetColUndefined(write_col, sf_tm, undefined);
		if (sel_checked) {
            bool flag = sel_c_str.IsEmpty();
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c_i;
					undefined[i] = flag;
				}
			}
		}
		if (unsel_checked) {
            bool flag = unsel_c_str.IsEmpty();
			for (int i=0; i<obs; i++) {
				if (!h[i]) {
					t[i] = unsel_c_i;
					undefined[i] = flag;
				}
			}
		}
		table_int->SetColData(write_col, sf_tm, t);
		table_int->SetColUndefined(write_col, sf_tm, undefined);
	} else if (table_int->GetColType(write_col) == GdaConst::double_type) {
		std::vector<double> t(table_int->GetNumberRows());
		table_int->GetColData(write_col, sf_tm, t);
		table_int->GetColUndefined(write_col, sf_tm, undefined);
		if (sel_checked) {
            bool flag = sel_c_str.IsEmpty();
			for (int i=0; i<obs; i++) {
				if (h[i]) {
					t[i] = sel_c;
					undefined[i] = flag;
				}
			}
		}
		if (unsel_checked) {
            bool flag = unsel_c_str.IsEmpty();
			for (int i=0; i<obs; i++) {
				if (!h[i]) {
					t[i] = unsel_c;
					undefined[i] = flag;
				}
			}
		}
		table_int->SetColData(write_col, sf_tm, t);
		table_int->SetColUndefined(write_col, sf_tm, undefined);
	} else {
		wxString msg = _("Chosen field is not a numeric type. Please select a numeric type field.");

		wxMessageDialog dlg(this, msg, _("Error"), wxOK | wxICON_ERROR );
		dlg.ShowModal();
		return;
	}
	
	wxString msg = _("Values assigned to target field successfully.");
	wxMessageDialog dlg(this, msg, "Success", wxOK | wxICON_INFORMATION );
	dlg.ShowModal();
}

void RangeSelectionDlg::OnCloseClick( wxCommandEvent& event )
{
    wxLogMessage("In RangeSelectionDlg::OnCloseClick()");
	event.Skip();
	EndDialog(wxID_CLOSE);
}

void RangeSelectionDlg::update(FramesManager* o)
{
}

void RangeSelectionDlg::update(TableState* o)
{
	RefreshColIdMap();
	InitSelectionVars();
	InitSaveVars();
	CheckRangeButtonSettings();
	CheckApplySaveSettings();
	Refresh();
}

/** Implementation of WeightsManStateObserver interface */
void RangeSelectionDlg::update(WeightsManState* o)
{
	RefreshWeightsIds();
	Refresh();
}


void RangeSelectionDlg::RefreshColIdMap()
{
	col_id_map.clear();
	table_int->FillNumericColIdMap(col_id_map);
}

void RangeSelectionDlg::RefreshWeightsIds()
{
	if (!m_weights_choice) return;
	int cur_sel = m_weights_choice->GetSelection();
	wxString sel_str;
	if (cur_sel >= 0) {
		sel_str = m_weights_choice->GetString(cur_sel);
	}
	m_weights_choice->Clear();
	weights_ids.clear();
	w_man_int->GetIds(weights_ids);
	int sel_pos=-1;
	for (size_t i=0; i<weights_ids.size(); ++i) {
		wxString weights_nm = w_man_int->GetShortDispName(weights_ids[i]);
		m_weights_choice->Append(weights_nm);
		if (weights_nm == sel_str) sel_pos = i;
	}
	if (sel_pos < 0) {
		for (size_t i=0; i<weights_ids.size(); ++i) {
			if (w_man_int->GetDefault() == weights_ids[i]) sel_pos = i;
		}
	}
	if (weights_ids.size() > 0 && sel_pos < 0) sel_pos = 0;
	if (sel_pos >= 0) m_weights_choice->SetSelection(sel_pos);
}

boost::uuids::uuid RangeSelectionDlg::GetWeightsId()
{
	if (!m_weights_choice) return boost::uuids::nil_uuid();
	int sel = m_weights_choice->GetSelection();
	if (sel < 0 || sel >= weights_ids.size()) return boost::uuids::nil_uuid();

	return weights_ids[sel];
}

void RangeSelectionDlg::InitSelectionVars()
{
	InitVars(m_field_choice, m_field_choice_tm);
}

void RangeSelectionDlg::InitSaveVars()
{
	InitVars(m_save_field_choice, m_save_field_choice_tm);
}	

void RangeSelectionDlg::InitVars(wxChoice* field, wxChoice* field_tm)
{
	if (!field	|| !field_tm) return;
	
	// save original field_choice selection
	wxString cur_str_sel = field->GetStringSelection();
	
	// clear field_choice selection and repopulate
	field->Clear();
	for (int i=0, iend=col_id_map.size(); i<iend; i++) {
		int col = col_id_map[i];
		field->Append(table_int->GetColName(col));
	}
	
	// reselect original selection if possible
	field->SetSelection(field->FindString(cur_str_sel));
	
	// save original field_choice time selection
	wxString cur_str_tm_sel = field_tm->GetStringSelection();
	
	// clear field_choice_tm selection and repopulate if currently
	// a valid field_choice selection
	// reselect original field_choice_tm selection if possible.  If not
	// possible, and if time variant, select the first item on the
	// list.
	field_tm->Clear();
	bool is_time_variant = false;
	if (field->FindString(cur_str_sel) != wxNOT_FOUND) {
		int col = table_int->FindColId(cur_str_sel);
		if (col != -1 && table_int->IsColTimeVariant(col)) {
			is_time_variant = true;
			std::vector<wxString> tm_strs;
			table_int->GetColNonPlaceholderTmStrs(col, tm_strs);
			BOOST_FOREACH(const wxString& s, tm_strs) field_tm->Append(s);
			int sel = field_tm->FindString(cur_str_tm_sel);
			if (sel != wxNOT_FOUND) {
				field_tm->SetSelection(sel);
			} else {
				field_tm->SetSelection(0);
			}
		}
	}
	
	// enable/disable time selection according to time variance
	field_tm->Enable(is_time_variant);
}

bool RangeSelectionDlg::IsTimeVariant()
{
	return table_int->IsTimeVariant();
}

void RangeSelectionDlg::CheckRangeButtonSettings()
{
	if (!all_init) return;
	
	int fc = m_field_choice->GetSelection();
	bool valid_field = fc != wxNOT_FOUND;
	if (valid_field) {
		wxString fn;
		fn = m_field_choice->GetStringSelection();
		wxString tm_str = m_field_choice_tm->GetStringSelection();
		if (!tm_str.IsEmpty()) {
			fn << " (" << tm_str << ")";
		}
		m_field_static_txt->SetLabelText(fn);
		m_field2_static_txt->SetLabelText(fn);
	} else {
		m_field_static_txt->SetLabelText("choose a variable");
		m_field2_static_txt->SetLabelText("choose a variable");
	}
	
	/** Check that min and max range text is valid.  If not valid, set
	 text color to red. */
	double val;
	wxString min_text = m_min_text->GetValue();
	bool min_valid = min_text.ToDouble(&val);
	{
		wxTextAttr style(m_min_text->GetDefaultStyle());
		style.SetTextColour(*(min_valid ? wxBLACK : wxRED));
		m_min_text->SetStyle(0, min_text.length(), style);
	}
	wxString max_text = m_max_text->GetValue();
	bool max_valid = max_text.ToDouble(&val);
	{
		wxTextAttr style(m_max_text->GetDefaultStyle());
		style.SetTextColour(*(max_valid ? wxBLACK : wxRED));
		m_max_text->SetStyle(0, max_text.length(), style);
	}
	
	m_sel_range_button->Enable(min_valid && max_valid && valid_field);
	m_sel_undef_button->Enable(valid_field);
}

void RangeSelectionDlg::CheckApplySaveSettings()
{
    wxLogMessage("In RangeSelectionDlg::CheckApplySaveSettings()");
    
	if (!all_init) return;
	
	bool target_field_empty = m_save_field_choice->GetSelection()==wxNOT_FOUND;
	
	// Check that m_sel_val_text and m_unsel_val_text is valid.
	// If not valid, set text color to red.
	double val;
	wxString sel_text = m_sel_val_text->GetValue();
	bool sel_valid = sel_text.ToDouble(&val) || sel_text.IsEmpty();
	{
		wxTextAttr style(m_sel_val_text->GetDefaultStyle());
		style.SetTextColour(*(sel_valid ? wxBLACK : wxRED));
		m_sel_val_text->SetStyle(0, sel_text.length(), style);
	}
	wxString unsel_text = m_unsel_val_text->GetValue();
	bool unsel_valid = unsel_text.ToDouble(&val) || unsel_text.IsEmpty();
	{
		wxTextAttr style(m_unsel_val_text->GetDefaultStyle());
		style.SetTextColour(*(unsel_valid ? wxBLACK : wxRED));
		m_unsel_val_text->SetStyle(0, unsel_text.length(), style);
	}
	
	bool sel_checked = m_sel_check_box->GetValue() == 1;
	bool unsel_checked = m_unsel_check_box->GetValue() == 1;
    
    // Check if objects already selected
    HighlightState& hs = *project->GetHighlightState();
    std::vector<bool>& hh = hs.GetHighlight();
    if (hh.size() > 0) {
        m_selection_made = true;
    }
	
	m_apply_save_button->Enable(!target_field_empty &&
								(sel_checked || unsel_checked) &&
								((sel_checked && sel_valid) || !sel_checked) &&
								((unsel_checked && unsel_valid) ||
								 !unsel_checked) && m_selection_made);
}

int RangeSelectionDlg::GetSelColInt()
{
	if (!m_field_choice) return -1;
	wxString str = m_field_choice->GetStringSelection();
	return table_int->FindColId(str);
}

int RangeSelectionDlg::GetSelColTmInt()
{
	if (!m_field_choice_tm) return 0;
	wxString str = m_field_choice_tm->GetStringSelection();
	int tm_int = table_int->GetTimeInt(str);
	return (tm_int < 0 ? 0 : tm_int);
}

int RangeSelectionDlg::GetSaveColInt()
{
	if (!m_save_field_choice) return -1;
	wxString str = m_save_field_choice->GetStringSelection();
	return table_int->FindColId(str);
}

int RangeSelectionDlg::GetSaveColTmInt()
{
	if (!m_save_field_choice_tm) return 0;
	wxString str = m_save_field_choice_tm->GetStringSelection();
	int tm_int = table_int->GetTimeInt(str);
	return (tm_int < 0 ? 0 : tm_int);
}
