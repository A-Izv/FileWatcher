#ifndef WIDGET_H
#define WIDGET_H
//------------------------------------------------------------------------------
#include <QWidget>
//------------------------------------------------------------------------------
#include <QTime>
#include <QDebug>
#include <QSettings>
#include <QFileDialog>
#include <QFileSystemWatcher>
//------------------------------------------------------------------------------
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>
//------------------------------------------------------------------------------
namespace Ui {
class mainWDG;
}
//------------------------------------------------------------------------------
class mainWDG : public QWidget
{
    Q_OBJECT

    static const int    BSIZE = 1024*1024;
public:
    explicit            mainWDG(QWidget *parent = 0);
                       ~mainWDG();

private slots:
    void                on_dirNamePB_clicked();

    void                on_dirChange( const QString& path );

    void                trayClicks( QSystemTrayIcon::ActivationReason r );
private:
    Ui::mainWDG         *ui;

    QSettings           settings;
    QString             dirName;

    QFileSystemWatcher  fsw;
    QStringList         fileList;

    int                 *buf;
    int                 table[256];

    QMenu               *trayMenu;
    QSystemTrayIcon     *trayIcon;

    void                hideEvent( QHideEvent * );

    void                log( const QString &s, const QString &title = QString() );

    void                startWatching();
    void                stopWatching();

    void                process( const QString &name );
};
//------------------------------------------------------------------------------
#endif // WIDGET_H
