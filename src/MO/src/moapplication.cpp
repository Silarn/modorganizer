/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "MO/moapplication.h"

#include <MO/Shared/appconfig.h>
#include <QFile>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <uibase/report.h>

using MOBase::reportError;

class ProxyStyle : public QProxyStyle {
public:
    ProxyStyle(QStyle* baseStyle = 0) : QProxyStyle(baseStyle) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget) const {
        // Custom behaviour for drag and drop only.
        if (element == QStyle::PE_IndicatorItemViewItemDrop) {
            painter->setRenderHint(QPainter::Antialiasing, true);

            QColor col(option->palette.foreground().color());
            QPen pen(col);
            pen.setWidth(2);
            col.setAlpha(50);
            QBrush brush(col);

            painter->setPen(pen);
            painter->setBrush(brush);
            if (option->rect.height() == 0) {
                QPoint tri[3] = {option->rect.topLeft(), option->rect.topLeft() + QPoint(-5, 5),
                                 option->rect.topLeft() + QPoint(-5, -5)};
                painter->drawPolygon(tri, 3);
                painter->drawLine(QPoint(option->rect.topLeft().x(), option->rect.topLeft().y()),
                                  option->rect.topRight());
            } else {
                painter->drawRoundedRect(option->rect, 5, 5);
            }
        } else {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    }
};

MOApplication::MOApplication(int& argc, char** argv) : QApplication(argc, argv) {
    connect(&m_StyleWatcher, SIGNAL(fileChanged(QString)), SLOT(updateStyle(QString)));
    m_DefaultStyle = style()->objectName();
    setStyle(new ProxyStyle(style()));
}

bool MOApplication::setStyleFile(const QString& styleName) {
    // remove all files from watch
    QStringList currentWatch = m_StyleWatcher.files();
    if (currentWatch.count() != 0) {
        m_StyleWatcher.removePaths(currentWatch);
    }
    // set new stylesheet or clear it
    if (styleName.length() != 0) {
        // Attempt to create a path to one of our own styles
        QString styleSheetName =
            applicationDirPath() + "/" + QString::fromStdWString(AppConfig::stylesheetsPath()) + "/" + styleName;
        // If it's a real style, use it.
        // Otherwise pass directly to updateStyle and let it figure it out.
        // Assuming styleName is a QT one or actually a path of it's own.
        if (QFile::exists(styleSheetName)) {
            m_StyleWatcher.addPath(styleSheetName);
            updateStyle(styleSheetName);
        } else {
            updateStyle(styleName);
        }
    } else {
        setStyle(new ProxyStyle(QStyleFactory::create(m_DefaultStyle)));
        setStyleSheet("");
    }
    return true;
}

// Intercept all events and report errors if they occur.
bool MOApplication::notify(QObject* receiver, QEvent* event) {
    try {
        return QApplication::notify(receiver, event);
    } catch (const std::exception& e) {
        qCritical("uncaught exception in handler (object %s, eventtype %d): %s",
                  receiver->objectName().toUtf8().constData(), event->type(), e.what());
        reportError(tr("an error occured: %1").arg(e.what()));
    } catch (...) {
        qCritical("uncaught non-std exception in handler (object %s, eventtype %d)",
                  receiver->objectName().toUtf8().constData(), event->type());
        reportError(tr("an error occured"));
    }
    return false;
}

void MOApplication::updateStyle(const QString& fileName) {
    if (fileName == "Fusion") {
        // Create the fusion style and apply it, along with an empty stylesheet.
        setStyle(QStyleFactory::create("fusion"));
        setStyleSheet("");
    } else {
        // Reset the style to default, then apply the stylesheet.
        setStyle(new ProxyStyle(QStyleFactory::create(m_DefaultStyle)));
        // Set the stylesheet, if the file exists.
        if (QFile::exists(fileName)) {
            setStyleSheet(QString("file:///%1").arg(fileName));
        } else {
            qWarning("invalid stylesheet: %s", qUtf8Printable(fileName));
        }
    }
}
