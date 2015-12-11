#include "halremotecomponent.h"
#include "debughelper.h"

HalRemoteComponent::HalRemoteComponent(QObject *parent) :
    HalRemoteComponentBase(parent),
    m_name("default"),
    m_connected(false),
    m_error(NoError),
    m_errorString(""),
    m_containerItem(this),
    m_create(true)
{
    connect(this, SIGNAL(halrcmdMessageReceived(pb::Container*)),
            this, SLOT(halrcmdSocketMessageReceived(pb::Container*)));
    connect(this, SIGNAL(halrcompMessageReceived(QByteArray,pb::Container*)),
            this, SLOT(halrcompSocketMessageReceived(QByteArray,pb::Container*)));
}

/** Updates a remote pin witht the value of a local pin */
void HalRemoteComponent::pinChange(QVariant value)
{
    Q_UNUSED(value)
    QHalPin *pin;
    pb::Pin *halPin;

    if (state() != Synced) // only accept pin changes if we are connected
    {
        return;
    }

    pin = static_cast<QHalPin *>(QObject::sender());

    if (pin->direction() == QHalPin::In)   // Only update Output or IO pins
    {
        return;
    }

#ifdef QT_DEBUG
    DEBUG_TAG(2, m_name,  "pin change" << pin->name() << pin->value())
#endif

    // This message MUST carry a Pin message for each pin which has
    // changed value since the last message of this type.
    // Each Pin message MUST carry the handle field.
    // Each Pin message MAY carry the name field.
    // Each Pin message MUST carry the type field
    // Each Pin message MUST - depending on pin type - carry a halbit,
    // halfloat, hals32, or halu32 field.
    halPin = m_tx.add_pin();

    halPin->set_handle(pin->handle());
    halPin->set_type((pb::ValueType)pin->type());
    if (pin->type() == QHalPin::Float)
    {
        halPin->set_halfloat(pin->value().toDouble());
    }
    else if (pin->type() == QHalPin::Bit)
    {
        halPin->set_halbit(pin->value().toBool());
    }
    else if (pin->type() == QHalPin::S32)
    {
        halPin->set_hals32(pin->value().toInt());
    }
    else if (pin->type() == QHalPin::U32)
    {
        halPin->set_halu32(pin->value().toUInt());
    }

    sendHalrcompSet(&m_tx);
}

/** Recurses through a list of objects */
QObjectList HalRemoteComponent::recurseObjects(const QObjectList &list)
{
    QObjectList halObjects;

    foreach (QObject *object, list)
    {
        QHalPin *halPin;
        halPin = qobject_cast<QHalPin *>(object);
        if (halPin != NULL)
        {
            halObjects.append(object);
        }

        if (object->children().size() > 0)
        {
            halObjects.append(recurseObjects(object->children()));
        }
    }

    return halObjects;
}

/** Updates a local pin with the value of a remote pin */
void HalRemoteComponent::pinUpdate(const pb::Pin &remotePin, QHalPin *localPin)
{
#ifdef QT_DEBUG
    DEBUG_TAG(2, m_name,  "pin update" << localPin->name() << remotePin.halfloat() << remotePin.halbit() << remotePin.hals32() << remotePin.halu32())
#endif

    if (remotePin.has_halfloat())
    {
        localPin->setValue(QVariant(remotePin.halfloat()), true);
    }
    else if (remotePin.has_halbit())
    {
        localPin->setValue(QVariant(remotePin.halbit()), true);
    }
    else if (remotePin.has_hals32())
    {
        localPin->setValue(QVariant(remotePin.hals32()), true);
    }
    else if (remotePin.has_halu32())
    {
        localPin->setValue(QVariant(remotePin.halu32()), true);
    }
}

/** Processes all message received on the update 0MQ socket */
void HalRemoteComponent::halrcompSocketMessageReceived(QByteArray topic, pb::Container *rx)
{
#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(*rx, &s);
    DEBUG_TAG(3, m_name, "status update" << topic << QString::fromStdString(s))
#else
    Q_UNUSED(topic)
#endif

    if (rx->type() == pb::MT_HALRCOMP_INCREMENTAL_UPDATE) //incremental update
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_name, "incremental update")
#endif
        for (int i = 0; i < rx->pin_size(); ++i)
        {
            pb::Pin remotePin = rx->pin(i);
            QHalPin *localPin = m_pinsByHandle.value(remotePin.handle(), NULL);
            if (localPin != NULL) // in case we received a wrong pin handle
            {
                pinUpdate(remotePin, localPin);
            }
        }

        return;
    }
    else if (rx->type() == pb::MT_HALRCOMP_FULL_UPDATE)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_name, "full update")
#endif
        for (int i = 0; i < rx->comp_size(); ++i)  // TODO: we can only handle one component
        {
            pb::Component component = rx->comp(i);  // shouldnt we check the name?
            for (int j = 0; j < component.pin_size(); j++)
            {
                pb::Pin remotePin = component.pin(j);
                QString name = QString::fromStdString(remotePin.name());
                int dotIndex = name.indexOf(".");
                if (dotIndex != -1)    // strip comp prefix
                {
                    name = name.mid(dotIndex + 1);
                }
                QHalPin *localPin = m_pinsByName.value(name);
                localPin->setHandle(remotePin.handle());
                m_pinsByHandle.insert(remotePin.handle(), localPin);
                pinUpdate(remotePin, localPin);
            }
        }

        return;
    }
    else if (rx->type() == pb::MT_HALRCOMMAND_ERROR)
    {
        QString errorString;

        for (int i = 0; i < rx->note_size(); ++i)
        {
            errorString.append(QString::fromStdString(rx->note(i)) + "\n");
        }

        //updateState(Error, CommandError, errorString);

#ifdef QT_DEBUG
        DEBUG_TAG(1, m_name, "proto error on subscribe" << errorString)
#endif

        return;
    }

#ifdef QT_DEBUG
    gpb::TextFormat::PrintToString(*rx, &s);
    DEBUG_TAG(1, m_name, "status_update: unknown message type: " << QString::fromStdString(s))
#endif
}

void HalRemoteComponent::halrcmdSocketMessageReceived(pb::Container *rx)
{
}

/** Generates a Bind messages and sends it over the suitable 0MQ socket */
void HalRemoteComponent::bind()
{
    pb::Component *component;

    component = m_tx.add_comp();
    component->set_name(m_name.toStdString());
    component->set_no_create(!m_create);
    foreach (QHalPin *pin, m_pinsByName)
    {
        pb::Pin *halPin = component->add_pin();
        halPin->set_name(QString("%1.%2").arg(m_name).arg(pin->name()).toStdString());  // pin name is always component.name
        halPin->set_type((pb::ValueType)pin->type());
        halPin->set_dir((pb::HalPinDirection)pin->direction());
        if (pin->type() == QHalPin::Float)
        {
            halPin->set_halfloat(pin->value().toDouble());
        }
        else if (pin->type() == QHalPin::Bit)
        {
            halPin->set_halbit(pin->value().toBool());
        }
        else if (pin->type() == QHalPin::S32)
        {
            halPin->set_hals32(pin->value().toInt());
        }
        else if (pin->type() == QHalPin::U32)
        {
            halPin->set_halu32(pin->value().toUInt());
        }
    }

#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(m_tx, &s);
    DEBUG_TAG(1, m_name, "bind");
    DEBUG_TAG(3, m_name, QString::fromStdString(s));
#endif

    sendHalrcompBind(&m_tx);
}

/** Scans all children of the container item for pins and adds them to a map */
void HalRemoteComponent::addPins()
{
    QObjectList halObjects;

    if (m_containerItem == NULL)
    {
        return;
    }

    clearHalrcompTopics();
    addHalrcompTopic(m_name);

    halObjects = recurseObjects(m_containerItem->children());
    foreach (QObject *object, halObjects)
    {
        QHalPin *pin = static_cast<QHalPin *>(object);
        if (pin->name().isEmpty()  || (pin->enabled() == false))    // ignore pins with empty name and disabled pins
        {
            continue;
        }
        m_pinsByName[pin->name()] = pin;
        connect(pin, SIGNAL(valueChanged(QVariant)),
                this, SLOT(pinChange(QVariant)));
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_name, "pin added: " << pin->name())
#endif
    }
}

/** Removes all previously added pins */
void HalRemoteComponent::removePins()
{
    foreach (QHalPin *pin, m_pinsByName)
    {
        disconnect(pin, SIGNAL(valueChanged(QVariant)),
                this, SLOT(pinChange(QVariant)));
    }

    m_pinsByHandle.clear();
    m_pinsByName.clear();
}

/** Sets synced of all pins to false */
void HalRemoteComponent::unsyncPins()
{
    m_connected = false;
    emit connectedChanged(false);

    QMapIterator<QString, QHalPin*> i(m_pinsByName);
    while (i.hasNext()) {
        i.next();
        i.value()->setSynced(false);
    }
}

void HalRemoteComponent::synced()
{
    m_connected = true;
    emit connectedChanged(true);

    m_connectionState = Synced;
}

