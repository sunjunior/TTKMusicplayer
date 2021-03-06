#include "musicalbumfoundwidget.h"
#include "musicdownloadqueryfactory.h"
#include "musicsourcedownloadthread.h"
#include "musicsongssummarizied.h"
#include "musicconnectionpool.h"
#include "musicsettingmanager.h"
#include "musicuiobject.h"
#include "musiccoreutils.h"
#include "musictime.h"

#include "qrcodewidget.h"

#include <qmath.h>
#include <QCheckBox>
#include <QBoxLayout>
#include <QPushButton>
#include <QScrollArea>

MusicAlbumFoundTableWidget::MusicAlbumFoundTableWidget(QWidget *parent)
    : MusicQueryFoundTableWidget(parent)
{
    QHeaderView *headerview = horizontalHeader();
    headerview->resizeSection(0, 30);
    headerview->resizeSection(1, 449);
    headerview->resizeSection(2, 60);
    headerview->resizeSection(3, 26);
    headerview->resizeSection(4, 26);
    headerview->resizeSection(5, 26);

    M_CONNECTION_PTR->setValue(getClassName(), this);
    M_CONNECTION_PTR->poolConnect(getClassName(), MusicSongsSummarizied::getClassName());
}

MusicAlbumFoundTableWidget::~MusicAlbumFoundTableWidget()
{
    M_CONNECTION_PTR->removeValue(getClassName());
    clearAllItems();
}

QString MusicAlbumFoundTableWidget::getClassName()
{
    return staticMetaObject.className();
}

void MusicAlbumFoundTableWidget::setQueryInput(MusicDownLoadQueryThreadAbstract *query)
{
    MusicQueryFoundTableWidget::setQueryInput(query);
    if(parent()->metaObject()->indexOfSlot("queryAlbumFinished()") != -1)
    {
        connect(m_downLoadManager, SIGNAL(downLoadDataChanged(QString)), parent(), SLOT(queryAlbumFinished()));
    }
}

void MusicAlbumFoundTableWidget::resizeEvent(QResizeEvent *event)
{
    MusicQueryFoundTableWidget::resizeEvent(event);
    int width = M_SETTING_PTR->value(MusicSettingManager::WidgetSize).toSize().width();
    QHeaderView *headerview = horizontalHeader();
    headerview->resizeSection(1, (width - WINDOW_WIDTH_MIN)*0.9 + 449);
    headerview->resizeSection(2, (width - WINDOW_WIDTH_MIN)*0.1 + 60);

    for(int i=0; i<rowCount(); ++i)
    {
        QTableWidgetItem *it = item(i, 1);
        it->setText(MusicUtils::Widget::elidedText(font(), it->toolTip(), Qt::ElideRight, headerview->sectionSize(1) - 31));
    }
}



MusicAlbumFoundWidget::MusicAlbumFoundWidget(QWidget *parent)
    : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    m_mainWindow = new QWidget(this);
    m_mainWindow->setObjectName("MainWindow");
    m_mainWindow->setStyleSheet(QString("#MainWindow{%1}").arg(MusicUIObject::MBackgroundStyle17));
    layout->addWidget(m_mainWindow);
    setLayout(layout);

    m_statusLabel = new QLabel(tr("Loading Now ... "), m_mainWindow);
    m_statusLabel->setStyleSheet(MusicUIObject::MFontStyle05 + MusicUIObject::MFontStyle01);
    m_iconLabel = nullptr;

    QHBoxLayout *mLayout = new QHBoxLayout(m_mainWindow);
    mLayout->addWidget(m_statusLabel, 0, Qt::AlignCenter);
    m_mainWindow->setLayout(mLayout);

    m_albumTableWidget = new MusicAlbumFoundTableWidget(this);
    m_downloadThread = M_DOWNLOAD_QUERY_PTR->getQueryThread(this);
    connect(m_downloadThread, SIGNAL(downLoadDataChanged(QString)), SLOT(queryAllFinished()));

}

MusicAlbumFoundWidget::~MusicAlbumFoundWidget()
{
    delete m_albumTableWidget;
    delete m_downloadThread;
    delete m_statusLabel;
    delete m_iconLabel;
    delete m_mainWindow;
}

QString MusicAlbumFoundWidget::getClassName()
{
    return staticMetaObject.className();
}

void MusicAlbumFoundWidget::setSongName(const QString &name)
{
    m_songNameFull = name;
    m_downloadThread->setQueryAllRecords(false);
    m_downloadThread->startSearchSong(MusicDownLoadQueryThreadAbstract::MusicQuery,
                                      MusicUtils::Core::artistName(name));
}

void MusicAlbumFoundWidget::queryAllFinished()
{
    MusicObject::MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(musicSongInfos.isEmpty())
    {
        m_statusLabel->setPixmap(QPixmap(":/image/lb_noAlbum"));
    }
    else
    {
        delete m_statusLabel;
        m_statusLabel = nullptr;

        bool hasItem = false;
        foreach(const MusicObject::MusicSongInfomation &info, musicSongInfos)
        {
            if(m_songNameFull.contains(info.m_songName))
            {
                hasItem = true;
                m_albumTableWidget->setQueryInput(M_DOWNLOAD_QUERY_PTR->getAlbumThread(this));
                m_albumTableWidget->startSearchQuery(info.m_albumId);
                break;
            }
        }

        if(!hasItem)
        {
            createNoAlbumLabel();
        }
    }
}

void MusicAlbumFoundWidget::queryAlbumFinished()
{
    MusicObject::MusicSongInfomations musicSongInfos(m_albumTableWidget->getMusicSongInfos());
    if(musicSongInfos.isEmpty())
    {
        createNoAlbumLabel();
    }
    else
    {
        MusicObject::MusicSongInfomation currentInfo = musicSongInfos.first();
        QStringList lists = currentInfo.m_albumId.split("<>");
        if(lists.count() < 4)
        {
            createNoAlbumLabel();
            return;
        }
        for(int i=0; i<lists.count(); ++i)
        {
            if(lists[i].isEmpty())
            {
                lists[i] = "-";
            }
        }

        layout()->removeWidget(m_mainWindow);
        QScrollArea *scrollArea = new QScrollArea(this);
        scrollArea->setStyleSheet(MusicUIObject::MScrollBarStyle01);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setAlignment(Qt::AlignLeft);
        scrollArea->setWidget(m_mainWindow);
        layout()->addWidget(scrollArea);

        QWidget *function = new QWidget(m_mainWindow);
        function->setStyleSheet(MusicUIObject::MCheckBoxStyle01 + MusicUIObject::MPushButtonStyle03);
        QVBoxLayout *grid = new QVBoxLayout(function);

        QLabel *firstLabel = new QLabel(function);
        firstLabel->setText(tr("<font color=#169AF3> Alubm > %1 </font>").arg(lists[0]));
        grid->addWidget(firstLabel);
        ////////////////////////////////////////////////////////////////////////////
        QWidget *topFuncWidget = new QWidget(function);
        QHBoxLayout *topFuncLayout = new QHBoxLayout(topFuncWidget);

        m_iconLabel = new QLabel(topFuncWidget);
        m_iconLabel->setPixmap(QPixmap(":/image/lb_warning").scaled(180, 180));
        m_iconLabel->setFixedSize(180, 180);

        MusicSourceDownloadThread *download = new MusicSourceDownloadThread(this);
        connect(download, SIGNAL(downLoadByteDataChanged(QByteArray)), SLOT(downLoadFinished(QByteArray)));
        download->startToDownload(currentInfo.m_smallPicUrl);
        ////////////////////////////////////////////////////////////////////////////

        QWidget *topLineWidget = new QWidget(topFuncWidget);
        QVBoxLayout *topLineLayout = new QVBoxLayout(topLineWidget);
        topLineLayout->setContentsMargins(10, 5, 5, 0);
        QLabel *albumLabel = new QLabel(topLineWidget);
        albumLabel->setStyleSheet(MusicUIObject::MFontStyle01 + MusicUIObject::MFontStyle05);
        albumLabel->setText(MusicUtils::Widget::elidedText(albumLabel->font(), lists[0], Qt::ElideRight, 220));
        albumLabel->setToolTip(lists[0]);
        QLabel *singerLabel = new QLabel(topLineWidget);
        singerLabel->setStyleSheet(MusicUIObject::MColorStyle04 + MusicUIObject::MFontStyle03);
        singerLabel->setToolTip(tr("Singer: %1").arg(currentInfo.m_singerName));
        singerLabel->setText(MusicUtils::Widget::elidedText(singerLabel->font(), singerLabel->toolTip(), Qt::ElideRight, 220));
        QLabel *languageLabel = new QLabel(topLineWidget);
        languageLabel->setStyleSheet(MusicUIObject::MColorStyle04 + MusicUIObject::MFontStyle03);
        languageLabel->setToolTip(tr("Language: %1").arg(lists[1]));
        languageLabel->setText(MusicUtils::Widget::elidedText(languageLabel->font(), languageLabel->toolTip(), Qt::ElideRight, 220));
        QLabel *companyLabel = new QLabel(topLineWidget);
        companyLabel->setStyleSheet(MusicUIObject::MColorStyle04 + MusicUIObject::MFontStyle03);
        companyLabel->setToolTip(tr("Company: %1").arg(lists[2]));
        companyLabel->setText(MusicUtils::Widget::elidedText(companyLabel->font(), companyLabel->toolTip(), Qt::ElideRight, 220));
        QLabel *yearLabel = new QLabel(topLineWidget);
        yearLabel->setStyleSheet(MusicUIObject::MColorStyle04 + MusicUIObject::MFontStyle03);
        yearLabel->setToolTip(tr("Year: %1").arg(lists[3]));
        yearLabel->setText(MusicUtils::Widget::elidedText(yearLabel->font(), yearLabel->toolTip(), Qt::ElideRight, 220));

        topLineLayout->addWidget(albumLabel);
        topLineLayout->addWidget(singerLabel);
        topLineLayout->addWidget(languageLabel);
        topLineLayout->addWidget(companyLabel);
        topLineLayout->addWidget(yearLabel);
        topLineWidget->setLayout(topLineLayout);

        QWidget *topButtonWidget = new QWidget(topFuncWidget);
        QHBoxLayout *topButtonLayout = new QHBoxLayout(topButtonWidget);
        topButtonLayout->setContentsMargins(0, 0, 0, 0);
        QPushButton *playAllButton = new QPushButton(tr("playAll"), topButtonWidget);
        QPushButton *shareButton = new QPushButton(tr("share"), topButtonWidget);
        playAllButton->setIcon(QIcon(":/contextMenu/btn_play_white"));
        playAllButton->setIconSize(QSize(14, 14));
        playAllButton->setCursor(QCursor(Qt::PointingHandCursor));
        shareButton->setCursor(QCursor(Qt::PointingHandCursor));
        playAllButton->setFixedSize(90, 30);
        shareButton->setFixedSize(55, 30);
        topButtonLayout->addWidget(playAllButton);
        topButtonLayout->addWidget(shareButton);
        topButtonLayout->addStretch(1);
        topButtonWidget->setLayout(topButtonLayout);
        topLineLayout->addWidget(topButtonWidget);
        connect(playAllButton, SIGNAL(clicked()), SLOT(playAllButtonClicked()));
        connect(shareButton, SIGNAL(clicked()), SLOT(shareButtonClicked()));
        ////////////////////////////////////////////////////////////////////////////
        QWidget *topRightWidget = new QWidget(topFuncWidget);
        QGridLayout *topRightLayout = new QGridLayout(topRightWidget);
        topRightLayout->setContentsMargins(0, 0, 0, 0);
        topRightLayout->setSpacing(0);

        MusicTime::timeSRand();
        QLabel *numberLabel = new QLabel(topRightWidget);
        numberLabel->setAlignment(Qt::AlignCenter);
        numberLabel->setStyleSheet(MusicUIObject::MFontStyle06 + MusicUIObject::MColorStyle08);
        int number = qrand()%10;
        numberLabel->setText(QString("%1.%2").arg(number).arg(qrand()%10));
        topRightLayout->addWidget(numberLabel, 0, 0);
        for(int i=1; i<=5; ++i)
        {
            QLabel *label = new QLabel(topRightWidget);
            label->setPixmap(QPixmap( (ceil(number/2.0) - i) >= 0 ? ":/tiny/lb_star" : ":/tiny/lb_unstar"));
            topRightLayout->addWidget(label, 0, i);
        }

        QLabel *numberTextLabel = new QLabel(tr("Score:"), topRightWidget);
        topRightLayout->addWidget(numberTextLabel, 1, 0);
        for(int i=1; i<=5; ++i)
        {
            QLabel *label = new QLabel(topRightWidget);
            label->setFixedSize(26, 22);
            label->setPixmap(QPixmap(":/tiny/lb_unstar"));
            topRightLayout->addWidget(label, 1, i);
        }

        QLabel *marginBottmLabel = new QLabel(topRightWidget);
        marginBottmLabel->setFixedHeight(40);
        topRightLayout->addWidget(marginBottmLabel, 2, 0);
        topRightWidget->setLayout(topRightLayout);

        QRCodeQWidget *code = new QRCodeQWidget(QByteArray(), QSize(90, 90), topRightWidget);
        code->setMargin(2);
        code->setIcon(":/image/lb_player_logo", 0.23);
        topRightLayout->addWidget(code, 3, 2, 1, 6);

        topFuncLayout->addWidget(m_iconLabel);
        topFuncLayout->addWidget(topLineWidget);
        topFuncLayout->addWidget(topRightWidget);
        topFuncWidget->setLayout(topFuncLayout);
        grid->addWidget(topFuncWidget);
        ////////////////////////////////////////////////////////////////////////////
        QLabel *songItems = new QLabel("|" + tr("songItems") + QString("(%1)").arg(musicSongInfos.count()), function);
        songItems->setFixedHeight(50);
        songItems->setStyleSheet(MusicUIObject::MFontStyle04);
        grid->addWidget(songItems);

        QWidget *middleFuncWidget = new QWidget(function);
        QHBoxLayout *middleFuncLayout = new QHBoxLayout(middleFuncWidget);
        middleFuncLayout->setContentsMargins(0, 0, 0, 0);
        QLabel *marginLabel = new QLabel(middleFuncWidget);
        marginLabel->setFixedWidth(1);
        QCheckBox *allCheckBox = new QCheckBox(" " + tr("all"), middleFuncWidget);
        QPushButton *playButton = new QPushButton(tr("play"), middleFuncWidget);
        playButton->setIcon(QIcon(":/contextMenu/btn_play_white"));
        playButton->setIconSize(QSize(14, 14));
        playButton->setFixedSize(55, 25);
        playButton->setCursor(QCursor(Qt::PointingHandCursor));
        QPushButton *addButton = new QPushButton(tr("add"), middleFuncWidget);
        addButton->setFixedSize(55, 25);
        addButton->setCursor(QCursor(Qt::PointingHandCursor));
        QPushButton *downloadButton = new QPushButton(tr("download"), middleFuncWidget);
        downloadButton->setFixedSize(55, 25);
        downloadButton->setCursor(QCursor(Qt::PointingHandCursor));

        middleFuncLayout->addWidget(marginLabel);
        middleFuncLayout->addWidget(allCheckBox);
        middleFuncLayout->addStretch(1);
        middleFuncLayout->addWidget(playButton);
        middleFuncLayout->addWidget(addButton);
        middleFuncLayout->addWidget(downloadButton);
        connect(allCheckBox, SIGNAL(clicked(bool)), m_albumTableWidget, SLOT(setSelectedAllItems(bool)));
        connect(playButton, SIGNAL(clicked()), SLOT(playButtonClicked()));
        connect(downloadButton, SIGNAL(clicked()), SLOT(downloadButtonClicked()));
        connect(addButton, SIGNAL(clicked()), SLOT(addButtonClicked()));
        grid->addWidget(middleFuncWidget);
        //////////////////////////////////////////////////////////////////////
        grid->addWidget(m_albumTableWidget);
        grid->addStretch(1);

        function->setLayout(grid);
        m_mainWindow->layout()->addWidget(function);

        m_resizeWidget << albumLabel << singerLabel << languageLabel << companyLabel << yearLabel;
    }
}

void MusicAlbumFoundWidget::downLoadFinished(const QByteArray &data)
{
    if(m_iconLabel)
    {
        QPixmap pix;
        pix.loadFromData(data);
        m_iconLabel->setPixmap(pix.scaled(m_iconLabel->size()));
    }
}

void MusicAlbumFoundWidget::playAllButtonClicked()
{
    m_albumTableWidget->setSelectedAllItems(true);
    m_albumTableWidget->downloadDataFrom(true);
}

void MusicAlbumFoundWidget::shareButtonClicked()
{

}

void MusicAlbumFoundWidget::playButtonClicked()
{
    m_albumTableWidget->downloadDataFrom(true);
}

void MusicAlbumFoundWidget::downloadButtonClicked()
{
    foreach(int index, m_albumTableWidget->getSelectedItems())
    {
        m_albumTableWidget->musicDownloadLocal(index);
    }
}

void MusicAlbumFoundWidget::addButtonClicked()
{
    m_albumTableWidget->downloadDataFrom(false);
}

void MusicAlbumFoundWidget::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event);
}

void MusicAlbumFoundWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if(!m_resizeWidget.isEmpty())
    {
        int width = M_SETTING_PTR->value(MusicSettingManager::WidgetSize).toSize().width();
        width = width - WINDOW_WIDTH_MIN;

        QLabel *label = m_resizeWidget[0];
        label->setText(MusicUtils::Widget::elidedText(label->font(), label->toolTip(), Qt::ElideRight, 220 + width));

        label = m_resizeWidget[1];
        label->setText(MusicUtils::Widget::elidedText(label->font(), label->toolTip(), Qt::ElideRight, 220 + width));

        label = m_resizeWidget[2];
        label->setText(MusicUtils::Widget::elidedText(label->font(), label->toolTip(), Qt::ElideRight, 220 + width));

        label = m_resizeWidget[3];
        label->setText(MusicUtils::Widget::elidedText(label->font(), label->toolTip(), Qt::ElideRight, 220 + width));

        label = m_resizeWidget[4];
        label->setText(MusicUtils::Widget::elidedText(label->font(), label->toolTip(), Qt::ElideRight, 220 + width));
    }
}

void MusicAlbumFoundWidget::createNoAlbumLabel()
{
    m_statusLabel = new QLabel(this);
    m_statusLabel->setPixmap(QPixmap(":/image/lb_noAlbum"));
    MStatic_cast(QHBoxLayout*, m_mainWindow->layout())->addWidget(m_statusLabel, 0, Qt::AlignCenter);
    m_albumTableWidget->hide();
}
