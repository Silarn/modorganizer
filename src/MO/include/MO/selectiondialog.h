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

#ifndef SELECTIONDIALOG_H
#define SELECTIONDIALOG_H

#include <QDialog>
#include <QIcon>
#include <QSize>
#include <QString>
#include <QVariant>

#include <memory>

class QAbstractButton;
class QWidget;

namespace Ui {
class SelectionDialog;
}

// Dialog used for Multiple Choice Selection.
// Displays a list of choices
class SelectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit SelectionDialog(const QString& description, QWidget* parent = nullptr, const QSize& iconSize = QSize());

    ~SelectionDialog();

    /**
     * @brief add a choice to the dialog
     * @param buttonText the text to be displayed on the button
     * @param description the description that shows up under in small letters inside the button
     * @param data data to be stored with the button. Please note that as soon as one choice has data associated with it
     * (non-invalid QVariant) all buttons that contain no data will be treated as "cancel" buttons
     * @param icon Optional icon to be used for the choice.
     */
    void addChoice(const QString& buttonText, const QString& description, const QVariant& data,
                   const QIcon& icon = QIcon());

    // Return the number of choices.
    int numChoices() const;

    // Return information from the currently selected Choice.
    QVariant getChoiceData();
    QString getChoiceString();

    // Disable the cancel button.
    void disableCancel();

private slots:
    void on_buttonBox_clicked(QAbstractButton* button);
    void on_cancelButton_clicked();

private:
    std::unique_ptr<Ui::SelectionDialog> ui;
    QAbstractButton* m_Choice = nullptr;
    QSize m_IconSize;
};

#endif // SELECTIONDIALOG_H
