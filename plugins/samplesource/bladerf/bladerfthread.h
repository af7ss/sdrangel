///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 maintech GmbH, Otto-Hahn-Str. 15, 97204 Hoechberg, Germany //
// written by Christian Daniel                                                   //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_BLADERFTHREAD_H
#define INCLUDE_BLADERFTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <libbladeRF.h>
#include "dsp/samplefifo.h"
#include "dsp/inthalfbandfilter.h"

#define BLADERF_BLOCKSIZE 16384

class BladerfThread : public QThread {
	Q_OBJECT

public:
	BladerfThread(struct bladerf* dev, SampleFifo* sampleFifo, QObject* parent = NULL);
	~BladerfThread();

	void startWork();
	void stopWork();
	void setSamplerate(int samplerate);
	void setLog2Decimation(unsigned int log2_decim);

private:
	QMutex m_startWaitMutex;
	QWaitCondition m_startWaiter;
	bool m_running;

	struct bladerf* m_dev;
	qint16 m_buf[2*BLADERF_BLOCKSIZE];
	SampleVector m_convertBuffer;
	SampleFifo* m_sampleFifo;

	int m_samplerate;
	unsigned int m_log2Decim;

	IntHalfbandFilter m_decimator2; // 1st stages
	IntHalfbandFilter m_decimator4; // 2nd stages
	IntHalfbandFilter m_decimator8; // 3rd stages

	void run();
	void decimate1(SampleVector::iterator* it, const qint16* buf, qint32 len);
	void decimate2_u(SampleVector::iterator* it, const quint16* buf, qint32 len);
	void decimate2(SampleVector::iterator* it, const qint16* buf, qint32 len);
	void decimate4(SampleVector::iterator* it, const qint16* buf, qint32 len);
	void decimate8(SampleVector::iterator* it, const qint16* buf, qint32 len);
	void decimate16(SampleVector::iterator* it, const qint16* buf, qint32 len);
	void decimate32(SampleVector::iterator* it, const qint16* buf, qint32 len);
	void callback(const qint16* buf, qint32 len);
};

#endif // INCLUDE_BLADERFTHREAD_H
