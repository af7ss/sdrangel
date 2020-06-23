///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019 Edouard Griffiths, F4EXB                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////


#include <QDebug>

#include <QTime>
#include <QDateTime>
#include <QString>
#include <QMessageBox>
#include <QFileDialog>

#include "plugin/pluginapi.h"
#include "device/deviceapi.h"
#include "device/deviceuiset.h"
#include "gui/colormapper.h"
#include "gui/glspectrum.h"
#include "gui/crightclickenabler.h"
#include "gui/basicdevicesettingsdialog.h"
#include "dsp/dspengine.h"
#include "dsp/dspdevicemimoengine.h"
#include "dsp/dspcommands.h"
#include "dsp/devicesamplestatic.h"
#include "util/db.h"

#include "mainwindow.h"

#include "bladerf2mimo.h"
#include "ui_bladerf2mimogui.h"
#include "bladerf2mimogui.h"

BladeRF2MIMOGui::BladeRF2MIMOGui(DeviceUISet *deviceUISet, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::BladeRF2MIMOGui),
    m_deviceUISet(deviceUISet),
    m_settings(),
    m_rxElseTx(true),
    m_streamIndex(0),
    m_spectrumRxElseTx(true),
    m_spectrumStreamIndex(0),
    m_gainLock(false),
    m_doApplySettings(true),
    m_forceSettings(true),
    m_sampleMIMO(nullptr),
    m_tickCount(0),
    m_rxBasebandSampleRate(3072000),
    m_txBasebandSampleRate(3072000),
    m_rxDeviceCenterFrequency(435000*1000),
    m_txDeviceCenterFrequency(435000*1000),
    m_lastRxEngineState(DeviceAPI::StNotStarted),
    m_lastTxEngineState(DeviceAPI::StNotStarted),
    m_sampleRateMode(true)
{
    qDebug("BladeRF2MIMOGui::BladeRF2MIMOGui");
    ui->setupUi(this);
    m_sampleMIMO = (BladeRF2MIMO*) m_deviceUISet->m_deviceAPI->getSampleMIMO();

    m_sampleMIMO->getRxFrequencyRange(m_fMinRx, m_fMaxRx, m_fStepRx);
    m_sampleMIMO->getTxFrequencyRange(m_fMinTx, m_fMaxTx, m_fStepTx);
    m_sampleMIMO->getRxBandwidthRange(m_bwMinRx, m_bwMaxRx, m_bwStepRx);
    m_sampleMIMO->getTxBandwidthRange(m_bwMinTx, m_bwMaxTx, m_bwStepTx);

    ui->centerFrequency->setColorMapper(ColorMapper(ColorMapper::GrayGold));
    ui->sampleRate->setColorMapper(ColorMapper(ColorMapper::GrayGreenYellow));
    ui->bandwidth->setColorMapper(ColorMapper(ColorMapper::GrayYellow));

    int minRx, maxRx, stepRx, minTx, maxTx, stepTx;
    m_sampleMIMO->getRxSampleRateRange(minRx, maxRx, stepRx);
    m_sampleMIMO->getTxSampleRateRange(minTx, maxTx, stepTx);
    m_srMin = std::max(minRx, minTx);
    m_srMax = std::min(maxRx, maxTx);

    displayGainModes();
    displaySettings();

    connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(updateHardware()));
    connect(&m_statusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    m_statusTimer.start(500);

    connect(&m_inputMessageQueue, SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()), Qt::QueuedConnection);
    m_sampleMIMO->setMessageQueueToGUI(&m_inputMessageQueue);

    CRightClickEnabler *startStopRightClickEnabler = new CRightClickEnabler(ui->startStopRx);
    connect(startStopRightClickEnabler, SIGNAL(rightClick(const QPoint &)), this, SLOT(openDeviceSettingsDialog(const QPoint &)));

    CRightClickEnabler *fileRecordRightClickEnabler = new CRightClickEnabler(ui->record);
    connect(fileRecordRightClickEnabler, SIGNAL(rightClick(const QPoint &)), this, SLOT(openFileRecordDialog(const QPoint &)));

    sendSettings();
}

BladeRF2MIMOGui::~BladeRF2MIMOGui()
{
    delete ui;
}

void BladeRF2MIMOGui::destroy()
{
    delete this;
}

void BladeRF2MIMOGui::setName(const QString& name)
{
    setObjectName(name);
}

QString BladeRF2MIMOGui::getName() const
{
    return objectName();
}

void BladeRF2MIMOGui::resetToDefaults()
{
    m_settings.resetToDefaults();
    displaySettings();
    sendSettings();
}

qint64 BladeRF2MIMOGui::getCenterFrequency() const
{
    return m_settings.m_rxCenterFrequency;
}

void BladeRF2MIMOGui::setCenterFrequency(qint64 centerFrequency)
{
    m_settings.m_rxCenterFrequency = centerFrequency;
    displaySettings();
    sendSettings();
}

QByteArray BladeRF2MIMOGui::serialize() const
{
    return m_settings.serialize();
}

bool BladeRF2MIMOGui::deserialize(const QByteArray& data)
{
    if (m_settings.deserialize(data))
    {
        displaySettings();
        m_forceSettings = true;
        sendSettings();
        return true;
    }
    else
    {
        resetToDefaults();
        return false;
    }
}

void BladeRF2MIMOGui::displaySettings()
{
    updateFrequencyLimits();

    if (m_rxElseTx)
    {
        ui->transverter->setDeltaFrequency(m_settings.m_rxTransverterDeltaFrequency);
        ui->transverter->setDeltaFrequencyActive(m_settings.m_rxTransverterMode);
        ui->transverter->setIQOrder(m_settings.m_iqOrder);
        ui->centerFrequency->setValueRange(7, m_fMinRx / 1000, m_fMaxRx / 1000);
        ui->centerFrequency->setValue(m_settings.m_rxCenterFrequency / 1000);
        ui->bandwidth->setValueRange(5, m_bwMinRx / 1000, m_bwMaxRx / 1000);
        ui->bandwidth->setValue(m_settings.m_rxBandwidth / 1000);
        ui->record->setEnabled(true);
        uint32_t basebandSampleRate = m_settings.m_devSampleRate/(1<<m_settings.m_log2Decim);
        ui->deviceRateText->setText(tr("%1k").arg(QString::number(basebandSampleRate / 1000.0f, 'g', 5)));
        ui->dcOffset->setEnabled(true);
        ui->dcOffset->setChecked(m_settings.m_dcBlock);
        ui->iqImbalance->setEnabled(true);
        ui->iqImbalance->setChecked(m_settings.m_iqCorrection);
        ui->biasTee->setChecked(m_settings.m_rxBiasTee);
        ui->decim->setCurrentIndex(m_settings.m_log2Decim);
        ui->label_decim->setText(QString("Dec"));
        ui->decim->setToolTip(QString("Decimation factor"));
        ui->gainMode->setEnabled(true);
        ui->fcPos->setCurrentIndex((int) m_settings.m_fcPosRx);

        if (m_streamIndex == 0)
        {
            ui->gainMode->setCurrentIndex(m_settings.m_rx0GainMode);
            ui->gainText->setText(tr("%1 dB").arg(m_settings.m_rx0GlobalGain));
            ui->gain->setValue(m_settings.m_rx0GlobalGain);
        }
        else if (m_streamIndex == 1)
        {
            ui->gainMode->setCurrentIndex(m_settings.m_rx1GainMode);
            ui->gainText->setText(tr("%1 dB").arg(m_settings.m_rx1GlobalGain));
            ui->gain->setValue(m_settings.m_rx1GlobalGain);
        }
    }
    else
    {
        ui->transverter->setDeltaFrequency(m_settings.m_txTransverterDeltaFrequency);
        ui->transverter->setDeltaFrequencyActive(m_settings.m_txTransverterMode);
        ui->transverter->setIQOrder(m_settings.m_iqOrder);
        ui->centerFrequency->setValueRange(7, m_fMinTx / 1000, m_fMaxTx / 1000);
        ui->centerFrequency->setValue(m_settings.m_txCenterFrequency / 1000);
        ui->bandwidth->setValueRange(5, m_bwMinTx / 1000, m_bwMaxTx / 1000);
        ui->bandwidth->setValue(m_settings.m_txBandwidth / 1000);
        ui->record->setEnabled(false);
        uint32_t basebandSampleRate = m_settings.m_devSampleRate/(1<<m_settings.m_log2Interp);
        ui->deviceRateText->setText(tr("%1k").arg(QString::number(basebandSampleRate / 1000.0f, 'g', 5)));
        ui->dcOffset->setEnabled(false);
        ui->iqImbalance->setEnabled(false);
        ui->biasTee->setChecked(m_settings.m_txBiasTee);
        ui->decim->setCurrentIndex(m_settings.m_log2Interp);
        ui->label_decim->setText(QString("Int"));
        ui->decim->setToolTip(QString("Interpolation factor"));
        ui->gainMode->setEnabled(false);
        ui->fcPos->setCurrentIndex((int) m_settings.m_fcPosTx);

        if (m_streamIndex == 0)
        {
            ui->gainText->setText(tr("%1 dB").arg(m_settings.m_tx0GlobalGain));
            ui->gain->setValue(m_settings.m_tx0GlobalGain);
        }
        else if (m_streamIndex == 1)
        {
            ui->gainText->setText(tr("%1 dB").arg(m_settings.m_tx1GlobalGain));
            ui->gain->setValue(m_settings.m_tx1GlobalGain);
        }
    }

    ui->sampleRate->setValue(m_settings.m_devSampleRate);
    ui->LOppm->setValue(m_settings.m_LOppmTenths);
    ui->LOppmText->setText(QString("%1").arg(QString::number(m_settings.m_LOppmTenths/10.0, 'f', 1)));

    displaySampleRate();
}

void BladeRF2MIMOGui::displaySampleRate()
{
    ui->sampleRate->blockSignals(true);
    displayFcTooltip();
    quint32 log2Factor = m_rxElseTx ? m_settings.m_log2Decim : m_settings.m_log2Interp;

    if (m_sampleRateMode)
    {
        ui->sampleRateMode->setStyleSheet("QToolButton { background:rgb(60,60,60); }");
        ui->sampleRateMode->setText("SR");
        // BladeRF can go as low as 80 kS/s but because of buffering in practice experience is not good below 330 kS/s
        ui->sampleRate->setValueRange(8, m_srMin, m_srMax);
        ui->sampleRate->setValue(m_settings.m_devSampleRate);
        ui->sampleRate->setToolTip("Device to host sample rate (S/s)");
        ui->deviceRateText->setToolTip("Baseband sample rate (S/s)");
        uint32_t basebandSampleRate = m_settings.m_devSampleRate/(1<<log2Factor);
        ui->deviceRateText->setText(tr("%1k").arg(QString::number(basebandSampleRate / 1000.0f, 'g', 5)));
    }
    else
    {
        ui->sampleRateMode->setStyleSheet("QToolButton { background:rgb(50,50,50); }");
        ui->sampleRateMode->setText("BB");
        // BladeRF can go as low as 80 kS/s but because of buffering in practice experience is not good below 330 kS/s
        ui->sampleRate->setValueRange(8, m_srMin/(1<<log2Factor), m_srMax/(1<<log2Factor));
        ui->sampleRate->setValue(m_settings.m_devSampleRate/(1<<log2Factor));
        ui->sampleRate->setToolTip("Baseband sample rate (S/s)");
        ui->deviceRateText->setToolTip("Device to host sample rate (S/s)");
        ui->deviceRateText->setText(tr("%1k").arg(QString::number(m_settings.m_devSampleRate / 1000.0f, 'g', 5)));
    }

    ui->sampleRate->blockSignals(false);
}

void BladeRF2MIMOGui::displayFcTooltip()
{
    int32_t fShift;

    if (m_rxElseTx)
    {
        fShift = DeviceSampleStatic::calculateSourceFrequencyShift(
            m_settings.m_log2Decim,
            (DeviceSampleStatic::fcPos_t) m_settings.m_fcPosRx,
            m_settings.m_devSampleRate,
            DeviceSampleStatic::FrequencyShiftScheme::FSHIFT_STD
        );
    }
    else
    {
        fShift = DeviceSampleStatic::calculateSinkFrequencyShift(
            m_settings.m_log2Interp,
            (DeviceSampleStatic::fcPos_t) m_settings.m_fcPosTx,
            m_settings.m_devSampleRate
        );
    }

    ui->fcPos->setToolTip(tr("Relative position of device center frequency: %1 kHz").arg(QString::number(fShift / 1000.0f, 'g', 5)));
}

void BladeRF2MIMOGui::displayGainModes()
{
    ui->gainMode->blockSignals(true);

    if (m_rxElseTx)
    {
        const std::vector<BladeRF2MIMO::GainMode>& modes = m_sampleMIMO->getRxGainModes();
        std::vector<BladeRF2MIMO::GainMode>::const_iterator it = modes.begin();

        for (; it != modes.end(); ++it) {
            ui->gainMode->addItem(it->m_name);
        }
    }
    else
    {
        ui->gainMode->clear();
        ui->gainMode->addItem("automatic");
    }

    ui->gainMode->blockSignals(false);
}

bool BladeRF2MIMOGui::handleMessage(const Message& message)
{
    if (DSPMIMOSignalNotification::match(message))
    {
        const DSPMIMOSignalNotification& notif = (const DSPMIMOSignalNotification&) message;
        int istream = notif.getIndex();
        bool sourceOrSink = notif.getSourceOrSink();

        if (sourceOrSink)
        {
            m_rxBasebandSampleRate = notif.getSampleRate();
            m_rxDeviceCenterFrequency = notif.getCenterFrequency();
        }
        else
        {
            m_txBasebandSampleRate = notif.getSampleRate();
            m_txDeviceCenterFrequency = notif.getCenterFrequency();
        }

        qDebug("BladeRF2MIMOGui::handleInputMessages: DSPMIMOSignalNotification: %s stream: %d SampleRate:%d, CenterFrequency:%llu",
                sourceOrSink ? "source" : "sink",
                istream,
                notif.getSampleRate(),
                notif.getCenterFrequency());

        updateSampleRateAndFrequency();

        return true;
    }
    else if (BladeRF2MIMO::MsgConfigureBladeRF2MIMO::match(message))
    {
        const BladeRF2MIMO::MsgConfigureBladeRF2MIMO& notif = (const BladeRF2MIMO::MsgConfigureBladeRF2MIMO&) message;
        m_settings = notif.getSettings();
        displaySettings();

        return true;
    }

    return false;
}

void BladeRF2MIMOGui::handleInputMessages()
{
    Message* message;

    while ((message = m_inputMessageQueue.pop()) != 0)
    {
        if (handleMessage(*message)) {
            delete message;
        } else {
            qDebug("BladeRF2MIMOGui::handleInputMessages: unhandled message: %s", message->getIdentifier());
        }
    }
}

void BladeRF2MIMOGui::sendSettings()
{
    if(!m_updateTimer.isActive()) {
        m_updateTimer.start(100);
    }
}

void BladeRF2MIMOGui::updateHardware()
{
    if (m_doApplySettings)
    {
        BladeRF2MIMO::MsgConfigureBladeRF2MIMO* message = BladeRF2MIMO::MsgConfigureBladeRF2MIMO::create(m_settings, m_forceSettings);
        m_sampleMIMO->getInputMessageQueue()->push(message);
        m_forceSettings = false;
        m_updateTimer.stop();
    }
}

void BladeRF2MIMOGui::updateSampleRateAndFrequency()
{
    if (m_spectrumRxElseTx)
    {
        m_deviceUISet->getSpectrum()->setSampleRate(m_rxBasebandSampleRate);
        m_deviceUISet->getSpectrum()->setCenterFrequency(m_rxDeviceCenterFrequency);
    }
    else
    {
        m_deviceUISet->getSpectrum()->setSampleRate(m_txBasebandSampleRate);
        m_deviceUISet->getSpectrum()->setCenterFrequency(m_txDeviceCenterFrequency);
    }
}

void BladeRF2MIMOGui::updateFileRecordStatus()
{
    if (m_sampleMIMO->isRecording(m_streamIndex)) {
        ui->record->setStyleSheet("QToolButton { background-color : red; }");
    } else {
        ui->record->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
    }
}

void BladeRF2MIMOGui::on_streamSide_currentIndexChanged(int index)
{
    m_rxElseTx = index == 0;
    displayGainModes();
    displaySettings();
}

void BladeRF2MIMOGui::on_streamIndex_currentIndexChanged(int index)
{
    m_streamIndex = index < 0 ? 0 : index > 1 ? 1 : index;
    displaySettings();
}

void BladeRF2MIMOGui::on_spectrumSide_currentIndexChanged(int index)
{
    m_spectrumRxElseTx = (index == 0);
    m_deviceUISet->m_spectrum->setDisplayedStream(m_spectrumRxElseTx, m_spectrumStreamIndex);
    m_deviceUISet->m_deviceAPI->setSpectrumSinkInput(m_spectrumRxElseTx, m_spectrumStreamIndex);
    m_deviceUISet->setSpectrumScalingFactor(m_spectrumRxElseTx ? SDR_RX_SCALEF : SDR_TX_SCALEF);
    updateSampleRateAndFrequency();
}

void BladeRF2MIMOGui::on_spectrumIndex_currentIndexChanged(int index)
{
    m_spectrumStreamIndex = index < 0 ? 0 : index > 1 ? 1 : index;
    m_deviceUISet->m_spectrum->setDisplayedStream(m_spectrumRxElseTx, m_spectrumStreamIndex);
    m_deviceUISet->m_deviceAPI->setSpectrumSinkInput(m_spectrumRxElseTx, m_spectrumStreamIndex);
    updateSampleRateAndFrequency();
}

void BladeRF2MIMOGui::on_startStopRx_toggled(bool checked)
{
    if (m_doApplySettings)
    {
        BladeRF2MIMO::MsgStartStop *message = BladeRF2MIMO::MsgStartStop::create(checked, true);
        m_sampleMIMO->getInputMessageQueue()->push(message);
    }
}

void BladeRF2MIMOGui::on_startStopTx_toggled(bool checked)
{
    if (m_doApplySettings)
    {
        BladeRF2MIMO::MsgStartStop *message = BladeRF2MIMO::MsgStartStop::create(checked, false);
        m_sampleMIMO->getInputMessageQueue()->push(message);
    }
}

void BladeRF2MIMOGui::on_record_toggled(bool checked)
{
    if (checked) {
        ui->record->setStyleSheet("QToolButton { background-color : red; }");
    } else {
        ui->record->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
    }

    BladeRF2MIMO::MsgFileRecord* message = BladeRF2MIMO::MsgFileRecord::create(checked, m_streamIndex);
    m_sampleMIMO->getInputMessageQueue()->push(message);
}

void BladeRF2MIMOGui::on_centerFrequency_changed(quint64 value)
{
    if (m_rxElseTx) {
        m_settings.m_rxCenterFrequency = value * 1000;
    } else {
        m_settings.m_txCenterFrequency = value * 1000;
    }

    sendSettings();
}

void BladeRF2MIMOGui::on_LOppm_valueChanged(int value)
{
    ui->LOppmText->setText(QString("%1").arg(QString::number(value/10.0, 'f', 1)));
    m_settings.m_LOppmTenths = value;
    sendSettings();
}

void BladeRF2MIMOGui::on_dcOffset_toggled(bool checked)
{
    m_settings.m_dcBlock = checked;
    sendSettings();
}

void BladeRF2MIMOGui::on_iqImbalance_toggled(bool checked)
{
    m_settings.m_iqCorrection = checked;
    sendSettings();
}

void BladeRF2MIMOGui::on_bandwidth_changed(quint64 value)
{
    if (m_rxElseTx) {
        m_settings.m_rxBandwidth = value * 1000;
    } else {
        m_settings.m_txBandwidth = value * 1000;
    }

    sendSettings();
}

void BladeRF2MIMOGui::on_sampleRate_changed(quint64 value)
{
    if (m_sampleRateMode)
    {
        m_settings.m_devSampleRate = value;
    }
    else
    {
        if (m_rxElseTx) {
            m_settings.m_devSampleRate = value * (1 << m_settings.m_log2Decim);
        } else {
            m_settings.m_devSampleRate = value * (1 << m_settings.m_log2Interp);
        }
    }

    displaySampleRate();
    displayFcTooltip();
    sendSettings();
}

void BladeRF2MIMOGui::on_fcPos_currentIndexChanged(int index)
{
    if (m_rxElseTx) {
        m_settings.m_fcPosRx = (BladeRF2MIMOSettings::fcPos_t) (index < 0 ? 0 : index > 2 ? 2 : index);
    } else {
        m_settings.m_fcPosTx = (BladeRF2MIMOSettings::fcPos_t) (index < 0 ? 0 : index > 2 ? 2 : index);
    }

    displayFcTooltip();
    sendSettings();
}

void BladeRF2MIMOGui::on_decim_currentIndexChanged(int index)
{
    if ((index <0) || (index > 6)) {
        return;
    }

    if (m_rxElseTx) {
        m_settings.m_log2Decim = index;
    } else {
        m_settings.m_log2Interp = index;
    }

    displaySampleRate();

    if (m_sampleRateMode) {
        m_settings.m_devSampleRate = ui->sampleRate->getValueNew();
    } else {
        m_settings.m_devSampleRate = ui->sampleRate->getValueNew() * (1 << (m_rxElseTx ? m_settings.m_log2Decim : m_settings.m_log2Interp));
    }

    sendSettings();
}

void BladeRF2MIMOGui::on_gainLock_toggled(bool checked)
{
    if (!m_gainLock && checked)
    {
        m_settings.m_rx1GlobalGain = m_settings.m_rx0GlobalGain;
        m_settings.m_rx1GainMode = m_settings.m_rx0GainMode;
        m_settings.m_tx1GlobalGain = m_settings.m_tx0GlobalGain;
        sendSettings();
    }

    m_gainLock = checked;
}

void BladeRF2MIMOGui::on_gainMode_currentIndexChanged(int index)
{
    if (!m_rxElseTx) { // not for Tx
        return;
    }

    const std::vector<BladeRF2MIMO::GainMode>& modes = m_sampleMIMO->getRxGainModes();
    unsigned int uindex = index < 0 ? 0 : (unsigned int) index;

    if (uindex < modes.size())
    {
        BladeRF2MIMO::GainMode mode = modes[index];

        if (m_streamIndex == 0 || m_gainLock)
        {
            if (m_settings.m_rx0GainMode != mode.m_value)
            {
                if (mode.m_value == BLADERF_GAIN_MANUAL)
                {
                    m_settings.m_rx0GlobalGain = ui->gain->value();
                    ui->gain->setEnabled(true);
                } else {
                    ui->gain->setEnabled(false);
                }
            }

            m_settings.m_rx0GainMode = mode.m_value;
        }

        if (m_streamIndex == 1 || m_gainLock)
        {
            if (m_settings.m_rx1GainMode != mode.m_value)
            {
                if (mode.m_value == BLADERF_GAIN_MANUAL)
                {
                    m_settings.m_rx1GlobalGain = ui->gain->value();
                    ui->gain->setEnabled(true);
                } else {
                    ui->gain->setEnabled(false);
                }
            }

            m_settings.m_rx1GainMode = mode.m_value;
        }

        sendSettings();
    }
}

void BladeRF2MIMOGui::on_gain_valueChanged(int value)
{
    ui->gainText->setText(tr("%1 dB").arg(value));

    if (m_rxElseTx)
    {
        if (m_streamIndex == 0 || m_gainLock) {
            m_settings.m_rx0GlobalGain = value;
        }
        if (m_streamIndex == 1 || m_gainLock) {
            m_settings.m_rx1GlobalGain = value;
        }
    }
    else
    {
        if (m_streamIndex == 0 || m_gainLock) {
            m_settings.m_tx0GlobalGain = value;
        }
        if (m_streamIndex == 1 || m_gainLock) {
            m_settings.m_tx1GlobalGain = value;
        }
    }

    sendSettings();
}

void BladeRF2MIMOGui::on_biasTee_toggled(bool checked)
{
    if (m_rxElseTx) {
        m_settings.m_rxBiasTee = checked;
    } else {
        m_settings.m_txBiasTee = checked;
    }

    sendSettings();
}

void BladeRF2MIMOGui::on_transverter_clicked()
{
    if (m_rxElseTx)
    {
        m_settings.m_rxTransverterMode = ui->transverter->getDeltaFrequencyAcive();
        m_settings.m_rxTransverterDeltaFrequency = ui->transverter->getDeltaFrequency();
        m_settings.m_iqOrder = ui->transverter->getIQOrder();
        qDebug("BladeRF2InputGui::on_transverter_clicked: Rx: %lld Hz %s", m_settings.m_rxTransverterDeltaFrequency, m_settings.m_rxTransverterMode ? "on" : "off");
    }
    else
    {
        m_settings.m_txTransverterMode = ui->transverter->getDeltaFrequencyAcive();
        m_settings.m_txTransverterDeltaFrequency = ui->transverter->getDeltaFrequency();
        qDebug("BladeRF2InputGui::on_transverter_clicked: Tx: %lld Hz %s", m_settings.m_txTransverterDeltaFrequency, m_settings.m_txTransverterMode ? "on" : "off");
    }

    updateFrequencyLimits();
    setCenterFrequencySetting(ui->centerFrequency->getValueNew());
    sendSettings();
}

void BladeRF2MIMOGui::updateFrequencyLimits()
{
    // values in kHz
    uint64_t f_min, f_max;
    int step;

    if (m_rxElseTx)
    {
        qint64 deltaFrequency = m_settings.m_rxTransverterMode ? m_settings.m_rxTransverterDeltaFrequency/1000 : 0;
        m_sampleMIMO->getRxFrequencyRange(f_min, f_max, step);
        qint64 minLimit = f_min/1000 + deltaFrequency;
        qint64 maxLimit = f_max/1000 + deltaFrequency;

        minLimit = minLimit < 0 ? 0 : minLimit > 9999999 ? 9999999 : minLimit;
        maxLimit = maxLimit < 0 ? 0 : maxLimit > 9999999 ? 9999999 : maxLimit;

        qDebug("BladeRF2MIMOGui::updateFrequencyLimits: Rx: delta: %lld min: %lld max: %lld", deltaFrequency, minLimit, maxLimit);

        ui->centerFrequency->setValueRange(7, minLimit, maxLimit);
    }
    else
    {
        qint64 deltaFrequency = m_settings.m_txTransverterMode ? m_settings.m_txTransverterDeltaFrequency/1000 : 0;
        m_sampleMIMO->getRxFrequencyRange(f_min, f_max, step);
        qint64 minLimit = f_min/1000 + deltaFrequency;
        qint64 maxLimit = f_max/1000 + deltaFrequency;

        minLimit = minLimit < 0 ? 0 : minLimit > 9999999 ? 9999999 : minLimit;
        maxLimit = maxLimit < 0 ? 0 : maxLimit > 9999999 ? 9999999 : maxLimit;

        qDebug("BladeRF2MIMOGui::updateFrequencyLimits: Rx: delta: %lld min: %lld max: %lld", deltaFrequency, minLimit, maxLimit);

        ui->centerFrequency->setValueRange(7, minLimit, maxLimit);
    }
}

void BladeRF2MIMOGui::setCenterFrequencySetting(uint64_t kHzValue)
{
    int64_t centerFrequency = kHzValue*1000;

    if (m_rxElseTx) {
        m_settings.m_rxCenterFrequency = centerFrequency < 0 ? 0 : (uint64_t) centerFrequency;
    } else {
        m_settings.m_txCenterFrequency = centerFrequency < 0 ? 0 : (uint64_t) centerFrequency;
    }

    ui->centerFrequency->setToolTip(QString("Main center frequency in kHz (LO: %1 kHz)").arg(centerFrequency/1000));
}

void BladeRF2MIMOGui::updateStatus()
{
    int stateRx = m_deviceUISet->m_deviceAPI->state(0);
    int stateTx = m_deviceUISet->m_deviceAPI->state(1);

    if (m_lastRxEngineState != stateRx)
    {
        qDebug("BladeRF2MIMOGui::updateStatus: stateRx: %d", (int) stateRx);
        switch(stateRx)
        {
            case DeviceAPI::StNotStarted:
                ui->startStopRx->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
                break;
            case DeviceAPI::StIdle:
                ui->startStopRx->setStyleSheet("QToolButton { background-color : blue; }");
                break;
            case DeviceAPI::StRunning:
                ui->startStopRx->setStyleSheet("QToolButton { background-color : green; }");
                break;
            case DeviceAPI::StError:
                ui->startStopRx->setStyleSheet("QToolButton { background-color : red; }");
                QMessageBox::information(this, tr("Message"), m_deviceUISet->m_deviceAPI->errorMessage(0));
                break;
            default:
                break;
        }

        m_lastRxEngineState = stateRx;
    }

    if (m_lastTxEngineState != stateTx)
    {
        qDebug("BladeRF2MIMOGui::updateStatus: stateTx: %d", (int) stateTx);
        switch(stateTx)
        {
            case DeviceAPI::StNotStarted:
                ui->startStopTx->setStyleSheet("QToolButton { background:rgb(79,79,79); }");
                break;
            case DeviceAPI::StIdle:
                ui->startStopTx->setStyleSheet("QToolButton { background-color : blue; }");
                break;
            case DeviceAPI::StRunning:
                ui->startStopTx->setStyleSheet("QToolButton { background-color : green; }");
                break;
            case DeviceAPI::StError:
                ui->startStopTx->setStyleSheet("QToolButton { background-color : red; }");
                QMessageBox::information(this, tr("Message"), m_deviceUISet->m_deviceAPI->errorMessage(1));
                break;
            default:
                break;
        }

        m_lastTxEngineState = stateTx;
    }
}

void BladeRF2MIMOGui::openDeviceSettingsDialog(const QPoint& p)
{
    BasicDeviceSettingsDialog dialog(this);
    dialog.setUseReverseAPI(m_settings.m_useReverseAPI);
    dialog.setReverseAPIAddress(m_settings.m_reverseAPIAddress);
    dialog.setReverseAPIPort(m_settings.m_reverseAPIPort);
    dialog.setReverseAPIDeviceIndex(m_settings.m_reverseAPIDeviceIndex);

    dialog.move(p);
    dialog.exec();

    m_settings.m_useReverseAPI = dialog.useReverseAPI();
    m_settings.m_reverseAPIAddress = dialog.getReverseAPIAddress();
    m_settings.m_reverseAPIPort = dialog.getReverseAPIPort();
    m_settings.m_reverseAPIDeviceIndex = dialog.getReverseAPIDeviceIndex();

    sendSettings();
}

void BladeRF2MIMOGui::openFileRecordDialog(const QPoint& p)
{
    QFileDialog fileDialog(
        this,
        tr("Save I/Q record file"),
        m_settings.m_fileRecordName,
        tr("SDR I/Q Files (*.sdriq)")
    );

    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.move(p);
    QStringList fileNames;

    if (fileDialog.exec())
    {
        fileNames = fileDialog.selectedFiles();

        if (fileNames.size() > 0)
        {
            m_settings.m_fileRecordName = fileNames.at(0);
            sendSettings();
        }
    }
}
