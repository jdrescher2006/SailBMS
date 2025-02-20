/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "device.h"
#include <QDebug>
#include <QList>
#include <QTimer>
#include <QDBusConnection>

Device::Device():
    connected(false), controller(0), m_deviceScanState(false), randomAddress(false)
{
    //! [les-devicediscovery-1]
    discoveryAgent = new BluetoothDeviceDiscoveryAgent();
    //discoveryAgent->setLowEnergyDiscoveryTimeout(5000);
    connect(discoveryAgent, SIGNAL(deviceDiscovered(const BluetoothDeviceInfo&)),
            this, SLOT(addDevice(const BluetoothDeviceInfo&)));
    connect(discoveryAgent, SIGNAL(error(BluetoothDeviceDiscoveryAgent::Error)),
            this, SLOT(deviceScanError(BluetoothDeviceDiscoveryAgent::Error)));
    connect(discoveryAgent, SIGNAL(finished()), this, SLOT(deviceScanFinished()));
    //! [les-devicediscovery-1]

    setUpdate("Search");
}

Device::~Device()
{
    delete discoveryAgent;
    delete controller;
    qDeleteAll(devices);
    qDeleteAll(m_services);
    qDeleteAll(m_characteristics);
    devices.clear();
    m_services.clear();
    m_characteristics.clear();
}

void Device::startDeviceDiscovery()
{
    qDeleteAll(devices);
    devices.clear();
    emit devicesUpdated();

    setUpdate("Scanning for devices ...");
    //! [les-devicediscovery-2]
    discoveryAgent->start();
    //! [les-devicediscovery-2]

    if (discoveryAgent->isActive()) {
        m_deviceScanState = true;
        Q_EMIT stateChanged();
    }
}

//! [les-devicediscovery-3]
void Device::addDevice(const BluetoothDeviceInfo &info)
{
    if (info.coreConfigurations() & BluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        DeviceInfo *d = new DeviceInfo(info);
        devices.append(d);
        setUpdate("Last device added: " + d->getName());
    }
}
//! [les-devicediscovery-3]

void Device::deviceScanFinished()
{
    emit devicesUpdated();
    m_deviceScanState = false;
    emit stateChanged();
    if (devices.isEmpty())
        setUpdate("No Low Energy devices found...");
    else
        setUpdate("Done! Scan Again!");
}

QVariant Device::getDevices()
{
    return QVariant::fromValue(devices);
}

QVariant Device::getServices()
{
    return QVariant::fromValue(m_services);
}

QVariant Device::getCharacteristics()
{
    return QVariant::fromValue(m_characteristics);
}

QString Device::getUpdate()
{
    return m_message;
}

void Device::scanServices(const QString &address)
{
    // We need the current device for service discovery.

    for (int i = 0; i < devices.size(); i++) {
        if (((DeviceInfo*)devices.at(i))->getAddress() == address ) {
            currentDevice.setDevice(((DeviceInfo*)devices.at(i))->getDevice());
        }
    }

    if (!currentDevice.getDevice().isValid()) {
        qWarning() << "Not a valid device";
        return;
    }

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit characteristicsUpdated();
    qDeleteAll(m_services);
    m_services.clear();
    emit servicesUpdated();

    setUpdate("Back\n(Connecting to device...)");

    if (controller && m_previousAddress != currentDevice.getAddress()) {
        controller->disconnectFromDevice();
        delete controller;
        controller = 0;
    }

    //! [les-controller-1]
    if (!controller) {
        // Connecting signals and slots for connecting to LE services.
        controller = new LowEnergyController(currentDevice.getDevice());
        connect(controller, SIGNAL(connected()),
                this, SLOT(deviceConnected()));
        connect(controller, SIGNAL(error(LowEnergyController::Error)),
                this, SLOT(errorReceived(LowEnergyController::Error)));
        connect(controller, SIGNAL(disconnected()),
                this, SLOT(deviceDisconnected()));
        connect(controller, SIGNAL(serviceDiscovered(QString)),
                this, SLOT(addLowEnergyService(QString)));
        connect(controller, SIGNAL(discoveryFinished()),
                this, SLOT(serviceScanDone()));
    }

    if (isRandomAddress())
        controller->setRemoteAddressType(LowEnergyController::RandomAddress);
    else
        controller->setRemoteAddressType(LowEnergyController::PublicAddress);
    controller->connectToDevice();
    //! [les-controller-1]

    m_previousAddress = currentDevice.getAddress();
}

void Device::addLowEnergyService(const QString &serviceUuid)
{
    //! [les-service-1]
    LowEnergyService *service = controller->createServiceObject(serviceUuid);
    if (!service) {
        qWarning() << "Cannot create service for uuid";
        return;
    }
    //! [les-service-1]
    ServiceInfo *serv = new ServiceInfo(service);
    m_services.append(serv);

    emit servicesUpdated();
}
//! [les-service-1]

void Device::serviceScanDone()
{
    setUpdate("Back\n(Service scan done!)");
    // force UI in case we didn't find anything
    if (m_services.isEmpty())
        emit servicesUpdated();
}

void Device::connectToService(const QString &uuid)
{
    LowEnergyService *service = 0;
    for (int i = 0; i < m_services.size(); i++) {
        ServiceInfo *serviceInfo = (ServiceInfo*)m_services.at(i);
        if (serviceInfo->getUuid() == uuid) {
            service = serviceInfo->service();
            break;
        }
    }

    if (!service)
        return;

    qDeleteAll(m_characteristics);
    m_characteristics.clear();
    emit characteristicsUpdated();

    if (service->state() == LowEnergyService::DiscoveryRequired) {
        //! [les-service-3]
        connect(service, SIGNAL(stateChanged(LowEnergyService::ServiceState)),
                this, SLOT(serviceDetailsDiscovered(LowEnergyService::ServiceState)));
        service->discoverDetails();
        setUpdate("Back\n(Discovering details...)");
        //! [les-service-3]
        return;
    }

    //discovery already done
    const QList<LowEnergyCharacteristic> chars = service->characteristics();
    foreach (const LowEnergyCharacteristic &ch, chars) {
        CharacteristicInfo *cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
    }

    QTimer::singleShot(0, this, SIGNAL(characteristicsUpdated()));
}

void Device::deviceConnected()
{
    setUpdate("Back\n(Discovering services...)");
    connected = true;
    //! [les-service-2]
    controller->discoverServices();
    //! [les-service-2]
}

void Device::errorReceived(LowEnergyController::Error /*error*/)
{
    qWarning() << "Error: " << controller->errorString();
    setUpdate(QString("Back\n(%1)").arg(controller->errorString()));
}

void Device::setUpdate(QString message)
{
    m_message = message;
    emit updateChanged();
}

void Device::disconnectFromDevice()
{
    // UI always expects disconnect() signal when calling this signal
    // TODO what is really needed is to extend state() to a multi value
    // and thus allowing UI to keep track of controller progress in addition to
    // device scan progress

    if (controller->state() != LowEnergyController::UnconnectedState)
        controller->disconnectFromDevice();
    else
        deviceDisconnected();
}

void Device::deviceDisconnected()
{
    qWarning() << "Disconnect from device";
    emit disconnected();
}

void Device::serviceDetailsDiscovered(LowEnergyService::ServiceState newState)
{
    if (newState != LowEnergyService::ServiceDiscovered) {
        // do not hang in "Scanning for characteristics" mode forever
        // in case the service discovery failed
        // We have to queue the signal up to give UI time to even enter
        // the above mode
        if (newState != LowEnergyService::DiscoveringServices) {
            QMetaObject::invokeMethod(this, "characteristicsUpdated",
                                      Qt::QueuedConnection);
        }
        return;
    }

    LowEnergyService *service = qobject_cast<LowEnergyService *>(sender());
    if (!service)
        return;



    //! [les-chars]
    const QList<LowEnergyCharacteristic> chars = service->characteristics();
    foreach (const LowEnergyCharacteristic &ch, chars) {
        CharacteristicInfo *cInfo = new CharacteristicInfo(ch);
        m_characteristics.append(cInfo);
    }
    //! [les-chars]

    emit characteristicsUpdated();
}

void Device::deviceScanError(BluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == BluetoothDeviceDiscoveryAgent::PoweredOffError)
        setUpdate("The Bluetooth adaptor is powered off, power it on before doing discovery.");
    else if (error == BluetoothDeviceDiscoveryAgent::InputOutputError)
        setUpdate("Writing or reading from the device resulted in an error.");
    else
        setUpdate("An unknown error has occurred.");

    m_deviceScanState = false;
    emit devicesUpdated();
    emit stateChanged();
}

bool Device::state()
{
    return m_deviceScanState;
}

bool Device::hasControllerError() const
{
    if (controller && controller->error() != LowEnergyController::NoError)
        return true;
    return false;
}

bool Device::isRandomAddress() const
{
    return randomAddress;
}

void Device::setRandomAddress(bool newValue)
{
    randomAddress = newValue;
    emit randomAddressChanged();
}
