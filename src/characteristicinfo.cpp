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

#include "characteristicinfo.h"
#include <QByteArray>

CharacteristicInfo::CharacteristicInfo()
{
}

CharacteristicInfo::CharacteristicInfo(const LowEnergyCharacteristic &characteristic):
    m_characteristic(characteristic)
{
}

void CharacteristicInfo::setCharacteristic(const LowEnergyCharacteristic &characteristic)
{
    m_characteristic = characteristic;
    emit characteristicChanged();
}

QString CharacteristicInfo::getName() const
{
    //! [les-get-descriptors]
    QString name = m_characteristic.name();
    if (!name.isEmpty())
        return name;

    // find descriptor with CharacteristicUserDescription
    foreach (const LowEnergyDescriptor &descriptor, m_characteristic.descriptors()) {
        if (descriptor.uuid().left(8) == QStringLiteral("00002901")) {
            name = descriptor.value();
            break;
        }
    }
    //! [les-get-descriptors]

    if (name.isEmpty())
        name = "Unknown";

    return name;
}

QString CharacteristicInfo::getUuid() const
{
    return m_characteristic.uuid();
}

QString CharacteristicInfo::getValue() const
{
    // Show raw string first and hex value below
    QByteArray a = m_characteristic.value();
    QString result;
    if (a.isEmpty()) {
        result = QStringLiteral("<none>");
        return result;
    }

    result = a;
    result += QLatin1Char('\n');
    result += a.toHex();

    return result;
}

QString CharacteristicInfo::getHandle() const
{
    return QStringLiteral("0x") + QString::number(m_characteristic.handle(), 16);
}

QString CharacteristicInfo::getPermission() const
{
    QString properties = "( ";
    int permission = m_characteristic.properties();
    if (permission & LowEnergyCharacteristic::Read)
        properties += QStringLiteral(" Read");
    if (permission & LowEnergyCharacteristic::Write)
        properties += QStringLiteral(" Write");
    if (permission & LowEnergyCharacteristic::Notify)
        properties += QStringLiteral(" Notify");
    if (permission & LowEnergyCharacteristic::Indicate)
        properties += QStringLiteral(" Indicate");
    if (permission & LowEnergyCharacteristic::ExtendedProperty)
        properties += QStringLiteral(" ExtendedProperty");
    if (permission & LowEnergyCharacteristic::Broadcasting)
        properties += QStringLiteral(" Broadcast");
    if (permission & LowEnergyCharacteristic::WriteNoResponse)
        properties += QStringLiteral(" WriteNoResp");
    if (permission & LowEnergyCharacteristic::WriteSigned)
        properties += QStringLiteral(" WriteSigned");
    properties += " )";
    return properties;
}

LowEnergyCharacteristic CharacteristicInfo::getCharacteristic() const
{
    return m_characteristic;
}
