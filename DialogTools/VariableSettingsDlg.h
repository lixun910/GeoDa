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

#ifndef __GEODA_CENTER_VARIABLE_SETTINGS_DLG_H___
#define __GEODA_CENTER_VARIABLE_SETTINGS_DLG_H___

#include <vector>
#include <map>

#include <boost/uuid/uuid.hpp>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>
#include "../VarTools.h"
#include "../Explore/CatClassification.h"
#include "../VarCalc/WeightsMetaInfo.h"
#include "../FramesManagerObserver.h"

class Project;
class TableInterface;


////////////////////////////////////////////////////////////////////////////
//
// class DiffMoranVarSettingDlg
//
////////////////////////////////////////////////////////////////////////////
class DiffMoranVarSettingDlg : public wxDialog
{
public:
    DiffMoranVarSettingDlg(Project* project);
    virtual ~DiffMoranVarSettingDlg();
    
    boost::uuids::uuid GetWeightsId();
    std::vector<GdaVarTools::VarInfo> var_info;
    std::vector<int> col_ids;
    
protected:
    void OnOK( wxCommandEvent& event );
    void OnClose( wxCommandEvent& event );

    void CreateControls();
    bool Init();
    
    void InitVariableCombobox(wxComboBox* var_box);
    void InitTimeComboboxes(wxComboBox* time1, wxComboBox* time2);
    void InitWeightsCombobox(wxComboBox* weights_ch);
    
private:
    Project* project;
    TableInterface* table_int;
    std::vector<wxString> tm_strs;
    std::vector<boost::uuids::uuid> weights_ids;
    
    wxComboBox* combo_var;
    wxComboBox* combo_time1;
    wxComboBox* combo_time2;
    wxComboBox* combo_weights;
};


////////////////////////////////////////////////////////////////////////////
//
// class VariableSettingsDlg
//
////////////////////////////////////////////////////////////////////////////


class VariableSettingsDlg: public wxDialog
{
public:
	enum VarType {
		univariate, bivariate, trivariate, quadvariate, rate_smoothed
	};
    
    enum Style {
        DEFAULT_STYLE = 0,
        SHOW_WEIGHTS = 1, //0b00000001
        SHOW_DISTANCE = 2, //0b00000010
        SET_SECOND_FROM_FIRST = 4,
        SET_FOURTH_FROM_THIRD = 8,
        SHOW_TIME = 16,
        ALLOW_STRING_IN_FIRST = 32,
        ALLOW_STRING_IN_SECOND = 64,
        ALLOW_STRING_IN_THIRD = 128,
        ALLOW_STRING_IN_FOURTH = 256,
        ALLOW_EMPTY_IN_FIRST = 512,
        ALLOW_EMPTY_IN_SECOND = 1024
    };
    
    VariableSettingsDlg(Project* project, VarType v_type, int style,
                        const wxString& title=_("Variable Settings"),
                        const wxString& var1_title=_("First Variable (X)"),
                        const wxString& var2_title=_("Second Variable (Y)"),
                        const wxString& var3_title=_("Third Variable (Z)"),
                        const wxString& var4_title=_("Fourth Variable"));
    
	VariableSettingsDlg(Project* project, VarType v_type,
						bool show_weights = false,
						bool show_distance = false,
						const wxString& title=_("Variable Settings"),
						const wxString& var1_title=_("First Variable (X)"),
						const wxString& var2_title=_("Second Variable (Y)"),
						const wxString& var3_title=_("Third Variable (Z)"),
						const wxString& var4_title=_("Fourth Variable"),
						bool set_second_from_first_mode = false,
						bool set_fourth_from_third_mode = false,
                        bool hide_time = false,
                        bool var1_str = false, // if show string fields
                        bool var2_str = false,
                        bool var3_str = false,
                        bool var4_str = false);
	virtual ~VariableSettingsDlg();
	void CreateControls();
	void Init(VarType var_type);

    void OnMapThemeChange( wxCommandEvent& event );
	void OnListVariable1DoubleClicked( wxCommandEvent& event );
	void OnListVariable2DoubleClicked( wxCommandEvent& event );
	void OnListVariable3DoubleClicked( wxCommandEvent& event );
	void OnListVariable4DoubleClicked( wxCommandEvent& event );
	void OnVar1Change( wxCommandEvent& event );
	void OnVar2Change( wxCommandEvent& event );
	void OnVar3Change( wxCommandEvent& event );
	void OnVar4Change( wxCommandEvent& event );
	void OnTime1( wxCommandEvent& event );
	void OnTime2( wxCommandEvent& event );
	void OnTime3( wxCommandEvent& event );
	void OnTime4( wxCommandEvent& event );
	void OnSpinCtrl( wxSpinEvent& event );
	void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );

    bool IsFirstVariableEmpty();
    bool IsSecondVariableEmpty();
    
	std::vector<GdaVarTools::VarInfo> var_info;
	std::vector<int> col_ids;
	CatClassification::CatClassifType GetCatClassifType(); // for rate smoothed
	int GetNumCategories(); // for rate smoothed
	boost::uuids::uuid GetWeightsId();
	WeightsMetaInfo::DistanceMetricEnum GetDistanceMetric();
	WeightsMetaInfo::DistanceUnitsEnum GetDistanceUnits();
	
protected:
	int m_theme; // for rate_smoothed
    int style;
    
    bool var1_str;
    bool var2_str;
    bool var3_str;
    bool var4_str;
    std::map<int, int> sel1_idx_map;
    std::map<int, int> sel2_idx_map;
    std::map<int, int> sel3_idx_map;
    std::map<int, int> sel4_idx_map;
    std::map<int, int> idx_sel1_map;
    std::map<int, int> idx_sel2_map;
    std::map<int, int> idx_sel3_map;
    std::map<int, int> idx_sel4_map;
    
    bool hide_time;
	wxString v1_name;
	wxString v2_name;
	wxString v3_name;
	wxString v4_name;
    wxString default_var_name1;
    wxString default_var_name2;
    wxString default_var_name3;
    wxString default_var_name4;
	int v1_time;
	int v2_time;
	int v3_time;
	int v4_time;
	int v1_col_id;
	int v2_col_id;
	int v3_col_id;
	int v4_col_id;
	
	VarType v_type;
	wxListBox* lb1;
    wxListBox* lb2;
	wxListBox* lb3;
	wxListBox* lb4;
	int lb1_cur_sel;
	int lb2_cur_sel;
	int lb3_cur_sel;
	int lb4_cur_sel;
	wxChoice* time_lb1;
	wxChoice* time_lb2;
	wxChoice* time_lb3;
	wxChoice* time_lb4;
	
	wxString title;
	wxString var1_title;
	wxString var2_title;
	wxString var3_title;
	wxString var4_title;
	
	wxChoice* map_theme_ch; // for rate_smoothed
	wxSpinCtrl* num_cats_spin;
	int num_categories;
	
	bool show_weights;
	bool no_weights_found_fail;
	wxChoice* weights_ch;
	std::vector<boost::uuids::uuid> weights_ids;
	
	bool show_distance;
	wxChoice* distance_ch;
	
	int num_var; // 1, 2, 3, or 4
	bool is_time;
	int time_steps;
	
	bool all_init;

	Project* project;
	TableInterface* table_int;
	// col_id_map[i] is a map from the i'th numeric item in the
	// fields drop-down to the actual col_id_map.  Items
	// in the fields dropdown are in the order displayed in wxGrid
	std::vector<int> col_id_map;

	void InitTimeChoices();
	void InitFieldChoices();
	wxString FillData();
    
    bool CheckEmptyColumn(int col_id, int time);
	
	/** Automatically set the second variable to the same value as
	 the first variable when first variable is changed. */
	bool set_second_from_first_mode;
	/** Automatically set the fourth variable to the same value as
	 the third variable when third variable is changed. */
	bool set_fourth_from_third_mode;

	DECLARE_EVENT_TABLE()
};

/*
class UniVarSettingsDlg: public VariableSettingsDlg
{
    UniVarSettingsDlg(Project* project,
                      VarType v_type,
                      bool show_weights = false,
                      bool show_distance = false,
                      bool show_time = true,
                      bool var1_str = false // if show string fields
    );
    virtual ~UniVarSettingsDlg();
   
    virtual void CreateControls();
    virtual void InitFieldChoices();
    virtual void FillData();
    
    void OnOkClick( wxCommandEvent& event );
};


class BiVarSettingsDlg: public VariableSettingsDlg
{
    BiVarSettingsDlg(Project* project,
                     VarType v_type,
                     bool show_weights = false,
                     bool show_distance = false,
                     bool show_time = true,
                     bool var1_str = false, // if show string fields
                     bool var2_str = false, // if show string fields
                     bool set_second_from_first_mode = false);
    virtual ~BiVarSettingsDlg();
   
    virtual void CreateControls();
    virtual void InitFieldChoices();
    virtual void FillData();
    
    void OnOkClick( wxCommandEvent& event );
};

class TriVarSettingsDlg: public VariableSettingsDlg
{
    TriVarSettingsDlg(Project* project,
                      VarType v_type,
                      bool show_weights = false,
                      bool show_distance = false,
                      bool show_time = true,
                      bool var1_str = false, // if show string fields
                      bool var2_str = false, // if show string fields
                      bool var3_str = false);
    virtual ~TriVarSettingsDlg();
   
    virtual void CreateControls();
    virtual void InitFieldChoices();
    virtual void FillData();
    
    void OnOkClick( wxCommandEvent& event );
};

class QuadVarSettingsDlg: public VariableSettingsDlg
{
    QuadVarSettingsDlg(Project* project,
                       VarType v_type,
                       bool show_weights = false,
                       bool show_distance = false,
                       bool show_time = true,
                       bool var1_str = false, // if show string fields
                       bool var2_str = false, // if show string fields
                       bool var3_str = false);
    virtual ~QuadVarSettingsDlg();
   
    virtual void CreateControls();
    virtual void InitFieldChoices();
    virtual void FillData();
    
    void OnOkClick( wxCommandEvent& event );
};

class RateSmoothedSettingsDlg: public VariableSettingsDlg
{
    RateSmoothedSettingsDlg(Project* project,
                            VarType v_type,
                            bool show_weights = false,
                            bool show_time = true
                            );
    virtual ~RateSmoothedSettingsDlg();
   
    virtual void CreateControls();
    virtual void InitFieldChoices();
    virtual void FillData();
    
    void OnOkClick( wxCommandEvent& event );
};
*/
#endif
