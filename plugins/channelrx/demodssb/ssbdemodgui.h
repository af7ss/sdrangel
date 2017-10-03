#ifndef INCLUDE_SSBDEMODGUI_H
#define INCLUDE_SSBDEMODGUI_H

#include <plugin/plugininstancegui.h>
#include "gui/rollupwidget.h"
#include "dsp/channelmarker.h"
#include "dsp/movingaverage.h"
#include "util/messagequeue.h"
#include "ssbdemodsettings.h"

class PluginAPI;
class DeviceSourceAPI;

class AudioFifo;
class SSBDemod;
class SpectrumVis;

namespace Ui {
	class SSBDemodGUI;
}

class SSBDemodGUI : public RollupWidget, public PluginInstanceGUI {
	Q_OBJECT

public:
	static SSBDemodGUI* create(PluginAPI* pluginAPI, DeviceSourceAPI *deviceAPI);
	virtual void destroy();

	void setName(const QString& name);
	QString getName() const;
	virtual qint64 getCenterFrequency() const;
	virtual void setCenterFrequency(qint64 centerFrequency);

	void resetToDefaults();
	QByteArray serialize() const;
	bool deserialize(const QByteArray& data);
	virtual MessageQueue *getInputMessageQueue() { return &m_inputMessageQueue; }
	virtual bool handleMessage(const Message& message);

	static const QString m_channelID;

private slots:
	void viewChanged();
	void on_deltaFrequency_changed(qint64 value);
	void on_audioBinaural_toggled(bool binaural);
	void on_audioFlipChannels_toggled(bool flip);
	void on_dsb_toggled(bool dsb);
	void on_BW_valueChanged(int value);
	void on_lowCut_valueChanged(int value);
	void on_volume_valueChanged(int value);
	void on_agc_toggled(bool checked);
    void on_agcClamping_toggled(bool checked);
	void on_agcTimeLog2_valueChanged(int value);
    void on_agcPowerThreshold_valueChanged(int value);
    void on_agcThresholdGate_valueChanged(int value);
	void on_audioMute_toggled(bool checked);
	void on_spanLog2_valueChanged(int value);
	void onWidgetRolled(QWidget* widget, bool rollDown);
	void onMenuDoubleClicked();
	void tick();

private:
	Ui::SSBDemodGUI* ui;
	PluginAPI* m_pluginAPI;
	DeviceSourceAPI* m_deviceAPI;
	ChannelMarker m_channelMarker;
	SSBDemodSettings m_settings;
	bool m_basicSettingsShown;
	bool m_doApplySettings;
	int m_rate;
	int m_spanLog2;
	bool m_audioBinaural;
	bool m_audioFlipChannels;
	bool m_dsb;
	bool m_audioMute;
	bool m_squelchOpen;
	uint32_t m_tickCount;

	SSBDemod* m_ssbDemod;
	SpectrumVis* m_spectrumVis;
	MessageQueue m_inputMessageQueue;

	explicit SSBDemodGUI(PluginAPI* pluginAPI, DeviceSourceAPI* deviceAPI, QWidget* parent = NULL);
	virtual ~SSBDemodGUI();

	int  getEffectiveLowCutoff(int lowCutoff);
	bool setNewRate(int spanLog2);

    void blockApplySettings(bool block);
	void applySettings(bool force = false);
	void displaySettings();
    void displayUDPAddress();
    void updateChannelMarker();

	void displayAGCPowerThreshold(int value);

	void leaveEvent(QEvent*);
	void enterEvent(QEvent*);
};

#endif // INCLUDE_SSBDEMODGUI_H
