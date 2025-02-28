/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2016  Alexandr Milovantsev <dzmat@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "shadowbanlistoptionsdialog.h"

#include <QHostAddress>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStringListModel>

#include "base/bittorrent/session.h"
#include "base/utils/net.h"
#include "ui_shadowbanlistoptionsdialog.h"
#include "utils.h"

#define SETTINGS_KEY(name) u"ShadowBanListOptionsDialog/" name

ShadowBanListOptionsDialog::ShadowBanListOptionsDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::ShadowBanListOptionsDialog)
    , m_storeDialogSize(SETTINGS_KEY(u"Size"_s))
    , m_model(new QStringListModel(BitTorrent::Session::instance()->shadowBannedIPs(), this))
{
    m_ui->setupUi(this);

    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_sortFilter = new QSortFilterProxyModel(this);
    m_sortFilter->setDynamicSortFilter(true);
    m_sortFilter->setSourceModel(m_model);

    m_ui->bannedIPList->setModel(m_sortFilter);
    m_ui->bannedIPList->sortByColumn(0, Qt::AscendingOrder);
    m_ui->buttonBanIP->setEnabled(false);

    if (const QSize dialogSize = m_storeDialogSize; dialogSize.isValid())
        resize(dialogSize);
}

ShadowBanListOptionsDialog::~ShadowBanListOptionsDialog()
{
    m_storeDialogSize = size();
    delete m_ui;
}

void ShadowBanListOptionsDialog::on_buttonBox_accepted()
{
    if (m_modified)
    {
        // save to session
        QStringList IPList;
        // Operate on the m_sortFilter to grab the strings in sorted order
        for (int i = 0; i < m_sortFilter->rowCount(); ++i)
        {
            QModelIndex index = m_sortFilter->index(i, 0);
            IPList << index.data().toString();
        }
        BitTorrent::Session::instance()->setShadowBannedIPs(IPList);
        QDialog::accept();
    }
    else
    {
        QDialog::reject();
    }
}

void ShadowBanListOptionsDialog::on_buttonBanIP_clicked()
{
    QString ip = m_ui->txtIP->text();
    if (!Utils::Net::isValidIP(ip))
    {
        QMessageBox::warning(this, tr("Warning"), tr("The entered IP address is invalid."));
        return;
    }
    // the same IPv6 addresses could be written in different forms;
    // QHostAddress::toString() result format follows RFC5952;
    // thus we avoid duplicate entries pointing to the same address
    ip = QHostAddress(ip).toString();
    for (int i = 0; i < m_sortFilter->rowCount(); ++i)
    {
        QModelIndex index = m_sortFilter->index(i, 0);
        if (ip == index.data().toString())
        {
            QMessageBox::warning(this, tr("Warning"), tr("The entered IP is already banned."));
            return;
        }
    }

    m_model->insertRow(m_model->rowCount());
    m_model->setData(m_model->index(m_model->rowCount() - 1, 0), ip);
    m_ui->txtIP->clear();
    m_modified = true;
}

void ShadowBanListOptionsDialog::on_buttonDeleteIP_clicked()
{
    QModelIndexList selection = m_ui->bannedIPList->selectionModel()->selectedIndexes();
    std::sort(selection.begin(), selection.end(), [](const QModelIndex &left, const QModelIndex &right)
    {
        return (left.row() > right.row());
    });
    for (const QModelIndex &i : selection)
        m_sortFilter->removeRow(i.row());

    m_modified = true;
}

void ShadowBanListOptionsDialog::on_txtIP_textChanged(const QString &ip)
{
    m_ui->buttonBanIP->setEnabled(Utils::Net::isValidIP(ip));
}
