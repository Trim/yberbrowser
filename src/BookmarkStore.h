/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef BookmarkStore_h_
#define BookmarkStore_h_

#include <QObject>
#include <QList>
#include <QUrl>
#include <QIcon>
#include "UrlItem.h"

typedef QList<UrlItem*> BookmarkList;

class BookmarkStore : public QObject {
    Q_OBJECT
public:
    static BookmarkStore* instance();    

    void add(const QUrl& url, const QString& title, QIcon favicon);
    void remove(const QString& url);
    UrlList* list() { return &m_list; }

private:
    BookmarkStore();
    ~BookmarkStore();

private Q_SLOTS:
    void externalize();

private:
    UrlList m_list;
    bool m_needsPersisting;
};

#endif

