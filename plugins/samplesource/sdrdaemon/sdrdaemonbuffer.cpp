///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016 Edouard Griffiths, F4EXB                                   //
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

#include <cassert>
#include <cstring>
#include <iostream>
#include "sdrdaemonbuffer.h"

SDRdaemonBuffer::SDRdaemonBuffer(std::size_t blockSize) :
	m_blockSize(blockSize),
	m_sync(false),
	m_lz4(false),
	m_lz4InBuffer(0),
	m_lz4InCount(0),
	m_lz4InSize(0),
	m_lz4OutBuffer(0),
	m_frameSize(0),
	m_nbDecodes(0),
	m_nbSuccessfulDecodes(0),
	m_nbCRCOK(0),
	m_dataCRC(0),
	m_sampleRate(1000000),
	m_sampleBytes(2),
	m_sampleBits(12),
	m_rawBuffer(0)
{
	m_currentMeta.init();
}

SDRdaemonBuffer::~SDRdaemonBuffer()
{
	if (m_rawBuffer) {
		delete[] m_rawBuffer;
	}

	if (m_lz4InBuffer) {
		delete[] m_lz4InBuffer;
	}
}

bool SDRdaemonBuffer::readMeta(char *array, std::size_t length)
{
	assert(length >= sizeof(MetaData) + 8);
	MetaData *metaData = (MetaData *) array;

	if (m_crc64.calculate_crc((uint8_t *)array, sizeof(MetaData) - 8) == metaData->m_crc)
	{
		memcpy((void *) &m_dataCRC, (const void *) &array[sizeof(MetaData)], 8);

		if (!(m_currentMeta == *metaData))
		{
			std::cerr << "SDRdaemonBuffer::readMeta: ";
			printMeta(metaData);
		}

		m_currentMeta = *metaData;

		// sanity checks
		if (metaData->m_blockSize == m_blockSize) // sent blocksize matches given blocksize
		{
			uint32_t sampleBytes = metaData->m_sampleBytes & 0x0F;
			uint32_t frameSize = sampleBytes * 2 * metaData->m_nbSamples * metaData->m_nbBlocks;

			if (metaData->m_sampleBytes & 0x10)
			{
				m_lz4 = true;
				m_lz4InSize = metaData->m_nbBytes; // compressed input size
				m_lz4InCount = 0;

				if (frameSize != m_frameSize)
				{
					updateLZ4Sizes(frameSize);
				}
			}
			else
			{
				m_lz4 = false;
			}

			if (frameSize != m_frameSize)
			{
				updateBufferSize(frameSize);
			}

			m_frameSize = frameSize;
			m_sync = true;
		}
		else
		{
			m_sync = false;
		}
	}

	return m_sync;
}

void SDRdaemonBuffer::writeData(char *array, std::size_t length)
{
	if (m_sync)
	{
		if (m_lz4)
		{
			writeDataLZ4(array, length);
		}
		else
		{
			// TODO: uncompressed case
		}
	}
}

void SDRdaemonBuffer::writeDataLZ4(char *array, std::size_t length)
{
    if (m_lz4InCount + length < m_lz4InSize)
    {
        m_lz4InCount += length;
    }
    else
    {
        std::memcpy((void *) &m_lz4InBuffer[m_lz4InCount], (const void *) array, m_lz4InSize - m_lz4InCount); // copy rest of data in compressed Buffer
        m_lz4InCount += length;
    }

    if (m_lz4InCount >= m_lz4InSize) // full input compressed block retrieved
    {
        if (m_nbDecodes == 100)
        {
            std::cerr << "SDRdaemonBuffer::writeAndReadLZ4:"
               << " decoding: " << m_nbCRCOK
               << ":" << m_nbSuccessfulDecodes
               << "/" <<  m_nbDecodes
               << std::endl;

        	m_nbDecodes = 0;
        	m_nbSuccessfulDecodes = 0;
            m_nbCRCOK = 0;
        }

        uint64_t crc64 = m_crc64.calculate_crc(m_lz4InBuffer, m_lz4InSize);
        //uint64_t crc64 = 0x0123456789ABCDEF;

        if (memcmp(&crc64, &m_dataCRC, 8) == 0)
        {
            m_nbCRCOK++;
        }

        int compressedSize = LZ4_decompress_fast((const char*) m_lz4InBuffer, (char*) m_lz4OutBuffer, m_frameSize);
        m_nbDecodes++;

        if (compressedSize == m_lz4InSize)
    	{
    		/*
    		std::cerr << "SDRdaemonBuffer::writeAndReadLZ4: decoding OK:"
    				<< " read: " << compressedSize
					<< " expected: " << m_lz4InSize
					<< " out: " << m_lz4OutSize
					<< std::endl;
            */
        	m_nbSuccessfulDecodes++;
    	}
    	else
    	{
//    		std::cerr << "SDRdaemonBuffer::writeAndReadLZ4: decoding error:"
//    				<< " read: " << compressedSize
//					<< " expected: " << m_lz4InSize
//					<< " out: " << m_lz4OutSize
//					<< std::endl;

    		//if (compressedSize > 0)
    		//{
    		//}
    		//else
    		//{
    		//	dataLength = 0;
    		//}
    	}

		m_lz4InCount = 0;
    }
}

void SDRdaemonBuffer::updateLZ4Sizes(uint32_t frameSize)
{
	uint32_t masInputSize = LZ4_compressBound(frameSize);

	if (m_lz4InBuffer) {
		delete[] m_lz4InBuffer;
	}

	m_lz4InBuffer = new uint8_t[m_lz4InSize]; // provide extra space for a full UDP block

	if (m_lz4OutBuffer) {
		delete[] m_lz4OutBuffer;
	}

	m_lz4OutBuffer = new uint8_t[frameSize];
}

void SDRdaemonBuffer::updateBufferSize(uint32_t frameSize)
{
	uint32_t nbFrames = ((m_sampleRate * 2 * m_sampleBytes) / frameSize) + 1; // store at least 1 second of samples

	std::cerr << "SDRdaemonBuffer::updateBufferSize:"
		<< " frameSize: " << frameSize
		<< " nbFrames: " << nbFrames
		<< std::endl;

	if (m_rawBuffer) {
		delete[] m_rawBuffer;
	}

	m_rawBuffer = new uint8_t[nbFrames * frameSize];
}

void SDRdaemonBuffer::printMeta(MetaData *metaData)
{
	std::cerr
			<< "|" << metaData->m_centerFrequency
			<< ":" << metaData->m_sampleRate
			<< ":" << (int) (metaData->m_sampleBytes & 0xF)
			<< ":" << (int) metaData->m_sampleBits
			<< ":" << metaData->m_blockSize
			<< ":" << metaData->m_nbSamples
			<< "||" << metaData->m_nbBlocks
			<< ":" << metaData->m_nbBytes
	        << "|" << metaData->m_tv_sec
			<< ":" << metaData->m_tv_usec
			<< std::endl;
}

