/**
 * SDRangel
 * This is the web REST/JSON API of SDRangel SDR software. SDRangel is an Open Source Qt5/OpenGL 3.0+ (4.3+ in Windows) GUI and server Software Defined Radio and signal analyzer in software. It supports Airspy, BladeRF, HackRF, LimeSDR, PlutoSDR, RTL-SDR, SDRplay RSP1 and FunCube    ---   Limitations and specifcities:    * In SDRangel GUI the first Rx device set cannot be deleted. Conversely the server starts with no device sets and its number of device sets can be reduced to zero by as many calls as necessary to /sdrangel/deviceset with DELETE method.   * Preset import and export from/to file is a server only feature.   * Device set focus is a GUI only feature.   * The following channels are not implemented (status 501 is returned): ATV and DATV demodulators, Channel Analyzer NG, LoRa demodulator   * The device settings and report structures contains only the sub-structure corresponding to the device type. The DeviceSettings and DeviceReport structures documented here shows all of them but only one will be or should be present at a time   * The channel settings and report structures contains only the sub-structure corresponding to the channel type. The ChannelSettings and ChannelReport structures documented here shows all of them but only one will be or should be present at a time    --- 
 *
 * OpenAPI spec version: 6.0.0
 * Contact: f4exb06@gmail.com
 *
 * NOTE: This class is auto generated by the swagger code generator program.
 * https://github.com/swagger-api/swagger-codegen.git
 * Do not edit the class manually.
 */

/*
 * SWGAISModSettings.h
 *
 * AISMod
 */

#ifndef SWGAISModSettings_H_
#define SWGAISModSettings_H_

#include <QJsonObject>


#include "SWGChannelMarker.h"
#include <QString>

#include "SWGObject.h"
#include "export.h"

namespace SWGSDRangel {

class SWG_API SWGAISModSettings: public SWGObject {
public:
    SWGAISModSettings();
    SWGAISModSettings(QString* json);
    virtual ~SWGAISModSettings();
    void init();
    void cleanup();

    virtual QString asJson () override;
    virtual QJsonObject* asJsonObject() override;
    virtual void fromJsonObject(QJsonObject &json) override;
    virtual SWGAISModSettings* fromJson(QString &jsonString) override;

    qint64 getInputFrequencyOffset();
    void setInputFrequencyOffset(qint64 input_frequency_offset);

    qint32 getBaud();
    void setBaud(qint32 baud);

    float getRfBandwidth();
    void setRfBandwidth(float rf_bandwidth);

    qint32 getFmDeviation();
    void setFmDeviation(qint32 fm_deviation);

    float getGain();
    void setGain(float gain);

    qint32 getChannelMute();
    void setChannelMute(qint32 channel_mute);

    qint32 getRepeat();
    void setRepeat(qint32 repeat);

    float getRepeatDelay();
    void setRepeatDelay(float repeat_delay);

    qint32 getRepeatCount();
    void setRepeatCount(qint32 repeat_count);

    qint32 getRampUpBits();
    void setRampUpBits(qint32 ramp_up_bits);

    qint32 getRampDownBits();
    void setRampDownBits(qint32 ramp_down_bits);

    qint32 getRampRange();
    void setRampRange(qint32 ramp_range);

    qint32 getRfNoise();
    void setRfNoise(qint32 rf_noise);

    qint32 getWriteToFile();
    void setWriteToFile(qint32 write_to_file);

    qint32 getMsgType();
    void setMsgType(qint32 msg_type);

    QString* getMmsi();
    void setMmsi(QString* mmsi);

    qint32 getStatus();
    void setStatus(qint32 status);

    float getLatitude();
    void setLatitude(float latitude);

    float getLongitude();
    void setLongitude(float longitude);

    float getCourse();
    void setCourse(float course);

    float getSpeed();
    void setSpeed(float speed);

    qint32 getHeading();
    void setHeading(qint32 heading);

    QString* getData();
    void setData(QString* data);

    float getBt();
    void setBt(float bt);

    qint32 getSymbolSpan();
    void setSymbolSpan(qint32 symbol_span);

    qint32 getRgbColor();
    void setRgbColor(qint32 rgb_color);

    QString* getTitle();
    void setTitle(QString* title);

    qint32 getStreamIndex();
    void setStreamIndex(qint32 stream_index);

    qint32 getUseReverseApi();
    void setUseReverseApi(qint32 use_reverse_api);

    QString* getReverseApiAddress();
    void setReverseApiAddress(QString* reverse_api_address);

    qint32 getReverseApiPort();
    void setReverseApiPort(qint32 reverse_api_port);

    qint32 getReverseApiDeviceIndex();
    void setReverseApiDeviceIndex(qint32 reverse_api_device_index);

    qint32 getReverseApiChannelIndex();
    void setReverseApiChannelIndex(qint32 reverse_api_channel_index);

    qint32 getUdpEnabled();
    void setUdpEnabled(qint32 udp_enabled);

    QString* getUdpAddress();
    void setUdpAddress(QString* udp_address);

    qint32 getUdpPort();
    void setUdpPort(qint32 udp_port);

    SWGChannelMarker* getChannelMarker();
    void setChannelMarker(SWGChannelMarker* channel_marker);


    virtual bool isSet() override;

private:
    qint64 input_frequency_offset;
    bool m_input_frequency_offset_isSet;

    qint32 baud;
    bool m_baud_isSet;

    float rf_bandwidth;
    bool m_rf_bandwidth_isSet;

    qint32 fm_deviation;
    bool m_fm_deviation_isSet;

    float gain;
    bool m_gain_isSet;

    qint32 channel_mute;
    bool m_channel_mute_isSet;

    qint32 repeat;
    bool m_repeat_isSet;

    float repeat_delay;
    bool m_repeat_delay_isSet;

    qint32 repeat_count;
    bool m_repeat_count_isSet;

    qint32 ramp_up_bits;
    bool m_ramp_up_bits_isSet;

    qint32 ramp_down_bits;
    bool m_ramp_down_bits_isSet;

    qint32 ramp_range;
    bool m_ramp_range_isSet;

    qint32 rf_noise;
    bool m_rf_noise_isSet;

    qint32 write_to_file;
    bool m_write_to_file_isSet;

    qint32 msg_type;
    bool m_msg_type_isSet;

    QString* mmsi;
    bool m_mmsi_isSet;

    qint32 status;
    bool m_status_isSet;

    float latitude;
    bool m_latitude_isSet;

    float longitude;
    bool m_longitude_isSet;

    float course;
    bool m_course_isSet;

    float speed;
    bool m_speed_isSet;

    qint32 heading;
    bool m_heading_isSet;

    QString* data;
    bool m_data_isSet;

    float bt;
    bool m_bt_isSet;

    qint32 symbol_span;
    bool m_symbol_span_isSet;

    qint32 rgb_color;
    bool m_rgb_color_isSet;

    QString* title;
    bool m_title_isSet;

    qint32 stream_index;
    bool m_stream_index_isSet;

    qint32 use_reverse_api;
    bool m_use_reverse_api_isSet;

    QString* reverse_api_address;
    bool m_reverse_api_address_isSet;

    qint32 reverse_api_port;
    bool m_reverse_api_port_isSet;

    qint32 reverse_api_device_index;
    bool m_reverse_api_device_index_isSet;

    qint32 reverse_api_channel_index;
    bool m_reverse_api_channel_index_isSet;

    qint32 udp_enabled;
    bool m_udp_enabled_isSet;

    QString* udp_address;
    bool m_udp_address_isSet;

    qint32 udp_port;
    bool m_udp_port_isSet;

    SWGChannelMarker* channel_marker;
    bool m_channel_marker_isSet;

};

}

#endif /* SWGAISModSettings_H_ */
