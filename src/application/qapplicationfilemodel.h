#ifndef QAPPLICATIONFILEMODEL_H
#define QAPPLICATIONFILEMODEL_H

/****************************************************************************
**
** Copyright (C) 2015 Alexander Rössler
** License: LGPL version 2.1
**
** This file is part of QtQuickVcp.
**
** All rights reserved. This program and the accompanying materials
** are made available under the terms of the GNU Lesser General Public License
** (LGPL) version 2.1 which accompanies this distribution, and is available at
** http://www.gnu.org/licenses/lgpl-2.1.html
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
**
** Contributors:
** Alexander Rössler @ The Cool Tool GmbH <mail DOT aroessler AT gmail DOT com>
**
****************************************************************************/

#include <QAbstractListModel>
#include "qapplicationfileitem.h"

class QApplicationFileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(ApplicationFileRoles)

public:
    enum ApplicationFileRoles {
            NameRole = Qt::UserRole,
            SizeRole,
            OwnerRole,
            GroupRole,
            LastModifiedRole,
            DirRole
        };

    explicit QApplicationFileModel(QObject *parent = 0);
    ~QApplicationFileModel();

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QHash<int, QByteArray> roleNames() const;

public slots:
    void addItem(QApplicationFileItem *item);
    QString getName(int row);
    void clear();
    void beginUpdate();
    void endUpdate();

private:
    QList<QApplicationFileItem*> m_items;

    QVariant internalData(const QModelIndex &index, int role) const;
    QString formatByteSize(qint64 bytes) const;
};

#endif // QAPPLICATIONFILEMODEL_H
