#pragma once

#include <QObject>
#include <QAudioSource>
#include <QUdpSocket>
#include <QTimer>
#include <QQueue>
#include <QHostAddress>

struct OpusEncoder;

class MicStream : public QObject
{
    Q_OBJECT

public:
    explicit MicStream(QObject *parent = nullptr);
    ~MicStream();

    bool start(const QString &host, int negotiatedPort);
    void stop();

private slots:
    void onAudio();
    void sendLoop();
    void logSummary();

private:
    QAudioSource *m_audioInput;
    QIODevice *m_audioDevice;
    OpusEncoder *m_encoder;
    QUdpSocket m_socket;
    QTimer m_sendTimer;
    QTimer m_logTimer;
    QQueue<QByteArray> m_queue;
    QHostAddress m_host;
    quint16 m_seq;
    quint32 m_timestamp;
    quint32 m_ssrc;
    int m_port;

    quint64 m_pcmBytes;
    quint64 m_opusBytes;
    quint64 m_sentBytes;
    int m_sentPackets;
    int m_idleLoops;
};

