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

private:
    QAudioSource *m_audioInput;
    QIODevice *m_audioDevice;
    OpusEncoder *m_encoder;
    QUdpSocket m_socket;
    QTimer m_sendTimer;
    QQueue<QByteArray> m_queue;
    QHostAddress m_host;
    quint16 m_seq;
    quint32 m_timestamp;
    quint32 m_ssrc;
    int m_port;
};

