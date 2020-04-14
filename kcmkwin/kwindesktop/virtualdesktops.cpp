/*
 * UKUI-KWin - the UKUI3.0 window manager
 * This file is part of the UKUI project
 * The ukui-kwin is forked from kwin
 *    
 * Copyright (C) 2014-2020 kylinos.cn
 *
 * Copyright (C) 2018 Eike Hein <hein@kde.org>
 * Copyright (C) 2018 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "virtualdesktops.h"
#include "animationsmodel.h"
#include "desktopsmodel.h"
#include "ukuivirtualdesktopssettings.h"

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>

K_PLUGIN_FACTORY_WITH_JSON(VirtualDesktopsFactory, "kcm_ukuikwin_virtualdesktops.json", registerPlugin<KWin::VirtualDesktops>();)

namespace KWin
{

VirtualDesktops::VirtualDesktops(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_settings(new VirtualDesktopsSettings(this))
    , m_desktopsModel(new KWin::DesktopsModel(this))
    , m_animationsModel(new AnimationsModel(this))
{
    KAboutData *about = new KAboutData(QStringLiteral("kcm_ukuikwin_virtualdesktops"),
        i18n("Virtual Desktops"),
        QStringLiteral("2.0"), QString(), KAboutLicense::GPL);
    setAboutData(about);

    qmlRegisterType<VirtualDesktopsSettings>();

    setButtons(Apply | Default);

    QObject::connect(m_desktopsModel, &KWin::DesktopsModel::userModifiedChanged,
        this, &VirtualDesktops::settingsChanged);
    connect(m_animationsModel, &AnimationsModel::enabledChanged,
        this, &VirtualDesktops::settingsChanged);
    connect(m_animationsModel, &AnimationsModel::currentIndexChanged,
        this, &VirtualDesktops::settingsChanged);
}

VirtualDesktops::~VirtualDesktops()
{
}

QAbstractItemModel *VirtualDesktops::desktopsModel() const
{
    return m_desktopsModel;
}

QAbstractItemModel *VirtualDesktops::animationsModel() const
{
    return m_animationsModel;
}

VirtualDesktopsSettings *VirtualDesktops::virtualDesktopsSettings() const
{
    return m_settings;
}

void VirtualDesktops::load()
{
    ManagedConfigModule::load();

    m_desktopsModel->load();
    m_animationsModel->load();
}

void VirtualDesktops::save()
{
    ManagedConfigModule::save();

    m_desktopsModel->syncWithServer();
    m_animationsModel->save();

    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KWin"),
        QStringLiteral("org.ukui.KWin"), QStringLiteral("reloadConfig"));
    QDBusConnection::sessionBus().send(message);
}

void VirtualDesktops::defaults()
{
    ManagedConfigModule::defaults();

    m_desktopsModel->defaults();
    m_animationsModel->defaults();
}

bool VirtualDesktops::isDefaults() const
{
    return m_animationsModel->isDefaults() && m_desktopsModel->isDefaults();
}

void VirtualDesktops::configureAnimation()
{
    const QModelIndex index = m_animationsModel->index(m_animationsModel->currentIndex(), 0);
    if (!index.isValid()) {
        return;
    }

    m_animationsModel->requestConfigure(index, nullptr);
}

void VirtualDesktops::showAboutAnimation()
{
    const QModelIndex index = m_animationsModel->index(m_animationsModel->currentIndex(), 0);
    if (!index.isValid()) {
        return;
    }

    const QString name    = index.data(AnimationsModel::NameRole).toString();
    const QString comment = index.data(AnimationsModel::DescriptionRole).toString();
    const QString author  = index.data(AnimationsModel::AuthorNameRole).toString();
    const QString email   = index.data(AnimationsModel::AuthorEmailRole).toString();
    const QString website = index.data(AnimationsModel::WebsiteRole).toString();
    const QString version = index.data(AnimationsModel::VersionRole).toString();
    const QString license = index.data(AnimationsModel::LicenseRole).toString();
    const QString icon    = index.data(AnimationsModel::IconNameRole).toString();

    const KAboutLicense::LicenseKey licenseType = KAboutLicense::byKeyword(license).key();

    KAboutData aboutData(
        name,              // Plugin name
        name,              // Display name
        version,           // Version
        comment,           // Short description
        licenseType,       // License
        QString(),         // Copyright statement
        QString(),         // Other text
        website.toLatin1() // Home page
    );
    aboutData.setProgramLogo(icon);

    const QStringList authors = author.split(',');
    const QStringList emails = email.split(',');

    if (authors.count() == emails.count()) {
        int i = 0;
        for (const QString &author : authors) {
            if (!author.isEmpty()) {
                aboutData.addAuthor(i18n(author.toUtf8()), QString(), emails[i]);
            }
            i++;
        }
    }

    QPointer<KAboutApplicationDialog> aboutPlugin = new KAboutApplicationDialog(aboutData);
    aboutPlugin->exec();

    delete aboutPlugin;
}

bool VirtualDesktops::isSaveNeeded() const
{
    return m_animationsModel->needsSave() || m_desktopsModel->needsSave();
}

}

#include "virtualdesktops.moc"
