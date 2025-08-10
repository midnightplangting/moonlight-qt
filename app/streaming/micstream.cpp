#include "micstream.h"

#include <opus.h>
#include <QtEndian>
#include <QRandomGenerator>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QAudio>
#include <QList>
#include "backend/Logger.h"

static const int PCM_FRAME_SAMPLES = 960; // 20 ms at 48 kHz
static const int PCM_FRAME_SIZE = PCM_FRAME_SAMPLES * 2; // mono 16-bit
static const int MAX_OPUS_SIZE = 4000;

MicStream::MicStream(QObject *parent)
    : QObject(parent),
      m_audioInput(nullptr),
      m_audioDevice(nullptr),
      m_encoder(nullptr),
      m_seq(0),
      m_timestamp(0),
      m_ssrc(0),
      m_port(47996)
{
    connect(&m_sendTimer, &QTimer::timeout, this, &MicStream::sendLoop);
}

MicStream::~MicStream()
{
    stop();
}

bool MicStream::start(const QString &host, int negotiatedPort)
{
    if (m_audioInput)
        return false;

    int err;
    m_encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK)
        return false;
    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(64000));

    QAudioFormat fmt;
    fmt.setSampleRate(48000);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    const QList<QAudioDevice> devices = QMediaDevices::audioInputs();
    if (devices.isEmpty()) {
        LOG_WARN(QStringLiteral("[MicStream] No audio input devices available"));
    } else {
        LOG_INFO(QStringLiteral("[MicStream] Available audio input devices:"));
        for (const QAudioDevice &dev : devices) {
            LOG_INFO(QStringLiteral("  %1").arg(dev.description()));
        }
    }

    QAudioDevice device = QMediaDevices::defaultAudioInput();
    LOG_INFO(QStringLiteral("[MicStream] Using audio input device: %1")
             .arg(device.description()));

    m_audioInput = new QAudioSource(device, fmt, this);
    m_audioInput->setBufferSize(PCM_FRAME_SIZE);
    m_audioDevice = m_audioInput->start();
    if (!m_audioDevice || m_audioInput->error() != QAudio::NoError) {
        LOG_WARN(QStringLiteral("[MicStream] Failed to start audio device error=%1")
                 .arg(m_audioInput->error()));
        delete m_audioInput;
        m_audioInput = nullptr;
        return false;
    }

    LOG_INFO(QStringLiteral("[MicStream] Audio device initialized successfully"));

    connect(m_audioDevice, &QIODevice::readyRead, this, &MicStream::onAudio);

    m_host = QHostAddress(host);
    m_port = negotiatedPort > 0 ? negotiatedPort : 47996;
    m_seq = 0;
    m_timestamp = 0;
    m_ssrc = QRandomGenerator::global()->generate();

    LOG_INFO(QStringLiteral("[MicStream] start host=%1 port=%2")
             .arg(host)
             .arg(m_port));

    m_sendTimer.start(20);
    return true;
}

void MicStream::stop()
{
    m_sendTimer.stop();
    if (m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
        m_audioInput = nullptr;
        m_audioDevice = nullptr;
    }
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
    }
    m_queue.clear();

    LOG_INFO(QStringLiteral("[MicStream] stop"));
}

void MicStream::onAudio()
{
    if (m_audioDevice) {
        LOG_DEBUG(QStringLiteral("[MicStream] onAudio bytesAvailable=%1 queue=%2")
                  .arg(m_audioDevice->bytesAvailable())
                  .arg(m_queue.size()));
    }

    while (m_audioDevice && m_audioDevice->bytesAvailable() >= PCM_FRAME_SIZE) {
        QByteArray pcm = m_audioDevice->read(PCM_FRAME_SIZE);
        if (pcm.size() < PCM_FRAME_SIZE) {
            LOG_WARN(QStringLiteral("[MicStream] PCM underrun read=%1 expected=%2")
                     .arg(pcm.size())
                     .arg(PCM_FRAME_SIZE));
            return;
        }

        LOG_INFO(QStringLiteral("[MicStream] read pcm=%1 bytes")
                 .arg(pcm.size()));

        unsigned char encoded[MAX_OPUS_SIZE];
        int len = opus_encode(m_encoder,
                              reinterpret_cast<const opus_int16*>(pcm.constData()),
                              PCM_FRAME_SAMPLES,
                              encoded,
                              MAX_OPUS_SIZE);
        if (len > 0) {
            m_queue.enqueue(QByteArray(reinterpret_cast<char*>(encoded), len));
            LOG_DEBUG(QStringLiteral("[MicStream] queued opus bytes=%1 totalQueued=%2")
                      .arg(len)
                      .arg(m_queue.size()));
        } else {
            LOG_WARN(QStringLiteral("[MicStream] opus_encode failed len=%1")
                     .arg(len));
        }
    }
}

void MicStream::sendLoop()
{
    if (m_queue.isEmpty()) {
        LOG_DEBUG(QStringLiteral("[MicStream] sendLoop no queued frames"));
    }

    while (!m_queue.isEmpty()) {
        QByteArray opus = m_queue.dequeue();
        QByteArray pkt;
        pkt.resize(12 + opus.size());
        pkt[0] = 0x00;
        pkt[1] = 0x61;
        quint16 seqbe = qToBigEndian(m_seq++);
        quint32 tsbe = qToBigEndian(m_timestamp);
        quint32 ssrcbe = qToBigEndian(m_ssrc);
        memcpy(pkt.data() + 2, &seqbe, 2);
        memcpy(pkt.data() + 4, &tsbe, 4);
        memcpy(pkt.data() + 8, &ssrcbe, 4);
        memcpy(pkt.data() + 12, opus.constData(), opus.size());
        m_socket.writeDatagram(pkt, m_host, m_port);
        LOG_DEBUG(QStringLiteral("[MicStream] sent seq=%1 ts=%2 bytes=%3")
                  .arg(m_seq - 1)
                  .arg(m_timestamp)
                  .arg(opus.size()));
        m_timestamp += PCM_FRAME_SAMPLES;
    }
}

