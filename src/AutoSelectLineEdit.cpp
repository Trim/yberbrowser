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

#include "AutoSelectLineEdit.h"
#include <QUrl>
#include "AutoSelectLineEdit_p.h"
#include "KeypadWidget.h"
#include "PopupView.h"

/*! \class AutoSelectLineEdit input element (\QLineEdit) that selects
  its contents when focused in.

  Needed, because I did not find the feature from stock Qt widgets.
 */
AutoSelectLineEdit::AutoSelectLineEdit(QGraphicsItem* parent)
    : QGraphicsWidget(parent)
    , d(new AutoSelectLineEditPrivate(this))
    , m_virtualKeypad(0)
    , m_urlfilterPopup(0)
{
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    // create internal widgets
    QGraphicsProxyWidget* proxy;
    proxy = new QGraphicsProxyWidget(this);
    proxy->setWidget(d);
    proxy->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QGraphicsLinearLayout* layout;
    layout = new QGraphicsLinearLayout(Qt::Horizontal, this);
    setLayout(layout);
    layout->addItem(proxy);

    // remove default background
    d->setFrame(false);
    d->setAttribute(Qt::WA_NoSystemBackground);
    d->setStyleSheet("background: transparent;");

    setFocusProxy(proxy);

    // set font settings
    QFont font("Nokia Sans", 18);
    QPalette palette = d->palette();
    palette.setColor(QPalette::Text, Qt::white);
    d->setPalette(palette);
    d->setFont(font);
}

AutoSelectLineEdit::~AutoSelectLineEdit()
{
    delete m_virtualKeypad;
    delete m_urlfilterPopup;
    delete d;
}

QString AutoSelectLineEdit::text()
{
    return d->url;
}

void AutoSelectLineEdit::setText(const QString& text)
{
    // FIXME: Move elsewhere.
    // When the user is editing, we don't want a loadFinished to override the current text.
    //if (d->hasFocus())
    //    return;
    d->url = text;
    d->adjustText();
}

int AutoSelectLineEdit::selectionStart() const
{
    return d->selectionStart();
}

void AutoSelectLineEdit::setCursorPosition(int pos)
{
    d->setCursorPosition(pos);
}

void AutoSelectLineEdit::setSelection(int start, int length)
{
    d->setSelection(start, length);
}

void AutoSelectLineEdit::togglePopup()
{
    if (!m_virtualKeypad) {
        popupDismissed();
        createVirtualKeypad();
    } else {
        keypadDismissed();
        createUrlFilterPopup();
    }
}

void AutoSelectLineEdit::newEditedText(const QString& newText)
{
    // this is a non-programatic text change (typing)
    keypadDismissed();
    // create home view and remove popupview when no text in the url field
    if (newText.isEmpty())
        popupDismissed();
    else {
        if (!m_urlfilterPopup)
            createUrlFilterPopup();
        m_urlfilterPopup->setFilterText(newText);
    }
    update();
    emit textEdited(newText);
}

void AutoSelectLineEdit::popupItemSelected(const QUrl& url)
{
    cleanupAndSendFinished(url.toString());
}

void AutoSelectLineEdit::popupDismissed()
{
    d->setFocus();
    delete m_urlfilterPopup;
    m_urlfilterPopup = 0;
    update();
}

void AutoSelectLineEdit::keyPadCharEntered(char key)
{
    // FIXME: there must be a less hackish way to do it
    if (d->hasSelectedText())
        d->url.remove(d->selectionStart(), d->selectedText().size()); 
    setText(d->url + key);
    emit textEdited(text());
}

void AutoSelectLineEdit::keyPadBackspace()
{
    // cant do d->backspace() as that goes through the typing codepath (apparently a programmatically change), 
    // removing the keypad
    QString newText(d->url);
    if (d->hasSelectedText())
        newText.remove(d->selectionStart(), d->selectedText().size()); 
    else if (!newText.isEmpty())
        newText.remove(newText.size() - 1, 1);
    setText(newText);
    emit textEdited(text());
}

void AutoSelectLineEdit::keyPadEnter()
{
    cleanupAndSendFinished(text());
}

void AutoSelectLineEdit::keypadDismissed()
{
    d->setFocus();
    delete m_virtualKeypad;
    m_virtualKeypad = 0;
    update();
}

void AutoSelectLineEdit::returnPressed()
{
    cleanupAndSendFinished(text());
}

bool AutoSelectLineEdit::realFocusEvent()
{
    return !m_urlfilterPopup && !m_virtualKeypad;
}

void AutoSelectLineEdit::createVirtualKeypad()
{
    m_virtualKeypad = new KeypadWidget(clientRect(), this);
    m_virtualKeypad->appear(rect().top());
    
    connect(m_virtualKeypad, SIGNAL(charEntered(char)), SLOT(keyPadCharEntered(char)));
    connect(m_virtualKeypad, SIGNAL(enter()), SLOT(keyPadEnter()));
    connect(m_virtualKeypad, SIGNAL(backspace()), SLOT(keyPadBackspace()));
    connect(m_virtualKeypad, SIGNAL(dismissed()), SLOT(keypadDismissed()));
}

void AutoSelectLineEdit::createUrlFilterPopup()
{
    QRectF r(clientRect());
    m_urlfilterPopup = new PopupView(this);
    m_urlfilterPopup->setPos(r.topLeft());
    m_urlfilterPopup->resize(r.size());
    m_urlfilterPopup->appear();
    // only the nonselected text to not to include autocomplete in the filter 
    // (bugish as the user can very well do text selection too, sorry user)
    QString str(text());
    if (d->hasSelectedText())
        str.remove(d->selectionStart(), d->selectedText().size()); 
    m_urlfilterPopup->setFilterText(str);

    connect(m_urlfilterPopup, SIGNAL(pageSelected(const QUrl&)), this, SLOT(popupItemSelected(const QUrl&)));
    connect(m_urlfilterPopup, SIGNAL(viewDismissed()), this, SLOT(popupDismissed()));
}

QRectF AutoSelectLineEdit::clientRect()
{
    // FIXME figure out this funky - business
    QRectF r(parentWidget()->geometry());
    r.moveLeft(r.left() - pos().x());
    r.moveTop(r.top() - pos().y());
    r.setHeight(r.height() - rect().size().height());
    return r;
}

void AutoSelectLineEdit::cleanupAndSendFinished(const QString& text)
{
    popupDismissed();
    keypadDismissed();
    emit textEditingFinished(text);
}

