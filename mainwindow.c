#include <cairo.h>
#include <stdint.h>
#include <gtk/gtk.h>

#include "pulse.h"
#include "adplug-instance.h"

struct operator_t
{
	int id;

	GtkWidget *frame;
	GtkWidget *grid;

	GtkWidget *ModulatorFrequencyMultiple;
	GtkWidget *KSR;
	GtkWidget *EGTyp;
	GtkWidget *Vib;
	GtkWidget *AmpMod;
	GtkWidget *TotalLevel;
	GtkWidget *ScalingLevel;
	GtkWidget *AttackRate;
	GtkWidget *DecayRate;
	GtkWidget *SustainLevel;
	GtkWidget *ReleaseRate;
	GtkWidget *WaveformSelect;
};

struct channel_t
{
	int id;
	struct operator_t *operator1;
	struct operator_t *operator2;

	GtkWidget *header;
	GtkWidget *grid;
	GtkWidget *Feedback;
	GtkWidget *KeyOn;
	GtkWidget *Alg;
	GtkWidget *FNumber;
	GtkWidget *Octave;
};

struct global_t
{
	GtkWidget *frame;
	GtkWidget *grid;
	GtkWidget *AMDep;
	GtkWidget *VibDep;
	GtkWidget *RhyEna;
	GtkWidget *BD;
	GtkWidget *SD;
	GtkWidget *TOM;
	GtkWidget *TopCym;
	GtkWidget *HH;
};

struct channel_t  channels[9];
struct operator_t operators[32];
struct global_t global;

GtkListStore *OperatorModulatorFrequencyMultipleModel;
GtkListStore *OperatorScalingLevelModel;
GtkListStore *OperatorADRRateModel;
GtkListStore *OperatorSustainLevelModel;
GtkListStore *ChannelOctaveModel;
GtkListStore *OperatorWaveformSelectModel;

GtkListStore *ChannelAlgModel;
GtkListStore *ChannelFeedbackModel;

GtkListStore *GlobalAMDepModel;
GtkListStore *GlobalVibDepModel;

GtkWidget *graph;

#define SAMPLES_N 1024
int16_t samples[SAMPLES_N];

static void write_reg(uint8_t reg, uint8_t value)
{
	fprintf (stderr, "[%02x] = 0x%02x\n", reg, value);
	opl_instance_write (reg, value);
}

static void refresh_20 (int opid)
{
	uint8_t value = 0;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(operators[opid].AmpMod))) value |= 0x80;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(operators[opid].Vib)))    value |= 0x40;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(operators[opid].EGTyp)))  value |= 0x20;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(operators[opid].KSR)))    value |= 0x10;

	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].ModulatorFrequencyMultiple));

	write_reg (0x20 + opid, value);
}

static void refresh_40 (int opid)
{
	uint8_t value = 0;

	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].ScalingLevel)) << 6;
	value |= gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(operators[opid].TotalLevel));

	write_reg (0x40 + opid, value);
}

static void refresh_60 (int opid)
{
	uint8_t value = 0;

	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].AttackRate)) << 4;
	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].DecayRate));

	write_reg (0x60 + opid, value);
}

static void refresh_80 (int opid)
{
	uint8_t value = 0;

	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].SustainLevel)) << 4;
	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].ReleaseRate));

	write_reg (0x80 + opid, value);
}

static void refresh_a0 (int chid)
{
	uint8_t value = 0;

	value |= gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(channels[chid].FNumber)) & 0xff;

	write_reg (0xa0 + chid, value);
}

static void refresh_b0 (int chid)
{
	uint8_t value = 0;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(channels[chid].KeyOn))) value |= 0x20;
	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(channels[chid].Octave)) << 2;
	value |= gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(channels[chid].FNumber)) >> 8;

	write_reg (0xb0 + chid, value);
}

static void refresh_bd (void)
{
	uint8_t value = 0;

#warning TODO, handle drums on/off
	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(global.AMDep)) << 7;
	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(global.VibDep)) << 6;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(global.RhyEna))) value |= 0x20;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(global.BD)))     value |= 0x10;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(global.SD)))     value |= 0x08;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(global.TOM)))    value |= 0x04;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(global.TopCym))) value |= 0x02;
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(global.HH)))     value |= 0x01;

	write_reg (0xbd, value);
}

static void refresh_c0 (int chid)
{
	uint8_t value = 0;

	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(channels[chid].Feedback)) << 1;
	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(channels[chid].Alg));

	write_reg (0xc0 + chid, value);
}

static void refresh_e0 (int opid)
{
	uint8_t value = 0;

	value |= gtk_combo_box_get_active (GTK_COMBO_BOX(operators[opid].WaveformSelect));

	write_reg (0xe0 + opid, value);
}


static void Operator_ModulatorFrequencyMultiple_changed (GtkComboBox *combo,
                                                         gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_20 (op->id);
}

static void Operator_KSR_toggled (GtkToggleButton *toggle_button,
                                  gpointer         user_data)
{
	struct operator_t *op = user_data;
	refresh_20 (op->id);
}

static void Operator_EGTyp_toggled (GtkToggleButton *toggle_button,
                                    gpointer         user_data)
{
	struct operator_t *op = user_data;
	refresh_20 (op->id);
}

static void Operator_Vib_toggled (GtkToggleButton *toggle_button,
                                  gpointer         user_data)
{
	struct operator_t *op = user_data;
	refresh_20 (op->id);
}

static void Operator_AmpMod_toggled (GtkToggleButton *toggle_button,
                                     gpointer         user_data)
{
	struct operator_t *op = user_data;
	refresh_20 (op->id);
}

static void Operator_TotalLevel_value_changed (GtkSpinButton *spin_button,
                                               gpointer       user_data)
{
	struct operator_t *op = user_data;
	refresh_40 (op->id);
}

static void Operator_ScalingLevel_changed (GtkComboBox *combo,
                                           gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_40 (op->id);
}

static void Operator_AttackRate_changed (GtkComboBox *combo,
                                         gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_60 (op->id);
}

static void Operator_DecayRate_changed (GtkComboBox *combo,
                                        gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_60 (op->id);
}

static void Operator_SustainLevel_changed (GtkComboBox *combo,
                                           gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_80 (op->id);
}


static void Operator_ReleaseRate_changed (GtkComboBox *combo,
                                          gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_80 (op->id);
}

static void Channel_FNumber_value_changed (GtkSpinButton *spin_button,
                                           gpointer       user_data)
{
	struct channel_t *ch = user_data;
	refresh_a0 (ch->id);
	refresh_b0 (ch->id);
}

static void Channel_Octave_changed (GtkComboBox *combo,
                                    gpointer     user_data)
{
	struct channel_t *ch = user_data;
	refresh_b0 (ch->id);
}

static void Operator_WaveformSelect_changed (GtkComboBox *combo,
                                             gpointer     user_data)
{
	struct operator_t *op = user_data;
	refresh_e0 (op->id);
}

static void Channel_KeyOn_toggled (GtkComboBox *combo,
                                   gpointer     user_data)
{
	struct channel_t *ch = user_data;
	refresh_b0 (ch->id);
}

static void Channel_Alg_changed (GtkComboBox *combo,
                                 gpointer     user_data)
{
	struct channel_t *ch = user_data;
	refresh_c0 (ch->id);
}


static void Channel_Feedback_changed (GtkComboBox *combo,
                                      gpointer     user_data)
{
	struct channel_t *ch = user_data;
	refresh_c0 (ch->id);
}


static void Global_AMDep_changed (GtkComboBox *combo,
                                  gpointer     user_data)
{
	refresh_bd ();
}

static void Global_VibDep_changed (GtkComboBox *combo,
                                   gpointer     user_data)
{
	refresh_bd ();
}

static void Global_RhyEna_toggled (GtkToggleButton *toggle_button,
                                   gpointer         user_data)
{
	refresh_bd ();
}

static void Global_BD_toggled (GtkToggleButton *toggle_button,
                               gpointer         user_data)
{
	refresh_bd ();
}

static void Global_SD_toggled (GtkToggleButton *toggle_button,
                               gpointer         user_data)
{
	refresh_bd ();
}

static void Global_TOM_toggled (GtkToggleButton *toggle_button,
                                gpointer         user_data)
{
	refresh_bd ();
}


static void Global_TopCym_toggled (GtkToggleButton *toggle_button,
                                   gpointer         user_data)
{
	refresh_bd ();
}


static void Global_HH_toggled (GtkToggleButton *toggle_button,
                               gpointer         user_data)
{
	refresh_bd ();
}

void CreateOperator (int id, int harmonics, int attack, int decay, int sustain, int release)
{
	char text[128];
	GtkWidget *temp;

	operators[id].id = id;

	snprintf (text, sizeof(text), "Operator %d%s%s%s%s", id,
		(id==17)?" (HiHat)":"",
		(id==18)?" (Tom Tom)":"",
		(id==20)?" (Snare Drum)":"",
		(id==21)?" (Top Cym)":""
	);
	operators[id].frame = gtk_frame_new (text);
	gtk_widget_set_hexpand (operators[id].frame, TRUE);
	gtk_widget_set_vexpand (operators[id].frame, TRUE);
	gtk_widget_set_margin_top (operators[id].frame, 10.0);
	gtk_widget_set_margin_bottom (operators[id].frame, 10.0);
	gtk_widget_set_margin_start (operators[id].frame, 10.0);
	gtk_widget_set_margin_end (operators[id].frame, 10.0);

	operators[id].grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (operators[id].frame), operators[id].grid);

	temp = gtk_label_new ("Harmonics");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 0, 2, 1, 1);
	operators[id].ModulatorFrequencyMultiple = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorModulatorFrequencyMultipleModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].ModulatorFrequencyMultiple, 1, 2, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].ModulatorFrequencyMultiple), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].ModulatorFrequencyMultiple), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (harmonics, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorModulatorFrequencyMultipleModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].ModulatorFrequencyMultiple), &iter);
	}
	g_signal_connect(operators[id].ModulatorFrequencyMultiple, "changed", G_CALLBACK(Operator_ModulatorFrequencyMultiple_changed), (gpointer)(operators + id));

	operators[id].KSR = gtk_toggle_button_new_with_label ("ADR Scaling");
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].KSR, 3, 0, 1, 1);
	g_signal_connect (operators[id].KSR, "toggled", G_CALLBACK (Operator_KSR_toggled), (gpointer)(operators + id));

	operators[id].EGTyp = gtk_toggle_button_new_with_label ("Sustain");
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].EGTyp, 2, 0, 1, 1);
	g_signal_connect (operators[id].EGTyp, "toggled", G_CALLBACK (Operator_EGTyp_toggled), (gpointer)(operators + id));

	operators[id].Vib = gtk_toggle_button_new_with_label ("Vibrator");
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].Vib, 1, 0, 1, 1);
	g_signal_connect (operators[id].Vib, "toggled", G_CALLBACK (Operator_Vib_toggled), (gpointer)(operators + id));

	operators[id].AmpMod = gtk_toggle_button_new_with_label ("Amplitude Modulation");
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].AmpMod, 0, 0, 1, 1);
	g_signal_connect (operators[id].AmpMod, "toggled", G_CALLBACK (Operator_AmpMod_toggled), (gpointer)(operators + id));

	gtk_widget_set_hexpand (operators[id].KSR,    TRUE);
	gtk_widget_set_hexpand (operators[id].EGTyp,  TRUE);
	gtk_widget_set_hexpand (operators[id].Vib,    TRUE);
	gtk_widget_set_hexpand (operators[id].AmpMod, TRUE);

	temp = gtk_label_new ("Volume (inverse)");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 0, 1, 1, 1);
	operators[id].TotalLevel = gtk_spin_button_new_with_range (0x00, 0x3f, 0x01);
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].TotalLevel, 1, 1, 1, 1);
	g_signal_connect(operators[id].TotalLevel, "value-changed", G_CALLBACK(Operator_TotalLevel_value_changed), (gpointer)(operators + id));

	temp = gtk_label_new ("Volume Scaling");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 2, 1, 1, 1);
	operators[id].ScalingLevel = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorScalingLevelModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].ScalingLevel, 3, 1, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].ScalingLevel), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].ScalingLevel), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (0, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorScalingLevelModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].ScalingLevel), &iter);
	}
	g_signal_connect(operators[id].ScalingLevel, "changed", G_CALLBACK(Operator_ScalingLevel_changed), (gpointer)(operators + id));

	temp = gtk_label_new ("Waveform");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 2, 2, 1, 1);
	operators[id].WaveformSelect = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorWaveformSelectModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].WaveformSelect, 3, 2, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].WaveformSelect), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].WaveformSelect), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (0, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorWaveformSelectModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].WaveformSelect), &iter);
	}
	g_signal_connect(operators[id].WaveformSelect, "changed", G_CALLBACK(Operator_WaveformSelect_changed), (gpointer)(operators + id));


	temp = gtk_label_new ("Attack Rate");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 0, 3, 1, 1);
	operators[id].AttackRate = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorADRRateModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].AttackRate, 1, 3, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].AttackRate), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].AttackRate), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (attack, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorADRRateModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].AttackRate), &iter);
	}
	g_signal_connect(operators[id].AttackRate, "changed", G_CALLBACK(Operator_AttackRate_changed), (gpointer)(operators + id));

	temp = gtk_label_new ("Decay Rate");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 2, 3, 1, 1);
	operators[id].DecayRate = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorADRRateModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].DecayRate, 3, 3, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].DecayRate), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].DecayRate), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (decay, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorADRRateModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].DecayRate), &iter);
	}
	g_signal_connect(operators[id].DecayRate, "changed", G_CALLBACK(Operator_DecayRate_changed), (gpointer)(operators + id));

	temp = gtk_label_new ("SustainLevel");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 0, 4, 1, 1);
	operators[id].SustainLevel = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorSustainLevelModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].SustainLevel, 1, 4, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].SustainLevel), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].SustainLevel), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (sustain, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorSustainLevelModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].SustainLevel), &iter);
	}
	g_signal_connect(operators[id].SustainLevel, "changed", G_CALLBACK(Operator_SustainLevel_changed), (gpointer)(operators + id));

	temp = gtk_label_new ("Release Rate");
	gtk_grid_attach (GTK_GRID (operators[id].grid), temp, 2, 4, 1, 1);
	operators[id].ReleaseRate = gtk_combo_box_new_with_model (GTK_TREE_MODEL (OperatorADRRateModel));
	gtk_grid_attach (GTK_GRID (operators[id].grid), operators[id].ReleaseRate, 3, 4, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (operators[id].ReleaseRate), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (operators[id].ReleaseRate), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (release, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (OperatorADRRateModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (operators[id].ReleaseRate), &iter);
	}
	g_signal_connect(operators[id].ReleaseRate, "changed", G_CALLBACK(Operator_ReleaseRate_changed), (gpointer)(operators + id));
}

void CreateChannel (int id, int op1, int op2, int fnumber, int octave, int harmonics1, int attack1, int decay1, int sustain1, int release1, int harmonics2, int attack2, int decay2, int sustain2, int release2)
{
	char text[128];
	GtkWidget *temp;

	CreateOperator(op1, harmonics1, attack1, decay1, sustain1, release1);
	CreateOperator(op2, harmonics2, attack2, decay2, sustain2, release2);

	channels[id].id = id;
	channels[id].operator1 = operators + op1;
	channels[id].operator2 = operators + op2;

	snprintf (text, sizeof(text), "Channel %d%s", id+1, id==6?" (Bass Drum)":"");
	channels[id].header = gtk_label_new (text);

	channels[id].grid = gtk_grid_new ();

	channels[id].KeyOn = gtk_toggle_button_new_with_label ("KeyOn");
	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].KeyOn, 0, 0, 4, 1);
	g_signal_connect(channels[id].KeyOn, "toggled", G_CALLBACK(Channel_KeyOn_toggled), (gpointer)(channels + id));

	temp = gtk_label_new ("Algoritm"); gtk_label_set_xalign (GTK_LABEL(temp), 0.0);
	gtk_grid_attach (GTK_GRID (channels[id].grid), temp, 0, 1, 1, 1);
	channels[id].Alg = gtk_combo_box_new_with_model (GTK_TREE_MODEL (ChannelAlgModel));
	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].Alg, 1, 1, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (channels[id].Alg), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (channels[id].Alg), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (0, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (ChannelAlgModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (channels[id].Alg), &iter);
	}
	g_signal_connect(channels[id].Alg, "changed", G_CALLBACK(Channel_Alg_changed), (gpointer)(channels + id));

	temp = gtk_label_new ("First operator feedback"); gtk_label_set_xalign (GTK_LABEL(temp), 0.0);
	gtk_grid_attach (GTK_GRID (channels[id].grid), temp, 2, 1, 1, 1);
	channels[id].Feedback = gtk_combo_box_new_with_model (GTK_TREE_MODEL (ChannelFeedbackModel));
	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].Feedback, 3, 1, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (channels[id].Feedback), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (channels[id].Feedback), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (0, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (ChannelFeedbackModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (channels[id].Feedback), &iter);
	}
	g_signal_connect(channels[id].Feedback, "changed", G_CALLBACK(Channel_Feedback_changed), (gpointer)(channels + id));

	temp = gtk_label_new ("Frequency");
	gtk_grid_attach (GTK_GRID (channels[id].grid), temp, 0, 2, 1, 1);
	channels[id].FNumber = gtk_spin_button_new_with_range (0x000, 0x3ff, 0x001);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (channels[id].FNumber), fnumber);
	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].FNumber, 1, 2, 1, 1);
	g_signal_connect(channels[id].FNumber, "value-changed", G_CALLBACK(Channel_FNumber_value_changed), (gpointer)(channels + id));

	temp = gtk_label_new ("Octave");
	gtk_grid_attach (GTK_GRID (channels[id].grid), temp, 2, 2, 1, 1);
	channels[id].Octave = gtk_combo_box_new_with_model (GTK_TREE_MODEL (ChannelOctaveModel));
	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].Octave, 3, 2, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (channels[id].Octave), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (channels[id].Octave), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (octave, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (ChannelOctaveModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (channels[id].Octave), &iter);
	}
	g_signal_connect(channels[id].Octave, "changed", G_CALLBACK(Channel_Octave_changed), (gpointer)(channels + id));

	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].operator1->frame, 0, 3, 4, 1);
	gtk_grid_attach (GTK_GRID (channels[id].grid), channels[id].operator2->frame, 0, 4, 4, 1);
}

void CreateGlobal ()
{
	char text[128];
	GtkWidget *temp;

	snprintf (text, sizeof(text), "Global");
	global.frame = gtk_frame_new (text);
	gtk_widget_set_margin_top (global.frame, 10.0);
	gtk_widget_set_margin_bottom (global.frame, 10.0);
	gtk_widget_set_margin_start (global.frame, 10.0);
	gtk_widget_set_margin_end (global.frame, 10.0);

	global.grid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (global.frame), global.grid);

	temp = gtk_label_new ("AM depth"); gtk_label_set_xalign (GTK_LABEL(temp), 0.0);
	gtk_grid_attach (GTK_GRID (global.grid), temp, 0, 1, 1, 1);
	global.AMDep = gtk_combo_box_new_with_model (GTK_TREE_MODEL (GlobalAMDepModel));
	gtk_grid_attach (GTK_GRID (global.grid), global.AMDep, 1, 1, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (global.AMDep), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (global.AMDep), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (0, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (GlobalAMDepModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (global.AMDep), &iter);
	}
	g_signal_connect(global.AMDep, "changed", G_CALLBACK(Global_AMDep_changed), 0);

	temp = gtk_label_new ("Vib depth"); gtk_label_set_xalign (GTK_LABEL(temp), 0.0);
	gtk_grid_attach (GTK_GRID (global.grid), temp, 2, 1, 1, 1);
	global.VibDep = gtk_combo_box_new_with_model (GTK_TREE_MODEL (GlobalVibDepModel));
	gtk_grid_attach (GTK_GRID (global.grid), global.VibDep, 3, 1, 1, 1);
	{
		GtkCellRenderer *renderer;
		GtkTreePath *path;
		GtkTreeIter iter;
		renderer = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (global.VibDep), renderer, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (global.VibDep), renderer, "text", 0, NULL);
		path = gtk_tree_path_new_from_indices (0, -1);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (GlobalVibDepModel), &iter, path);
		gtk_tree_path_free (path);
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (global.VibDep), &iter);
	}
	g_signal_connect(global.VibDep, "changed", G_CALLBACK(Global_VibDep_changed), 0);

	global.RhyEna = gtk_toggle_button_new_with_label ("Rhythm Enable");
	gtk_grid_attach (GTK_GRID (global.grid), global.RhyEna, 0, 2, 5, 1);
	g_signal_connect (global.RhyEna, "toggled", G_CALLBACK (Global_RhyEna_toggled), 0);
	gtk_widget_set_hexpand (global.RhyEna, TRUE);

	global.BD = gtk_toggle_button_new_with_label ("Bass Drum");
	gtk_grid_attach (GTK_GRID (global.grid), global.BD, 0, 3, 1, 1);
	g_signal_connect (global.BD, "toggled", G_CALLBACK (Global_BD_toggled), 0);

	global.SD = gtk_toggle_button_new_with_label ("Snare Drum");
	gtk_grid_attach (GTK_GRID (global.grid), global.SD, 1, 3, 1, 1);
	g_signal_connect (global.SD, "toggled", G_CALLBACK (Global_SD_toggled), 0);

	global.TOM = gtk_toggle_button_new_with_label ("Tom Tom");
	gtk_grid_attach (GTK_GRID (global.grid), global.TOM, 2, 3, 1, 1);
	g_signal_connect (global.TOM, "toggled", G_CALLBACK (Global_TOM_toggled), 0);

	global.TopCym = gtk_toggle_button_new_with_label ("Top Cym");
	gtk_grid_attach (GTK_GRID (global.grid), global.TopCym, 3, 3, 1, 1);
	g_signal_connect (global.TopCym, "toggled", G_CALLBACK (Global_TopCym_toggled), 0);

	global.HH = gtk_toggle_button_new_with_label ("Hi Hat");
	gtk_grid_attach (GTK_GRID (global.grid), global.HH, 4, 3, 1, 1);
	g_signal_connect (global.HH, "toggled", G_CALLBACK (Global_HH_toggled), 0);
}

static gboolean graph_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	int i;

	//gdouble dx, dy;
	guint width, height;
	//GdkRectangle da;            /* GtkDrawingArea size */
	GtkAllocation clip;

	width = gtk_widget_get_allocated_width (widget);
	height = gtk_widget_get_allocated_height (widget);

	gtk_widget_get_clip (graph, &clip);

	/* Draw on a black background */
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
	cairo_paint (cr);

	/* Change the transformation matrix */
	cairo_scale (cr, (gdouble)width/SAMPLES_N, ((gdouble)height)/-(256+5.0));
	cairo_translate (cr, 0, -128.0-2.5);

	/* Determine the data points to calculate (ie. those in the clipping zone */
	//cairo_device_to_user_distance (cr, &dx, &dy);
	//cairo_clip_extents (cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);

	//fprintf (stderr, "linewidth: %lf (%lf)\n", dx, dy);
	cairo_set_line_width (cr, 2.0);

	cairo_set_source_rgb (cr, 0.0, 1.0, 0.0);

	cairo_move_to (cr, 0.0, (gdouble)samples[0]/256);
	for (i=0; i < SAMPLES_N; i++)
	{
		cairo_line_to (cr, i, (gdouble)samples[i]/256);
	}

	cairo_stroke (cr);

	return FALSE;
}

#define INPUTLEN 2048
int16_t input[INPUTLEN];
int inputpos = INPUTLEN;

void mainwindow_getsound (void *data, size_t bytes)
{
	int16_t *output = data;

	int count;

	while (bytes)
	{
		if (inputpos >= INPUTLEN)
		{
			int i;
			int smallestvalue=65536;
			int smallestat = -1;

			opl_instance_clock (input, INPUTLEN);
			inputpos = 0;

			for (i=0; i < (INPUTLEN - SAMPLES_N); i++)
			{
				if (input[i] < smallestvalue)
				{
					smallestvalue = input[i];
					smallestat = i;
				}
			}

			if (smallestat >= 0)
			{
				memcpy (samples, input + smallestat, SAMPLES_N*sizeof(int16_t));

				gtk_widget_queue_draw (graph);
			}
		}

		output[1] = output[0] = input [inputpos];

		output += 2;
		bytes -= 4;
		inputpos++;
	}
}

static gboolean window_delete (GtkWidget *object,
                               gpointer   userpointer)
{
	pulse_done ();

	return FALSE;
}

static void
activate (GtkApplication* app,
          gpointer        user_data)
{
	GtkWidget *window, *thegrid, *notebook;
	int i;

	pulse_init ();

	opl_instance_init ();

	OperatorModulatorFrequencyMultipleModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "0 - One octav below", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "1 - same as base frequency", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "2 - One octave above", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "3", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "4 - Two octaves above", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "5", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "6", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "7", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "8 - Three octaves above", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "9", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "10", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "11", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "12", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "13", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "14", -1);
		gtk_list_store_append (OperatorModulatorFrequencyMultipleModel, &iter); gtk_list_store_set (OperatorModulatorFrequencyMultipleModel, &iter, 0, "15", -1);
	}

	OperatorScalingLevelModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (OperatorScalingLevelModel, &iter); gtk_list_store_set (OperatorScalingLevelModel, &iter, 0, "0 - None", -1);
		gtk_list_store_append (OperatorScalingLevelModel, &iter); gtk_list_store_set (OperatorScalingLevelModel, &iter, 0, "1 - 1.5dB/8ve", -1);
		gtk_list_store_append (OperatorScalingLevelModel, &iter); gtk_list_store_set (OperatorScalingLevelModel, &iter, 0, "2 - 3 dB/8ve", -1);
		gtk_list_store_append (OperatorScalingLevelModel, &iter); gtk_list_store_set (OperatorScalingLevelModel, &iter, 0, "3 - 6 db/8ve", -1);
	}

	OperatorADRRateModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "0 - Slowest", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "1", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "2", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "3", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "4", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "5", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "6", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "7", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "8", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "9", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "10", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "11", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "12", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "13", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "14", -1);
		gtk_list_store_append (OperatorADRRateModel, &iter); gtk_list_store_set (OperatorADRRateModel, &iter, 0, "15 - Fastest", -1);
	}

	OperatorSustainLevelModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "0 - Loudest", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "1", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "2", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "3", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "4", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "5", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "6", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "7", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "8", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "9", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "10", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "11", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "12", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "13", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "14", -1);
		gtk_list_store_append (OperatorSustainLevelModel, &iter); gtk_list_store_set (OperatorSustainLevelModel, &iter, 0, "15 - Softest", -1);
	}

	ChannelOctaveModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "0 - Lowest", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "1", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "2", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "3", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "4", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "5", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "6", -1);
		gtk_list_store_append (ChannelOctaveModel, &iter); gtk_list_store_set (ChannelOctaveModel, &iter, 0, "7 - Highest", -1);
	}

	ChannelAlgModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (ChannelAlgModel, &iter); gtk_list_store_set (ChannelAlgModel, &iter, 0, "FM * AM", -1);
		gtk_list_store_append (ChannelAlgModel, &iter); gtk_list_store_set (ChannelAlgModel, &iter, 0, "AM + AM", -1);
	}

	OperatorWaveformSelectModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (OperatorWaveformSelectModel, &iter); gtk_list_store_set (OperatorWaveformSelectModel, &iter, 0, "Sinus", -1);
		gtk_list_store_append (OperatorWaveformSelectModel, &iter); gtk_list_store_set (OperatorWaveformSelectModel, &iter, 0, "Half-Sinus", -1);
		gtk_list_store_append (OperatorWaveformSelectModel, &iter); gtk_list_store_set (OperatorWaveformSelectModel, &iter, 0, "Absolute Sinus", -1);
		gtk_list_store_append (OperatorWaveformSelectModel, &iter); gtk_list_store_set (OperatorWaveformSelectModel, &iter, 0, "Pulse Sinus", -1);
	}


	ChannelFeedbackModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "0 - None", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "1 - Small", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "2", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "3", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "4 - Medium", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "5", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "6", -1);
		gtk_list_store_append (ChannelFeedbackModel, &iter); gtk_list_store_set (ChannelFeedbackModel, &iter, 0, "7 - Most", -1);
	}

	GlobalAMDepModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (GlobalAMDepModel, &iter); gtk_list_store_set (GlobalAMDepModel, &iter, 0, "0 - 1 dB", -1);
		gtk_list_store_append (GlobalAMDepModel, &iter); gtk_list_store_set (GlobalAMDepModel, &iter, 0, "1 - 4.8 dB", -1);
	}

	GlobalVibDepModel = gtk_list_store_new (1, G_TYPE_STRING);
	{
		GtkTreeIter iter;

		gtk_list_store_append (GlobalVibDepModel, &iter); gtk_list_store_set (GlobalVibDepModel, &iter, 0, "0 - 7 cents", -1);
		gtk_list_store_append (GlobalVibDepModel, &iter); gtk_list_store_set (GlobalVibDepModel, &iter, 0, "1 - 14 cents", -1);
	}

	window = gtk_application_window_new (app);
	gtk_window_set_title (GTK_WINDOW (window), "Window");
	gtk_window_set_default_size (GTK_WINDOW (window), 640, 480);
	g_signal_connect (window, "delete-event", G_CALLBACK(window_delete), 0);

	thegrid = gtk_grid_new ();
	gtk_container_add (GTK_CONTAINER (window), thegrid);

	/*                            FNumber Octave Harmonics A D S R Harmonics A D S R  */
	CreateChannel (0, 0x00, 0x03, 100,    3,            1, 5,5,0,5,       1,5,5,0,5);
	CreateChannel (1, 0x01, 0x04, 200,    3,            1, 5,5,0,5,       1,5,5,0,5);
	CreateChannel (2, 0x02, 0x05, 300,    3,            1, 5,5,0,5,       1,5,5,0,5);

	CreateChannel (3, 0x08, 0x0b, 400,    3,            1, 5,5,0,5,       1,5,5,0,5);
	CreateChannel (4, 0x09, 0x0c, 500,    3,            1, 5,5,0,5,       1,5,5,0,5);
	CreateChannel (5, 0x0a, 0x0d, 600,    3,            1, 5,5,0,5,       1,5,5,0,5);

	CreateChannel (6, 0x10, 0x13, 200,    0,            1, 12,5,0,5,      0, 15,5,0,5);
	CreateChannel (7, 0x11, 0x14, 300,    3,            1, 8,5,0,8,       0, 5,5,0,5);
	CreateChannel (8, 0x12, 0x15, 500,    2,            1, 5,5,0,5,       1, 14,3,8,5);

	CreateGlobal();

	notebook = gtk_notebook_new ();

	gtk_grid_attach (GTK_GRID (thegrid), notebook, 0, 0, 1, 2);
	gtk_widget_set_hexpand (notebook, TRUE);
	gtk_widget_set_vexpand (notebook, TRUE);

	for (i=0; i < 9; i++)
	{
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), channels[i].grid, channels[i].header);
	}
	gtk_grid_attach (GTK_GRID (thegrid), global.frame, 0, 2, 1, 1);

	graph = gtk_drawing_area_new ();
	gtk_grid_attach (GTK_GRID (thegrid), graph, 1, 0, 1, 1);
	g_signal_connect (graph, "draw", G_CALLBACK (graph_draw), NULL);

	gtk_widget_set_size_request (graph, 512, 256);
	gtk_widget_set_vexpand (graph, FALSE);

	gtk_widget_show_all (window);

	write_reg (0x01, 0x20); // Enable uniqe WaveselectFunc per FM synth
	for (i=0; i < 9; i++)
	{
		refresh_20(channels[i].operator1->id);
		refresh_20(channels[i].operator2->id);

		refresh_40(channels[i].operator1->id);
		refresh_40(channels[i].operator2->id);

		refresh_60(channels[i].operator1->id);
		refresh_60(channels[i].operator2->id);

		refresh_80(channels[i].operator1->id);
		refresh_80(channels[i].operator2->id);

		refresh_e0(channels[i].operator1->id);
		refresh_e0(channels[i].operator2->id);

		refresh_c0(channels[i].id);
		refresh_a0(channels[i].id);
		refresh_b0(channels[i].id);
	}
}

int
main (int    argc,
      char **argv)
{
	GtkApplication *app;
	int status, i;

	app = gtk_application_new ("mywave.oplsynth", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	opl_instance_done();

	pulse_done ();

	return status;
}
