#ifndef HALREMOTECOMPONENT_H
#define HALREMOTECOMPONENT_H

#include <QObject>
#include <QQuickItem>
#include <QHash>
#include <QTimer>
#include <QUuid>
#include <nzmqt/nzmqt.hpp>
#include <machinetalk/protobuf/message.pb.h>
#include <google/protobuf/text_format.h>
#include "qhalpin.h"
#include "halremotecomponentbase.h"

class HalRemoteComponent : public HalRemoteComponentBase
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int heartbeatPeriod READ heartbeatPeriod WRITE heartbeatPeriod NOTIFY heartbeatPeriodChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(State connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(ConnectionError error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(QObject *containerItem READ containerItem WRITE setContainerItem NOTIFY containerItemChanged)
    Q_PROPERTY(bool create READ create WRITE setCreate NOTIFY createChanged)
    Q_ENUMS(ConnectionError)

public:
    explicit HalRemoteComponent(QObject *parent  = 0);

    enum ConnectionError {
        NoError = 0,
        BindError = 1,
        PinChangeError = 2,
        CommandError = 3,
        SocketError = 4
    };

    QString name() const
    {
        return m_name;
    }

    bool isConnected() const
    {
        return m_connected;
    }

    State connectionState() const
    {
        return m_connectionState;
    }

    ConnectionError error() const
    {
        return m_error;
    }

    QString errorString() const
    {
        return m_errorString;
    }

    QObject * containerItem() const
    {
        return m_containerItem;
    }

    bool create() const
    {
        return m_create;
    }

    int heartbeatPeriod() const
    {
        return m_heartbeatPeriod;
    }

public slots:
    void pinChange(QVariant value);

    void setName(QString name)
    {
        if (m_name == name)
            return;

        m_name = name;
        emit nameChanged(name);
    }

    void setContainerItem(QObject * containerItem)
    {
        if (m_containerItem == containerItem)
            return;

        m_containerItem = containerItem;
        emit containerItemChanged(containerItem);
    }

    void setCreate(bool create)
    {
        if (m_create == create)
            return;

        m_create = create;
        emit createChanged(create);
    }

    void heartbeatPeriod(int heartbeatPeriod)
    {
        if (m_heartbeatPeriod == heartbeatPeriod)
            return;

        m_heartbeatPeriod = heartbeatPeriod;
        emit heartbeatPeriodChanged(heartbeatPeriod);
    }

private:
    QString m_name;
    bool m_connected;
    State m_connectionState;
    ConnectionError m_error;
    QString m_errorString;
    QObject * m_containerItem;
    bool m_create;
    int m_heartbeatPeriod;

    // more efficient to reuse a protobuf Message
    pb::Container   m_tx;
    QMap<QString, QHalPin*> m_pinsByName;
    QHash<int, QHalPin*>    m_pinsByHandle;

    QObjectList recurseObjects(const QObjectList &list);

private slots:
    void pinUpdate(const pb::Pin &remotePin, QHalPin *localPin);

    void halrcompSocketMessageReceived(QByteArray topic, pb::Container *rx);
    void halrcmdSocketMessageReceived(pb::Container *rx);

    void bind();
    void addPins();
    void removePins();
    void unsyncPins();
    void synced();

signals:
    void nameChanged(QString name);
    void connectedChanged(bool connected);
    void connectionStateChanged(State connectionState);
    void errorChanged(ConnectionError error);
    void errorStringChanged(QString errorString);
    void containerItemChanged(QObject * containerItem);
    void createChanged(bool create);
    void heartbeatPeriodChanged(int heartbeatPeriod);
};

#endif // HALREMOTECOMPONENT_H
