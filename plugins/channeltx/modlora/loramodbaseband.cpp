///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2020 Edouard Griffiths, F4EXB                                   //
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

#include "dsp/upchannelizer.h"
#include "dsp/dspengine.h"
#include "dsp/dspcommands.h"

#include "loramodbaseband.h"

MESSAGE_CLASS_DEFINITION(LoRaModBaseband::MsgConfigureLoRaModBaseband, Message)
MESSAGE_CLASS_DEFINITION(LoRaModBaseband::MsgConfigureLoRaModPayload, Message)

LoRaModBaseband::LoRaModBaseband() :
    m_mutex(QMutex::Recursive)
{
    m_sampleFifo.resize(SampleSourceFifo::getSizePolicy(48000));
    m_channelizer = new UpChannelizer(&m_source);

    qDebug("LoRaModBaseband::LoRaModBaseband");
    QObject::connect(
        &m_sampleFifo,
        &SampleSourceFifo::dataRead,
        this,
        &LoRaModBaseband::handleData,
        Qt::QueuedConnection
    );

    connect(&m_inputMessageQueue, SIGNAL(messageEnqueued()), this, SLOT(handleInputMessages()));
}

LoRaModBaseband::~LoRaModBaseband()
{
    delete m_channelizer;
}

void LoRaModBaseband::reset()
{
    QMutexLocker mutexLocker(&m_mutex);
    m_sampleFifo.reset();
}

void LoRaModBaseband::pull(const SampleVector::iterator& begin, unsigned int nbSamples)
{
    unsigned int part1Begin, part1End, part2Begin, part2End;
    m_sampleFifo.read(nbSamples, part1Begin, part1End, part2Begin, part2End);
    SampleVector& data = m_sampleFifo.getData();

    if (part1Begin != part1End)
    {
        std::copy(
            data.begin() + part1Begin,
            data.begin() + part1End,
            begin
        );
    }

    unsigned int shift = part1End - part1Begin;

    if (part2Begin != part2End)
    {
        std::copy(
            data.begin() + part2Begin,
            data.begin() + part2End,
            begin + shift
        );
    }
}

void LoRaModBaseband::handleData()
{
    QMutexLocker mutexLocker(&m_mutex);
    SampleVector& data = m_sampleFifo.getData();
    unsigned int ipart1begin;
    unsigned int ipart1end;
    unsigned int ipart2begin;
    unsigned int ipart2end;
    qreal rmsLevel, peakLevel;
    int numSamples;

    unsigned int remainder = m_sampleFifo.remainder();

    while ((remainder > 0) && (m_inputMessageQueue.size() == 0))
    {
        m_sampleFifo.write(remainder, ipart1begin, ipart1end, ipart2begin, ipart2end);

        if (ipart1begin != ipart1end) { // first part of FIFO data
            processFifo(data, ipart1begin, ipart1end);
        }

        if (ipart2begin != ipart2end) { // second part of FIFO data (used when block wraps around)
            processFifo(data, ipart2begin, ipart2end);
        }

        remainder = m_sampleFifo.remainder();
    }

    m_source.getLevels(rmsLevel, peakLevel, numSamples);
    emit levelChanged(rmsLevel, peakLevel, numSamples);
}

void LoRaModBaseband::processFifo(SampleVector& data, unsigned int iBegin, unsigned int iEnd)
{
    m_channelizer->pull(data.begin() + iBegin, iEnd - iBegin);
}

void LoRaModBaseband::handleInputMessages()
{
	Message* message;

	while ((message = m_inputMessageQueue.pop()) != nullptr)
	{
		if (handleMessage(*message)) {
			delete message;
		}
	}
}

bool LoRaModBaseband::handleMessage(const Message& cmd)
{
    if (MsgConfigureLoRaModBaseband::match(cmd))
    {
        qDebug() << "LoRaModBaseband::handleMessage: MsgConfigureLoRaModBaseband";
        QMutexLocker mutexLocker(&m_mutex);
        MsgConfigureLoRaModBaseband& cfg = (MsgConfigureLoRaModBaseband&) cmd;

        applySettings(cfg.getSettings(), cfg.getForce());

        return true;
    }
    else if (MsgConfigureLoRaModPayload::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        MsgConfigureLoRaModPayload& cfg = (MsgConfigureLoRaModPayload&) cmd;
        qDebug() << "LoRaModBaseband::handleMessage: MsgConfigureLoRaModPayload:" << cfg.getPayload().size();
        m_source.setSymbols(cfg.getPayload());

        return true;
    }
    else if (DSPSignalNotification::match(cmd))
    {
        QMutexLocker mutexLocker(&m_mutex);
        DSPSignalNotification& notif = (DSPSignalNotification&) cmd;
        m_sampleFifo.resize(SampleSourceFifo::getSizePolicy(notif.getSampleRate()));
        qDebug() << "LoRaModBaseband::handleMessage: DSPSignalNotification: basebandSampleRate: " << notif.getSampleRate();
        m_channelizer->setBasebandSampleRate(notif.getSampleRate());
        m_source.applyChannelSettings(
            m_channelizer->getChannelSampleRate(),
            LoRaModSettings::bandwidths[m_settings.m_bandwidthIndex],
            m_channelizer->getChannelFrequencyOffset()
        );

		return true;
    }
    else
    {
        return false;
    }
}

void LoRaModBaseband::applySettings(const LoRaModSettings& settings, bool force)
{
    if ((settings.m_bandwidthIndex != m_settings.m_bandwidthIndex)
     || (settings.m_inputFrequencyOffset != m_settings.m_inputFrequencyOffset) || force)
    {
        int thisBW = LoRaModSettings::bandwidths[settings.m_bandwidthIndex];
        m_channelizer->setChannelization(
            thisBW * LoRaModSettings::oversampling,
            settings.m_inputFrequencyOffset
        );
        m_source.applyChannelSettings(
            m_channelizer->getChannelSampleRate(),
            thisBW,
            m_channelizer->getChannelFrequencyOffset()
        );
    }

    m_source.applySettings(settings, force);

    m_settings = settings;
}

int LoRaModBaseband::getChannelSampleRate() const
{
    return m_channelizer->getChannelSampleRate();
}