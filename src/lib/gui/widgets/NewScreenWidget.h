/*
 * Synergy -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <https://github.com/Tupig/Tupig_synergy/issues>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QLabel>

class QMouseEvent;
class QWidget;

class NewScreenWidget : public QLabel
{
  Q_OBJECT

public:
  explicit NewScreenWidget(QWidget *parent);

protected:
  void mousePressEvent(QMouseEvent *) override;
};
