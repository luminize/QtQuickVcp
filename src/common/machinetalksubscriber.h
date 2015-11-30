#ifndef MACHINETALKSUBSCRIBER_H
#define MACHINETALKSUBSCRIBER_H

#include <QObject>
#include <QSet>
#include <nzmqt/nzmqt.hpp>
#include <machinetalk/protobuf/message.pb.h>
#include <google/protobuf/text_format.h>
#include "machinetalk.h"

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

class MachinetalkSubscriber : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready WRITE setReady NOTIFY readyChanged)
    Q_PROPERTY(QString uri READ uri WRITE setUri NOTIFY uriChanged)
    Q_PROPERTY(QString debugName READ debugName WRITE setDebugName NOTIFY debugNameChanged)
    Q_PROPERTY(SocketState socketState READ socketState NOTIFY socketStateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_ENUMS(SocketState)

public:
    explicit MachinetalkSubscriber(QObject *parent = 0);
    ~MachinetalkSubscriber();

    QString uri() const
    {
        return m_uri;
    }

    QString debugName() const
    {
        return m_debugName;
    }

    SocketState socketState() const
    {
        return m_socketState;
    }

    QString errorString() const
    {
        return m_errorString;
    }

    bool ready() const
    {
        return m_ready;
    }

public slots:

    void setUri(QString uri)
    {
        if (m_uri == uri)
            return;

        m_uri = uri;
        emit uriChanged(uri);
    }

    void setDebugName(QString debugName)
    {
        if (m_debugName == debugName)
            return;

        m_debugName = debugName;
        emit debugNameChanged(debugName);
    }

    void setReady(bool ready)
    {
        if (m_ready == ready)
            return;

        m_ready = ready;
        emit readyChanged(ready);

        if (m_ready)
        {
            start();
        }
        else
        {
            stop();
        }
    }

    void addTopic(const QString &name);
    void removeTopic(const QString &name);
    void clearTopics();



private:
    bool m_ready;
    QString m_uri;
    QString m_debugName;
    QSet<QString> m_topics;  // the topics we are interested in
    QSet<QString> m_subscriptions;  // subscribed topics
    QSet<QString> m_publishers;  // connected publishers

    SocketNotifierZMQContext *m_context;
    ZMQSocket  *m_socket;
    SocketState m_socketState;
    QString     m_errorString;
    int         m_heartbeatPeriod;
    QTimer     *m_heartbeatTimer;
    pb::Container m_rx;  // more efficient to reuse a protobuf Message

    void start();
    void stop();
    void refreshHeartbeat();
    void stopHeartbeat();
    void updateState(SocketState state);
    void updateState(SocketState state, QString errorString);

private slots:
    void heartbeatTimerTick();
    void socketError(int errorNum, const QString &errorMsg);
    void socketMessageReceived(QList<QByteArray> messageList);

    bool connectSockets();
    void disconnectSockets();
    void subscribe();
    void unsubscribe();

signals:
    void messageReceived(QByteArray topic, pb::Container *rx);
    void uriChanged(QString uri);
    void debugNameChanged(QString debugName);
    void socketStateChanged(SocketState socketState);
    void errorStringChanged(QString errorString);
    void readyChanged(bool ready);
};

#endif // MACHINETALKSUBSCRIBER_H
